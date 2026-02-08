// Example usage of dudero
//
// Run with: cargo run --example example

use dudero::{check_buffer, DuderoContext, DuderoResult};

fn main() {
    println!("=== Dudero Rust Example ===\n");

    // Example 1: Check a buffer with all zeros (should fail)
    println!("Example 1: Buffer with all zeros");
    let bad_data = vec![0u8; 64];
    match check_buffer(&bad_data) {
        Ok(DuderoResult::Ok) => println!("  Result: Looks random"),
        Ok(DuderoResult::BadRandomness) => println!("  Result: Bad randomness detected ✓"),
        Err(e) => println!("  Error: {}", e),
    }

    // Example 2: Check a buffer that's too short
    println!("\nExample 2: Buffer too short (15 bytes)");
    let short_data = vec![0x42u8; 15];
    match check_buffer(&short_data) {
        Ok(_) => println!("  Result: Passed"),
        Err(e) => println!("  Error: {} ✓", e),
    }

    // Example 3: Streaming API
    println!("\nExample 3: Streaming API with biased data");
    let mut ctx = DuderoContext::new();
    for _ in 0..32 {
        ctx.add(0xAA).unwrap();
    }
    match ctx.finish() {
        Ok(DuderoResult::Ok) => println!("  Result: Looks random"),
        Ok(DuderoResult::BadRandomness) => println!("  Result: Bad randomness detected ✓"),
        Err(e) => println!("  Error: {}", e),
    }

    // Example 4: Simple incrementing pattern (should fail)
    println!("\nExample 4: Incrementing pattern");
    let pattern: Vec<u8> = (0..64).map(|i| i as u8).collect();
    match check_buffer(&pattern) {
        Ok(DuderoResult::Ok) => println!("  Result: Looks random"),
        Ok(DuderoResult::BadRandomness) => println!("  Result: Bad randomness detected ✓"),
        Err(e) => println!("  Error: {}", e),
    }

    println!("\n=== Done ===");
}
