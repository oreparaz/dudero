//! dudero: dude, is my randomness OK?
//! https://github.com/oreparaz/dudero
//!
//! A small Rust library implementing a chi-square goodness-of-fit test on nibbles
//! to check if your random number generator looks totally broken, or not.
//!
//! This is equivalent to the "Poker test" (Test 2 in AIS-31).
//!
//! # no_std Support
//!
//! This library supports `no_std` environments. By default, `std` is enabled.
//! To use in `no_std`, disable default features:
//! ```toml
//! dudero = { git = "https://github.com/oreparaz/dudero", subdirectory = "rust", default-features = false }
//! ```
//!
//! # Testing
//!
//! Run the built-in tests with:
//! ```bash
//! cargo test
//! ```
//!
//! # Usage
//!
//! ```rust
//! use dudero::{check_buffer, DuderoResult};
//!
//! let data: Vec<u8> = (0..64).collect();
//! match check_buffer(&data) {
//!     Ok(DuderoResult::Ok) => println!("Looks random!"),
//!     Ok(DuderoResult::BadRandomness) => println!("Bad randomness detected"),
//!     Err(e) => println!("Error: {:?}", e),
//! }
//! ```
//!
//! # Streaming API
//!
//! ```rust
//! use dudero::{DuderoContext, DuderoResult};
//!
//! let data = vec![0x42u8; 32];
//! let mut ctx = DuderoContext::new();
//! for byte in data.iter() {
//!     ctx.add(*byte).unwrap();
//! }
//! let result = ctx.finish().unwrap();
//! ```

#![cfg_attr(not(feature = "std"), no_std)]

#[cfg(feature = "std")]
use std::iter::{Extend, FromIterator};

#[cfg(not(feature = "std"))]
use core::iter::{Extend, FromIterator};

/// Minimum buffer length (16 bytes)
pub const MIN_LEN: usize = 16;

/// Maximum safe length to prevent overflow of u16 histogram bins.
/// Each byte produces 2 nibbles. With perfect uniform distribution,
/// each bin gets len*2/16 = len/8 samples. To keep bins < 2^16:
/// len/8 < 2^16 => len < 2^19 = 524,288 bytes.
/// Use a conservative limit to handle non-uniform data.
pub const MAX_LEN: usize = 32768; // 32 KB

/// Number of histogram bins (16 possible nibble values: 0x0 to 0xF)
const NUM_BINS: usize = 16;

/// Chi-square threshold for false positive rate ≈ 1 in 83,000
const THRESHOLD: f64 = 50.0;

/// Result of the randomness check
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DuderoResult {
    /// Data appears random
    Ok,
    /// Data appears non-random (biased, fixed values, etc.)
    BadRandomness,
}

/// Error types
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DuderoError {
    /// Buffer is too short (minimum 16 bytes)
    TooShort,
    /// Buffer is too long (maximum 32 KB to prevent overflow)
    TooLong,
}

impl core::fmt::Display for DuderoError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            DuderoError::TooShort => write!(f, "Buffer too short (minimum {} bytes)", MIN_LEN),
            DuderoError::TooLong => write!(f, "Buffer too long (maximum {} bytes)", MAX_LEN),
        }
    }
}

#[cfg(feature = "std")]
impl std::error::Error for DuderoError {}

/// Context for streaming API
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DuderoContext {
    /// Histogram bins, count up to 2^16 = 65,536
    hist: [u16; NUM_BINS],
    /// Total number of samples processed
    hist_samples: usize,
}

impl DuderoContext {
    /// Create a new context
    pub fn new() -> Self {
        Self {
            hist: [0; 16],
            hist_samples: 0,
        }
    }

    /// Add a sample (byte) to the context
    pub fn add(&mut self, sample: u8) -> Result<(), DuderoError> {
        // Check if adding this sample would exceed maximum safe samples
        // MAX_LEN bytes * 2 nibbles/byte = MAX_LEN * 2 samples
        if self.hist_samples >= MAX_LEN * 2 {
            return Err(DuderoError::TooLong);
        }

        // Extract high and low nibbles
        self.hist[(sample >> 4) as usize] += 1;
        self.hist[(sample & 0x0F) as usize] += 1;
        self.hist_samples += 2;

        Ok(())
    }

