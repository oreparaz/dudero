#include "dudero.h"

#include <stdio.h>
#include <string.h>

#include "randombytes/randombytes.h"

static int tests_run = 0;
static int tests_passed = 0;

#define CHECK(x, expected)                                                     \
  do {                                                                         \
    dudero_ret_t ret;                                                          \
    tests_run++;                                                               \
    if ((ret = x) != expected) {                                               \
      printf("FAIL line %d: expected %d got %d\n", __LINE__, expected, ret);   \
      return DUDERO_RET_ERROR;                                                 \
    }                                                                          \
    tests_passed++;                                                            \
  } while (0)

static void fill_random(uint8_t *buf, size_t len) {
    (void)randombytes(buf, len);
}

// Helper: fill buffer with a repeating byte
static void fill_byte(uint8_t *buf, size_t len, uint8_t val) {
    memset(buf, val, len);
}

// === Error path tests ===

dudero_ret_t test_too_short(void) {
    uint8_t buf[15];
    fill_random(buf, sizeof buf);
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_TOO_SHORT);
    return DUDERO_RET_OK;
}

dudero_ret_t test_too_long(void) {
    // Just check the return code with a length > 32KB; no need to allocate
    static uint8_t buf[32769];
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_TOO_LONG);
    return DUDERO_RET_OK;
}

dudero_ret_t test_minimum_length(void) {
    uint8_t buf[16];
    fill_byte(buf, sizeof buf, 0x42);
    // Should not error on length (will fail randomness, but that's fine)
    dudero_ret_t ret = dudero_check_buffer(buf, sizeof buf);
    tests_run++;
    if (ret == DUDERO_RET_TOO_SHORT || ret == DUDERO_RET_TOO_LONG || ret == DUDERO_RET_ERROR) {
        printf("FAIL line %d: unexpected error %d for minimum length buffer\n", __LINE__, ret);
        return DUDERO_RET_ERROR;
    }
    tests_passed++;
    return DUDERO_RET_OK;
}

// === Known bad patterns (buffer API) ===

