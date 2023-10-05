#include "dudero.h"

#include <stdint.h>
#include <stdbool.h>

uint32_t hist[16] = {0};

static dudero_ret_t chi_sq(const uint8_t *buf, size_t len) {
    for (size_t i=0; i<16; i++) {
        hist[i] = 0;
    }
    for (size_t i=0; i<len; i++) {
        hist[buf[i] >> 4]++;
        hist[buf[i]&0x0F]++;
    }

    // TODO: handle rounding if len isn't multiple of 8
    int expected = 2*len/16; // 2* because we check nibbles
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

#define MIN_LEN (16)

dudero_ret_t dudero_check_buffer(const uint8_t *buf, size_t len) {
    if (len < MIN_LEN) {
        return DUDERO_RET_TOO_SHORT;
    }

    dudero_ret_t ret = chi_sq(buf, len);
    if (ret != DUDERO_RET_OK) {
        return ret;
    }

    return DUDERO_RET_OK;
}