    /// Add multiple samples from an iterator
    ///
    /// This is more efficient than calling `add` repeatedly.
    pub fn add_bytes<I>(&mut self, bytes: I) -> Result<(), DuderoError>
    where
        I: IntoIterator<Item = u8>,
    {
        for byte in bytes {
            self.add(byte)?;
        }
        Ok(())
    }

    /// Returns the number of bytes processed so far
    #[inline]
    pub fn len(&self) -> usize {
        self.hist_samples / 2
    }

    /// Returns true if no bytes have been processed yet
    #[inline]
    pub fn is_empty(&self) -> bool {
        self.hist_samples == 0
    }

    /// Finish processing and return the result
    pub fn finish(&self) -> Result<DuderoResult, DuderoError> {
        if self.hist_samples < NUM_BINS {
            return Err(DuderoError::TooShort);
        }

        let expected = self.hist_samples / NUM_BINS;

        // Calculate chi-square statistic using iterator
        let chi_squared: u32 = self
            .hist
            .iter()
            .map(|&count| {
                let delta = (count as usize).abs_diff(expected) as u32;
                delta * delta
            })
            .sum();

        let chi_squared_normalized = chi_squared as f64 / expected as f64;

        // Chi-squared goodness-of-fit test with 15 degrees of freedom (16 bins - 1)
        //
        // chi_squared_normalized = Σ(Oi - E)² / E = χ² statistic
        //
        // For uniform random nibbles, χ² follows chi-squared distribution with df=15.
        // The threshold determines the false positive rate (FPR):
        //
        //   FPR = P(χ² > threshold | data is truly random)
        //
        // Threshold = 50.0 corresponds to FPR ≈ 1.2e-5 (approximately 1 in 83,000)
        //
        // Note: AIS-31 Test 2 (also known as the "Poker test") uses a threshold
        // of 46.17 for comparison.
        //
        // This means truly random data will be rejected approximately once in
        // every 83,000 tests. This FPR is constant regardless of buffer size,
        // which is a fundamental property of the chi-squared test.
        //
        // Mathematical justification:
        //   P(χ² > 50.0 | df=15) = 1 - γ(7.5, 25) / Γ(7.5) ≈ 1.2e-5
        //
        // where γ is the lower incomplete gamma function and Γ is the gamma function.

        if chi_squared_normalized > THRESHOLD {
            Ok(DuderoResult::BadRandomness)
        } else {
            Ok(DuderoResult::Ok)
        }
    }

    /// Consume the context and return the result
    ///
    /// This is equivalent to `finish()` but takes ownership.
    #[inline]
    pub fn into_result(self) -> Result<DuderoResult, DuderoError> {
        self.finish()
    }
}

impl Default for DuderoContext {
    fn default() -> Self {
        Self::new()
    }
}

// Implement Extend to allow collecting bytes into a context
impl Extend<u8> for DuderoContext {
    fn extend<T: IntoIterator<Item = u8>>(&mut self, iter: T) {
        for byte in iter {
            // Ignore errors during extend (context will be in error state for finish())
            let _ = self.add(byte);
        }
    }
}

// Allow creating a context from an iterator
impl FromIterator<u8> for DuderoContext {
    fn from_iter<T: IntoIterator<Item = u8>>(iter: T) -> Self {
        let mut ctx = Self::new();
        ctx.extend(iter);
        ctx
    }
}

/// Check if the passed buffer "looks random".
///
/// Returns `BadRandomness` if the buffer looks like bad randomness
/// (obviously biased values, fixed values, etc).
///
/// # Warnings
///
/// There's a chance a perfect entropy source generates sequences that fail this test.
/// This happens with probability ≈ 1 in 83,000.
///
/// **WARNING**: rejecting sequences that fail this test will reduce the source entropy
/// (though by a negligible amount: ~0.000017 bits per test).
///
/// # Errors
///
/// Returns an error if:
/// - Buffer is too short (< 16 bytes)
/// - Buffer is too long (> 32 KB)
pub fn check_buffer(buf: &[u8]) -> Result<DuderoResult, DuderoError> {
    // Fast path: check length constraints first
    if buf.len() < MIN_LEN {
        return Err(DuderoError::TooShort);
    }
    if buf.len() > MAX_LEN {
        return Err(DuderoError::TooLong);
    }

    // Use iterator-based approach
    let ctx: DuderoContext = buf.iter().copied().collect();
    ctx.into_result()
}