dudero_ret_t test_all_zeros(void) {
    uint8_t buf[64];
    fill_byte(buf, sizeof buf, 0x00);
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_all_ones(void) {
    uint8_t buf[64];
    fill_byte(buf, sizeof buf, 0xFF);
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_single_repeating_byte(void) {
    uint8_t buf[256];
    fill_byte(buf, sizeof buf, 0x42);
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_alternating_pattern(void) {
    uint8_t buf[128];
    for (size_t i = 0; i < sizeof buf; i++) {
        buf[i] = (i % 2 == 0) ? 0xAA : 0x55;
    }
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_low_nibble_bias(void) {
    // High nibble varies, low nibble always 0
    uint8_t buf[64];
    for (size_t i = 0; i < sizeof buf; i++) {
        buf[i] = (uint8_t)((i % 16) << 4);
    }
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_high_nibble_bias(void) {
    // High nibble always 0, low nibble varies
    uint8_t buf[64];
    for (size_t i = 0; i < sizeof buf; i++) {
        buf[i] = (uint8_t)(i % 16);
    }
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_missing_nibble_values(void) {
    // Only nibbles 0-7, never 8-15
    uint8_t buf[256];
    for (size_t i = 0; i < sizeof buf; i++) {
        uint8_t val = (uint8_t)(i % 8);
        buf[i] = (uint8_t)((val << 4) | val);
    }
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_repeating_short_sequence(void) {
    const uint8_t seq[] = {0x12, 0x34, 0x56, 0x78};
    uint8_t buf[256];
    for (size_t i = 0; i < sizeof buf; i++) {
        buf[i] = seq[i % sizeof seq];
    }
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_high_bit_always_set(void) {
    uint8_t buf[256];
    for (size_t i = 0; i < sizeof buf; i++) {
        buf[i] = (uint8_t)(i | 0x80);
    }
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_high_bit_always_clear(void) {
    uint8_t buf[256];
    for (size_t i = 0; i < sizeof buf; i++) {
        buf[i] = (uint8_t)(i & 0x7F);
    }
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_every_other_bit_cleared(void) {
    uint8_t buf[128];
    fill_byte(buf, sizeof buf, 0xAA);
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_every_other_bit_set(void) {
    uint8_t buf[128];
    fill_byte(buf, sizeof buf, 0x55);
    CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

// === Good random data ===

dudero_ret_t test_good_random(void) {
    for (int i = 0; i < 100; i++) {
        uint8_t buf[64];
        fill_random(buf, sizeof buf);
        CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_OK);
    }
    return DUDERO_RET_OK;
}

// === Biased RNG detection ===

dudero_ret_t test_badbit(void) {
    int fails = 0;
    for (int i = 0; i < 100; i++) {
        uint32_t buf[8] = {0};
        fill_random((uint8_t *)buf, 32);
        for (int j = 0; j < 8; j++) {
            buf[j] &= 0x7FFF7F00;
        }
        if (dudero_check_buffer((uint8_t *)buf, 32) != DUDERO_RET_BAD_RANDOMNESS) {
            fails++;
        }
    }
    tests_run++;
    if (fails >= 20) {
        printf("FAIL line %d: biased RNG not detected %d/100 times\n", __LINE__, fails);
        return DUDERO_RET_ERROR;
    }
    tests_passed++;
    return DUDERO_RET_OK;
}

// === Streaming API tests ===

dudero_ret_t test_stream_basic(void) {
    dudero_ctx_t ctx;
    dudero_stream_init(&ctx);
    for (int i = 0; i < 32; i++) {
        CHECK(dudero_stream_add(&ctx, 0x00), DUDERO_RET_OK);
    }
    CHECK(dudero_stream_finish(&ctx), DUDERO_RET_BAD_RANDOMNESS);
    return DUDERO_RET_OK;
}

dudero_ret_t test_stream_too_short(void) {
    dudero_ctx_t ctx;
    dudero_stream_init(&ctx);
    // Add fewer than 8 bytes (< 16 nibbles)
    for (int i = 0; i < 7; i++) {
        dudero_stream_add(&ctx, 0x42);
    }
    CHECK(dudero_stream_finish(&ctx), DUDERO_RET_TOO_SHORT);
    return DUDERO_RET_OK;
}

dudero_ret_t test_stream_too_long(void) {
    dudero_ctx_t ctx;
    dudero_stream_init(&ctx);
    // Add exactly MAX_LEN bytes (should succeed)
    for (int i = 0; i < 32768; i++) {
        dudero_ret_t ret = dudero_stream_add(&ctx, 0x42);
        if (ret != DUDERO_RET_OK) {
            tests_run++;
            printf("FAIL line %d: stream_add failed at byte %d\n", __LINE__, i);
            return DUDERO_RET_ERROR;
        }
    }
    // One more should fail
    CHECK(dudero_stream_add(&ctx, 0x42), DUDERO_RET_TOO_LONG);
    return DUDERO_RET_OK;
}

dudero_ret_t test_stream_matches_buffer(void) {
    // Streaming and buffer API should give the same result
    uint8_t buf[128];
    fill_random(buf, sizeof buf);

    dudero_ret_t buf_ret = dudero_check_buffer(buf, sizeof buf);

    dudero_ctx_t ctx;
    dudero_stream_init(&ctx);
    for (size_t i = 0; i < sizeof buf; i++) {
        dudero_stream_add(&ctx, buf[i]);
    }
    dudero_ret_t stream_ret = dudero_stream_finish(&ctx);

    tests_run++;
    if (buf_ret != stream_ret) {
        printf("FAIL line %d: buffer returned %d but stream returned %d\n", __LINE__, buf_ret, stream_ret);
        return DUDERO_RET_ERROR;
    }
    tests_passed++;
    return DUDERO_RET_OK;
}

// === Statistical test ===

dudero_ret_t test_statistical(void) {
    #define STAT_ITERATIONS 100000
    #define STAT_BUF_LEN 512

    // Part 1: biased data should be detected most of the time
    int missed = 0;
    for (int i = 0; i < STAT_ITERATIONS; i++) {
        uint8_t buf[STAT_BUF_LEN];
        fill_random(buf, sizeof buf);
        // Introduce bias: clear one bit on every other byte
        for (int j = 0; j < STAT_BUF_LEN; j += 2) {
            buf[j] &= 0xEF;
        }

        dudero_ctx_t ctx;
        dudero_stream_init(&ctx);
        for (int j = 0; j < STAT_BUF_LEN; j++) {
            dudero_stream_add(&ctx, buf[j]);
        }
        if (dudero_stream_finish(&ctx) != DUDERO_RET_BAD_RANDOMNESS) {
            missed++;
        }
    }
    double miss_rate = (double)missed * 100.0 / (double)STAT_ITERATIONS;
    printf("  detection: missed %d/%d (%.2f%%)\n", missed, STAT_ITERATIONS, miss_rate);

    tests_run++;
    // Should detect at least 90% of biased samples
    if (missed > STAT_ITERATIONS / 10) {
        printf("FAIL line %d: missed too many biased samples (%d/%d)\n", __LINE__, missed, STAT_ITERATIONS);
        return DUDERO_RET_ERROR;
    }
    tests_passed++;

    // Part 2: truly random data should rarely be flagged
    int false_positives = 0;
    for (int i = 0; i < STAT_ITERATIONS; i++) {
        uint8_t buf[STAT_BUF_LEN];
        fill_random(buf, sizeof buf);

        dudero_ctx_t ctx;
        dudero_stream_init(&ctx);
        for (int j = 0; j < STAT_BUF_LEN; j++) {
            dudero_stream_add(&ctx, buf[j]);
        }
        if (dudero_stream_finish(&ctx) != DUDERO_RET_OK) {
            false_positives++;
        }
    }
    double fp_rate = (double)false_positives * 100.0 / (double)STAT_ITERATIONS;
    if (false_positives > 0) {
        printf("  false positives: %d/%d (1 in %d, %.2f%%)\n",
            false_positives, STAT_ITERATIONS, STAT_ITERATIONS / false_positives, fp_rate);
    } else {
        printf("  false positives: 0/%d\n", STAT_ITERATIONS);
    }

    tests_run++;
    // FPR should be well under 1% (theoretical is ~0.0012%)
    if (false_positives > STAT_ITERATIONS / 100) {
        printf("FAIL line %d: too many false positives (%d/%d)\n", __LINE__, false_positives, STAT_ITERATIONS);
        return DUDERO_RET_ERROR;
    }
    tests_passed++;

    return DUDERO_RET_OK;
}

// === Test runner ===

typedef dudero_ret_t (*test_fn)(void);

typedef struct {
    const char *name;
    test_fn fn;
} test_entry;

static const test_entry all_tests[] = {
    // Error paths
    {"too_short",               test_too_short},
    {"too_long",                test_too_long},
    {"minimum_length",          test_minimum_length},
    // Bad patterns
    {"all_zeros",               test_all_zeros},
    {"all_ones",                test_all_ones},
    {"single_repeating_byte",   test_single_repeating_byte},
    {"alternating_pattern",     test_alternating_pattern},
    {"low_nibble_bias",         test_low_nibble_bias},
    {"high_nibble_bias",        test_high_nibble_bias},
    {"missing_nibble_values",   test_missing_nibble_values},
    {"repeating_short_sequence",test_repeating_short_sequence},
    {"high_bit_always_set",     test_high_bit_always_set},
    {"high_bit_always_clear",   test_high_bit_always_clear},
    {"every_other_bit_cleared", test_every_other_bit_cleared},
    {"every_other_bit_set",     test_every_other_bit_set},
    // Good data
    {"good_random",             test_good_random},
    {"badbit",                  test_badbit},
    // Streaming
    {"stream_basic",            test_stream_basic},
    {"stream_too_short",        test_stream_too_short},
    {"stream_too_long",         test_stream_too_long},
    {"stream_matches_buffer",   test_stream_matches_buffer},
    // Statistical
    {"statistical",             test_statistical},
};

int main(void) {
    int failed = 0;
    int num_tests = (int)(sizeof(all_tests) / sizeof(all_tests[0]));

    for (int i = 0; i < num_tests; i++) {
        printf("%-28s", all_tests[i].name);
        dudero_ret_t ret = all_tests[i].fn();
        if (ret != DUDERO_RET_OK) {
            printf(" FAIL\n");
            failed++;
        } else {
            printf(" ok\n");
        }
    }

    printf("\n%d/%d checks passed", tests_passed, tests_run);
    if (failed > 0) {
        printf(", %d test(s) FAILED\n", failed);
        return 1;
    }
    printf(", all tests passed\n");
    return 0;
}
