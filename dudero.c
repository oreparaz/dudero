#include "dudero.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define MIN_LEN (16)
// Maximum safe length to prevent overflow of uint16_t histogram bins.
// Each byte produces 2 nibbles. With perfect uniform distribution,
// each bin gets len*2/16 = len/8 samples. To keep bins < 2^16:
// len/8 < 2^16 => len < 2^19 = 524,288 bytes.
// Use a conservative limit to handle non-uniform data.
#define MAX_LEN (32768)  // 32 KB

dudero_ret_t dudero_check_buffer(const uint8_t *buf, size_t len) {
    if (len < MIN_LEN) {
        return DUDERO_RET_TOO_SHORT;
    }
    if (len > MAX_LEN) {
        return DUDERO_RET_TOO_LONG;
    }

    dudero_ctx_t ctx;
    dudero_stream_init(&ctx);

    for (size_t i=0; i<len; i++) {
        dudero_stream_add(&ctx, buf[i]);
    }

    return dudero_stream_finish(&ctx);
}

void dudero_stream_init(dudero_ctx_t *ctx) {
    memset(ctx->hist, 0, sizeof(ctx->hist));
    ctx->hist_samples = 0;
}

dudero_ret_t dudero_stream_add(dudero_ctx_t *ctx, uint8_t sample) {
    // Check if adding this sample would exceed maximum safe samples
    // MAX_LEN bytes * 2 nibbles/byte = MAX_LEN * 2 samples
    if (ctx->hist_samples >= MAX_LEN * 2) {
        return DUDERO_RET_TOO_LONG;
    }
    ctx->hist[sample >> 4]++;
    ctx->hist[sample & 0x0F]++;
    ctx->hist_samples += 2;
    return DUDERO_RET_OK;
}

dudero_ret_t dudero_stream_finish(dudero_ctx_t *ctx) {
    if (ctx->hist_samples < 16) {
        return DUDERO_RET_TOO_SHORT;
    }
    // TODO: handle rounding if len isn't multiple of 8
    int expected = ctx->hist_samples / 16;
    uint32_t cum = 0;
    for (size_t i=0; i<16; i++) {
        uint32_t delta = (ctx->hist[i] > expected) ? ctx->hist[i]-expected : expected-ctx->hist[i];
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
