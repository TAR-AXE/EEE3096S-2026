/*
 * Task 1: The Golden Measure on the PC
 *
 * Integer square root of a 32-bit unsigned input x: the largest integer
 * whose square does not exceed x.  Golden version uses double precision
 * arithmetic and the standard library square root.
 *
 * Prediction (written before running):
 *   On a ~3 GHz x86-64 CPU, sqrt + floor + conversion is roughly
 *   20..40 instructions.  Estimate: 40 instructions * 0.33 ns/instr
 *   ~= 13 ns per call.  One call alone cannot be timed reliably because
 *   clock_gettime resolution is ~10..50 ns and scheduling noise is larger
 *   than the call itself, so we loop and divide.
 *
 * Build:   gcc -O2 -o golden_measure golden_measure.c -lm
 * Run:     ./golden_measure
 */

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

static const uint32_t inputs[10] = {
    0, 1, 15, 16, 4095, 65535,
    123456789, 987654321, 4294836225u, 4294967295u
};

static uint32_t golden_isqrt(uint32_t x)
{
    return (uint32_t)floor(sqrt((double)x));
}

static double timestamp_us(void)
{
#ifdef _WIN32
    static LARGE_INTEGER freq = {0};
    LARGE_INTEGER now;
    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (double)now.QuadPart * 1e6 / (double)freq.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e6 + (double)ts.tv_nsec / 1e3;
#endif
}

static int hand_check(uint32_t x, uint32_t r)
{
    uint64_t r2   = (uint64_t)r * r;
    uint64_t rp1_2 = (uint64_t)(r + 1) * (r + 1);
    return (r2 <= x) && (x < rp1_2);
}

/* Returns average time per single golden_isqrt call in microseconds */
static double time_n_calls(long reps)
{
    volatile uint32_t sink = 0;   /* prevent the compiler from removing the work */

    double t0 = timestamp_us();
    for (long i = 0; i < reps; i++) {
        for (int j = 0; j < 10; j++)
            sink += golden_isqrt(inputs[j]);
    }
    double t1 = timestamp_us();

    /* force the sink to be used */
    if (sink == 0xDEADBEEF) printf("never happens\n");

    return (t1 - t0) / (reps * 10.0);
}

int main(void)
{
    printf("=== Golden Measure: integer square-root ===\n\n");

    printf("Ten golden outputs:\n");
    for (int i = 0; i < 10; i++) {
        uint32_t x = inputs[i];
        uint32_t r = golden_isqrt(x);
        int ok = hand_check(x, r);
        printf("  inputs[%d] = %10" PRIu32 "  ->  isqrt = %10" PRIu32
               "   %s\n", i, x, r, ok ? "OK" : "FAIL");
    }

    printf("\nTiming (average per call):\n");

    long reps1 = 100000;
    double t1 = time_n_calls(reps1);
    printf("  %ld outer repetitions  ->  %.4f µs / call\n", reps1, t1);

    long reps2 = 1000000;
    double t2 = time_n_calls(reps2);
    printf("  %ld outer repetitions  ->  %.4f µs / call\n", reps2, t2);

    printf("\nDone.\n");
    return 0;
}