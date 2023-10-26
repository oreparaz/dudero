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
    // TODO: handle rounding if len isn't multiple of 8
    int expected = hist_samples / 16;
    uint32_t cum = 0;
    for (size_t i=0; i<16; i++) {
        uint32_t delta = (hist[i] > expected) ? hist[i]-expected : expected-hist[i];
        cum += delta*delta;
    }
    double cum_norm = (double)cum / (double)expected;
    double thres = 45.0;
  
    if (cum_norm > thres) {
        return DUDERO_RET_BAD_RANDOMNESS;
    }

    return DUDERO_RET_OK;
}