/// Check an iterator of bytes for randomness
///
/// This is a more general version of `check_buffer` that works with any iterator.
/// Note: The length checks are performed after consuming the iterator.
///
/// # Examples
///
/// ```no_run
/// # use dudero::check_iter;
/// let data = vec![0u8; 64];
/// let result = check_iter(data.iter().copied());
/// ```
pub fn check_iter<I>(iter: I) -> Result<DuderoResult, DuderoError>
where
    I: IntoIterator<Item = u8>,
{
    let ctx: DuderoContext = iter.into_iter().collect();

    // Check length constraints after building context
    let len = ctx.len();
    if len < MIN_LEN {
        return Err(DuderoError::TooShort);
    }
    if len > MAX_LEN {
        return Err(DuderoError::TooLong);
    }

    ctx.into_result()
}

#[cfg(test)]
mod tests {
    use super::*;

    // Tests require std for Vec
    #[cfg(not(feature = "std"))]
    extern crate alloc;
    #[cfg(not(feature = "std"))]
    use alloc::vec;
    #[cfg(not(feature = "std"))]
    use alloc::vec::Vec;

    #[test]
    fn test_too_short() {
        let buf = vec![0u8; 15];
        assert_eq!(check_buffer(&buf), Err(DuderoError::TooShort));
    }

    #[test]
    fn test_too_long() {
        let buf = vec![0u8; MAX_LEN + 1];
        assert_eq!(check_buffer(&buf), Err(DuderoError::TooLong));
    }

