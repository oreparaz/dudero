# dudero: dude, is my randomness OK?

`dudero` is a small library (C and Rust) implementing a chi-square goodness-of-fit test to check if your random number generator looks totally broken, or not. `dudero` is loosely inspired by online randomness health checks a la AIS-31 (but does not conform to AIS-31). `dudero` targets embedded devices and runs in low-memory environments with no allocations.

Purists, theoreticians and other species will complain this isn't a sound way of checking randomness, or that this endeavor is doomed to fail. The discerning practitioner will appreciate a sanity check that can catch bad bugs.

## Usage (C)

Copy `dudero.c` and `dudero.h` into your project.

**One-shot check:**

```c
#include "dudero.h"

uint8_t buf[64];
// ... fill buf from your RNG ...
dudero_ret_t ret = dudero_check_buffer(buf, sizeof(buf));
if (ret == DUDERO_RET_BAD_RANDOMNESS) {
    // RNG looks broken
}
```

**Streaming API** (for incremental processing):

```c
dudero_ctx_t ctx;
dudero_stream_init(&ctx);
for (int i = 0; i < len; i++) {
    dudero_stream_add(&ctx, buf[i]);
}
dudero_ret_t ret = dudero_stream_finish(&ctx);
```

Buffer must be between 16 bytes and 32 KB.

## Usage (Rust)

Add to your `Cargo.toml`:

```toml
[dependencies]
dudero = { git = "https://github.com/oreparaz/dudero", subdirectory = "rust" }
```

```rust
use dudero::{check_buffer, DuderoResult};

let data: Vec<u8> = get_random_bytes();
match check_buffer(&data) {
    Ok(DuderoResult::Ok) => println!("Looks random"),
    Ok(DuderoResult::BadRandomness) => println!("Bad randomness detected"),
    Err(e) => println!("Error: {:?}", e),
}
```

Supports `no_std` by disabling default features. See `rust/src/lib.rs` for the streaming API and full documentation.

## How this works

`dudero` performs a chi-square goodness-of-fit test on nibbles (4-bit values). It counts the frequency of each of the 16 possible nibble values and compares the observed distribution to the expected uniform distribution. This is equivalent to the "Poker test" (Test 2 in AIS-31). The test uses a threshold of 50.0 on the chi-square statistic with 15 degrees of freedom, corresponding to a false positive rate of approximately 1 in 83,000.

**Entropy loss:** Conditioning on passing this test causes negligible entropy loss. A passing string comes from approximately (1 - 1/83,000) of the input space, giving an entropy loss of -log₂(1 - 1/83,000) ≈ 0.0000173 bits. For practical purposes, the entropy loss is zero—passing strings remain essentially uniformly distributed over nearly the entire input space.

See [THRESHOLD_CHOICE.md](THRESHOLD_CHOICE.md) for detailed analysis of the threshold selection and its trade-offs.

## References

- [AIS-31: A proposal for: Functionality classes for random number generators](https://www.bsi.bund.de/SharedDocs/Downloads/DE/BSI/Zertifizierung/Interpretationen/AIS_31_Functionality_classes_for_random_number_generators_e.pdf)
