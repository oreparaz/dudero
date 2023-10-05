# dudero: dude, is my randomness OK?

`dudero` is a small C-only library implementing a collection of heuristics to check if your random number generator looks totally broken, or not. `dudero` is loosely inspired in online randomness health checks a la AIS-31 (but does not conform to AIS-31). `dudero` targets embedded devices and runs in low-memory environments.

Purists, theoreticians and other species will complain this isn't a sound way of checking randomness, or that this endeavor is doomed to fail. The discerning practitioner will appreciate a sanity check that can catch bad bugs.

## TODO

- [ ] github actions: build / memory safety
- [ ] clang format
- [ ] doc on how to use
- [ ] doc on working principle
- [ ] doc readme on false positives
- [ ] test cases: bad RNG, tune
- [ ] test on differences


## References 

- AIS-31...