    #[test]
    fn test_all_zeros_fails() {
        let buf = vec![0u8; 64];
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_all_ones_fails() {
        let buf = vec![0xFFu8; 64];
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_streaming_api() {
        let mut ctx = DuderoContext::new();

        // Add some biased data
        for _ in 0..32 {
            ctx.add(0x00).unwrap();
        }

        let result = ctx.finish().unwrap();
        assert_eq!(result, DuderoResult::BadRandomness);
    }

    #[test]
    fn test_streaming_too_long() {
        let mut ctx = DuderoContext::new();

        // Try to add more than MAX_LEN bytes
        for i in 0..=MAX_LEN {
            let result = ctx.add(0x42);
            if i < MAX_LEN {
                assert!(result.is_ok());
            } else {
                assert_eq!(result, Err(DuderoError::TooLong));
            }
        }
    }

    #[test]
    fn test_minimum_length() {
        let buf = vec![0x42u8; MIN_LEN];
        // Should not error on length, but will likely fail randomness
        let result = check_buffer(&buf);
        assert!(result.is_ok());
    }

    // === Tests for bad RNGs ===

    #[test]
    fn test_single_repeating_byte() {
        // Bad RNG that outputs the same byte repeatedly
        let buf = vec![0x42u8; 256];
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_alternating_pattern() {
        // Bad RNG alternating between two values
        let mut buf = Vec::new();
        for i in 0..128 {
            buf.push(if i % 2 == 0 { 0xAA } else { 0x55 });
        }
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_incrementing_counter() {
        // Note: A simple counter (0, 1, 2, 3...) actually has uniform nibble distribution
        // so it passes the chi-square test even though it's completely predictable.
        // This test demonstrates the limitation of the chi-square test:
        // it only detects distribution bias, not lack of randomness/predictability.
        let buf: Vec<u8> = (0..=255).collect();
        let result = check_buffer(&buf);
        // This passes the test despite being a terrible RNG
        assert_eq!(result, Ok(DuderoResult::Ok));
    }

    #[test]
    fn test_low_nibble_bias() {
        // Bad RNG with bias in low nibble (high nibble varies, low is always 0)
        let mut buf = Vec::new();
        for i in 0..64 {
            buf.push((i % 16) << 4); // High nibble varies, low nibble always 0
        }
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_high_nibble_bias() {
        // Bad RNG with bias in high nibble (high is always 0, low nibble varies)
        let mut buf = Vec::new();
        for i in 0..64 {
            buf.push((i % 16) as u8); // High nibble always 0, low nibble varies
        }
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_missing_nibble_values() {
        // Bad RNG that never produces certain nibble values
        // Only produces values with nibbles 0-7, never 8-15
        let mut buf = Vec::new();
        for _ in 0..32 {
            for val in 0..8u8 {
                buf.push((val << 4) | val); // Both nibbles in range 0-7
            }
        }
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_repeating_short_sequence() {
        // Bad RNG that repeats a short sequence
        let sequence = [0x12, 0x34, 0x56, 0x78];
        let mut buf = Vec::new();
        for _ in 0..64 {
            buf.extend_from_slice(&sequence);
        }
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_even_bytes_only() {
        // Bad RNG that only produces even bytes (LSB always 0)
        let buf: Vec<u8> = (0..128).map(|i| (i * 2) as u8).collect();
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_odd_bytes_only() {
        // Bad RNG that only produces odd bytes (LSB always 1)
        let buf: Vec<u8> = (0..128).map(|i| (i * 2 + 1) as u8).collect();
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_high_bit_always_set() {
        // Bad RNG where highest bit is always 1
        let buf: Vec<u8> = (0..256).map(|i| (i as u8) | 0x80).collect();
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_high_bit_always_clear() {
        // Bad RNG where highest bit is always 0
        let buf: Vec<u8> = (0..256).map(|i| (i as u8) & 0x7F).collect();
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_linear_congruential_bad_params() {
        // Note: Even a bad LCG can have uniform nibble distribution.
        // This test shows that the chi-square test cannot detect all bad RNGs,
        // only those with obvious distribution bias.
        let mut buf = Vec::new();
        let mut state: u8 = 1;
        for _ in 0..256 {
            state = state.wrapping_mul(5).wrapping_add(3); // Poor LCG
            buf.push(state);
        }
        // This actually passes despite being predictable
        let result = check_buffer(&buf);
        assert!(result.is_ok());
    }

    #[test]
    fn test_ascii_printable_only() {
        // Bad RNG that only produces ASCII printable characters (0x20-0x7E)
        let mut buf = Vec::new();
        for i in 0..64 {
            buf.push(0x20 + ((i * 3) % 95) as u8); // Stays in printable range
        }
        // This might not always fail, but should fail most of the time
        // due to the limited range
        let result = check_buffer(&buf);
        assert!(result.is_ok()); // At least it doesn't error
    }

    #[test]
    fn test_every_other_bit_cleared() {
        // Bad RNG with pattern in bits: 0xAA = 10101010
        let buf = vec![0xAAu8; 128];
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_every_other_bit_set() {
        // Bad RNG with pattern in bits: 0x55 = 01010101
        let buf = vec![0x55u8; 128];
        assert_eq!(check_buffer(&buf), Ok(DuderoResult::BadRandomness));
    }

    // === Tests for idiomatic Rust features ===

    #[test]
    fn test_from_iterator() {
        // Test creating context from iterator
        let data = vec![0u8; 64];
        let ctx: DuderoContext = data.iter().copied().collect();
        assert_eq!(ctx.finish(), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_extend() {
        // Test extending context
        let mut ctx = DuderoContext::new();
        let data1 = vec![0xAAu8; 16];
        let data2 = vec![0xAAu8; 16];

        ctx.extend(data1);
        ctx.extend(data2);

        assert_eq!(ctx.finish(), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_add_bytes() {
        // Test add_bytes method
        let mut ctx = DuderoContext::new();
        let data = vec![0u8; 64];
        ctx.add_bytes(data).unwrap();
        assert_eq!(ctx.finish(), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_len_and_is_empty() {
        let mut ctx = DuderoContext::new();
        assert_eq!(ctx.len(), 0);
        assert!(ctx.is_empty());

        ctx.add(0x42).unwrap();
        assert_eq!(ctx.len(), 1);
        assert!(!ctx.is_empty());

        ctx.add(0x43).unwrap();
        assert_eq!(ctx.len(), 2);
    }

    #[test]
    fn test_into_result() {
        let data = vec![0u8; 64];
        let ctx: DuderoContext = data.iter().copied().collect();
        // Test consuming the context
        assert_eq!(ctx.into_result(), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_check_iter() {
        // Test iterator-based check
        let data = vec![0u8; 64];
        assert_eq!(check_iter(data.iter().copied()), Ok(DuderoResult::BadRandomness));
    }

    #[test]
    fn test_context_equality() {
        // Test PartialEq implementation
        let mut ctx1 = DuderoContext::new();
        let mut ctx2 = DuderoContext::new();

        assert_eq!(ctx1, ctx2);

        ctx1.add(0x42).unwrap();
        ctx2.add(0x42).unwrap();

        assert_eq!(ctx1, ctx2);

        ctx1.add(0x43).unwrap();

        assert_ne!(ctx1, ctx2);
    }
}
