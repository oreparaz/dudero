#include "dudero.h"

#include <stdio.h>

#define CHECK(x, expected)                                                     \
  do {                                                                         \
    dudero_ret_t ret;                                                          \
    if ((ret = x) != expected) {                                               \
      printf("line %d error, expected %d got %d\n", __LINE__, expected, ret);  \
      if (expected == DUDERO_RET_BAD_RANDOMNESS) { printf("not sensitive enough (decrease threshold)\n"); } \
      if (expected == DUDERO_RET_OK) { printf("too sensitive (increase threshold)\n"); } \
      return (DUDERO_RET_ERROR);                                               \
    }                                                                          \
  } while (0)

dudero_ret_t test_known_bad(void) {
    {
        const uint8_t buf1[32] = {0};
        CHECK(dudero_check_buffer(buf1, sizeof buf1), DUDERO_RET_BAD_RANDOMNESS);
    }

    {
        const uint8_t buf2[32] = {1,2,3,4};
        CHECK(dudero_check_buffer(buf2, sizeof buf2), DUDERO_RET_BAD_RANDOMNESS);
    }

    {
        const uint8_t buf3[32] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};
        CHECK(dudero_check_buffer(buf3, sizeof buf3), DUDERO_RET_BAD_RANDOMNESS);
    }

    {
        const uint8_t buf4[32] = {10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};
        CHECK(dudero_check_buffer(buf4, sizeof buf4), DUDERO_RET_BAD_RANDOMNESS);
    }
    return DUDERO_RET_OK;
}

#include "randombytes/randombytes.h"
void fill_random(uint8_t *buf, size_t len) {
    (void)randombytes(buf, len); // yolo
}

dudero_ret_t test_good(void) {
    for (int i=0; i<100; i++) {
       uint8_t buf[16] = {0};
       fill_random(buf, sizeof(buf));
       CHECK(dudero_check_buffer(buf, sizeof buf), DUDERO_RET_OK);
    }
    return DUDERO_RET_OK;
}

// check biased RNG (bad driver)
dudero_ret_t test_badbit(void) {
    int fails = 0;
    for (int i=0; i<100; i++) {
       uint32_t buf[8] = {0};
       fill_random((uint8_t *)buf, 32);
       for (int i=0; i<8; i++) {
        buf[i] &= 0x7FFF7F00;
       }
       if (dudero_check_buffer((uint8_t *)buf, 32) != DUDERO_RET_BAD_RANDOMNESS) {
        fails++;
       }
    }
    if (fails < 20) {
        return DUDERO_RET_OK; 
    }
    return DUDERO_RET_ERROR;
}


void test_printstat_stream(void) {
    #define HOWMANY (100000)
    #define buffer_len (512)
    int failures = 0;
    for (int i=0; i< HOWMANY; i++) {
       uint8_t buf[buffer_len] = {0};
       fill_random((uint8_t *)buf, buffer_len);
       for (int i=0; i<(buffer_len); i++) {
        if ((i%2)!=0) continue;
         buf[i] &= 0xEF;
       }

       dudero_stream_init();
       for (int i=0; i<buffer_len; i++) {
        dudero_stream_add(buf[i]);
       }
       if (dudero_stream_finish() != DUDERO_RET_BAD_RANDOMNESS) {
        failures++;
       }
    }
    printf("failed (not sensitive enough): %d / %d, missed %2.2f%%\n", failures, HOWMANY, (double)(100*failures) / (double)HOWMANY);


    int false_positive = 0;
    for (int i=0; i<HOWMANY; i++) {
       uint8_t buf[buffer_len] = {0};
       fill_random(buf, sizeof(buf));

       dudero_stream_init();
       for (int i=0; i<buffer_len; i++) {
        dudero_stream_add(buf[i]);
       }

       if (dudero_stream_finish() != DUDERO_RET_OK) {
        false_positive++;
       }
    }
    printf("failed (too sensitive): %d / %d (1 in %d, %2.2f)\n", false_positive, HOWMANY, HOWMANY/false_positive, (double)false_positive*100 / (double)HOWMANY);
}

int main(int argc, char **argv) {
    {
        dudero_ret_t ret = test_known_bad();
        if (ret != DUDERO_RET_OK) { printf("fail\n"); return -1; }
    }
    {
        dudero_ret_t ret = test_good();
        if (ret != DUDERO_RET_OK) { printf("fail\n"); return -1; }
    }
    {
        dudero_ret_t ret = test_badbit();
        if (ret != DUDERO_RET_OK) { printf("fail\n"); return -1; }
    }
    test_printstat_stream(); // TODO: this test should be able to fail and return -1
    printf("pass\n");
    return 0;
}
