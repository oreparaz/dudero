#include "dudero.h"

#include <stdint.h>
#include <stdbool.h>

uint16_t hist[16] = {0}; // count up to 2^16 = 65 536
size_t hist_samples = 0;

#define MIN_LEN (16)

dudero_ret_t dudero_check_buffer(const uint8_t *buf, size_t len) {
    if (len < MIN_LEN) {
        return DUDERO_RET_TOO_SHORT;
    }

    dudero_stream_init();

    for (size_t i=0; i<len; i++) {
        dudero_stream_add(buf[i]);
    }

    return dudero_stream_finish();
}

dudero_ret_t dudero_stream_init(void) {
    for (size_t i=0; i<16; i++) {
        hist[i] = 0;
    }
    hist_samples = 0;
    return DUDERO_RET_OK;
}

dudero_ret_t dudero_stream_add(uint8_t sample) {
    hist[sample >> 4]++;
    hist[sample&0x0F]++;
    hist_samples += 2; // TODO: check this isn't larger than 2^16
    return DUDERO_RET_OK;
}

dudero_ret_t dudero_stream_finish(void) {
    if (hist_samples < 16) {
        return DUDERO_RET_TOO_SHORT;
    }
    // TODO: handle rounding if len isn't multiple of 8
    int expected = hist_samples / 16;
    uint32_t cum = 0;
    for (size_t i=0; i<16; i++) {
        uint32_t delta = (hist[i] > expected) ? hist[i]-expected : expected-hist[i];
        cum += delta*delta;
    }
    double cum_norm = (double)cum / (double)expected;

    // Chi-squared goodness-of-fit test with 15 degrees of freedom (16 bins - 1)
    //
    // cum_norm = Σ(Oi - E)² / E = χ² statistic
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
    // See auxiliary/verify_threshold.py for complete derivation and numerical verification.
    double thres = 50.0;

    if (cum_norm > thres) {
        return DUDERO_RET_BAD_RANDOMNESS;
    }

    return DUDERO_RET_OK;
}
