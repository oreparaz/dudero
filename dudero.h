#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    DUDERO_RET_OK = 0,
    DUDERO_RET_ERROR, // generic error
    DUDERO_RET_BAD_RANDOMNESS,
    DUDERO_RET_TOO_SHORT, // passed buffer is too short
    DUDERO_RET_TOO_LONG, // passed buffer is too long (would overflow)
    DUDERO_RET_KNOWN_BAD,
} dudero_ret_t;

// Checks if the passed buffer "looks random".  Fails if the passed
// buffer looks like "bad randomness" (obviously biased values, fixed values, etc).
//
// What to do when this test fails? There's a chance a perfect
// entropy source generates sequences that fail this test. This
// happens with chance ...
//
// WARNING: rejecting sequences that fail this test will reduce the source entropy!
//
dudero_ret_t dudero_check_buffer(const uint8_t *buf, size_t len);

// you need to use either the buffer OR the stream API,
// mixing them is bad
//
// nothing of this is thread safe
//
// TODO: this is screaming passing a ctx
dudero_ret_t dudero_stream_init(void);
dudero_ret_t dudero_stream_add(uint8_t sample);
dudero_ret_t dudero_stream_finish(void);
