# dudero: dude, is my randomness OK?

`dudero` is a small C-only library implementing a collection of heuristics to check if your random number generator looks totally broken, or not. `dudero` is loosely inspired in online randomness health checks a la AIS-31 (but does not conform to AIS-31). `dudero` targets embedded devices and runs in low-memory environments.

Purists, theoreticians and other species will complain this isn't a sound way of checking randomness, or that this endeavor is doomed to fail. The discerning practitioner will appreciate a sanity check that can catch bad bugs.

## How this works

`dudero` performs a chi-square goodness-of-fit test on nibbles (4-bit values). It counts the frequency of each of the 16 possible nibble values and compares the observed distribution to the expected uniform distribution. This is equivalent to the "Poker test" (Test 2 in AIS-31). The test uses a threshold of 50.0 on the chi-square statistic with 15 degrees of freedom, corresponding to a false positive rate of approximately 1 in 83,000.

**Entropy loss:** Conditioning on passing this test causes negligible entropy loss. A passing string comes from approximately (1 - 1/83,000) of the input space, giving an entropy loss of -log₂(1 - 1/83,000) ≈ 0.0000173 bits. For practical purposes, the entropy loss is zero—passing strings remain essentially uniformly distributed over nearly the entire input space.

## TODO

- [X] github actions: build
- [ ] github actions: memory safety
- [ ] clang format
- [ ] doc on how to use
- [ ] doc on working principle
- [ ] doc readme on false positives
- [ ] test cases: bad RNG, tune
- [ ] test on differences


## References 

- AIS-31...

