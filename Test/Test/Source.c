// x86-to-C interface project (spec 1, single precision)
// dot product: sdot = A[0]*B[0] + A[1]*B[1] + ... + A[n-1]*B[n-1]
// runs both the C kernel and the asm kernel, times them, then
// checks if the asm answer matches the C one

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>    // for QueryPerformanceCounter timing

#define NUM_RUNS 20     // spec says at least 20 runs for the average

// the asm kernel from Source.asm
extern float sdot_asm(int n, const float* A, const float* B);

// C version of the kernel, this is the "answer key"
float sdot_c(int n, const float* A, const float* B) {
    float sdot = 0.0f;
    int i;
    for (i = 0; i < n; i++) {
        sdot += A[i] * B[i];
    }
    return sdot;
}

// runs a kernel NUM_RUNS times and gives back the average time in ms
// only the kernel call itself is inside the timer, nothing else
double time_kernel(float (*kernel)(int, const float*, const float*),
    int n, const float* A, const float* B, float* result) {
    LARGE_INTEGER freq, start, end;
    double total_ms = 0.0;
    float sdot = 0.0f;
    int run;

    QueryPerformanceFrequency(&freq);   // ticks per second, needed for the math

    for (run = 0; run < NUM_RUNS; run++) {
        QueryPerformanceCounter(&start);
        sdot = kernel(n, A, B);         // <-- only this part gets timed
        QueryPerformanceCounter(&end);

        // ticks -> milliseconds
        total_ms += (double)(end.QuadPart - start.QuadPart) * 1000.0
            / (double)freq.QuadPart;
    }

    *result = sdot;                     // hand back the last result too
    return total_ms / NUM_RUNS;
}

int main(void) {
    // vector sizes from the spec: 2^20, 2^24, 2^30
    int exponents[] = { 20, 24, 30 };
    int num_sizes = sizeof(exponents) / sizeof(exponents[0]);
    int s;

    printf("============================================================\n");
    printf(" Dot Product Kernel: C vs x86-64 (scalar SIMD, single float)\n");
    printf(" Average of %d runs per kernel\n", NUM_RUNS);
    printf("============================================================\n\n");

    srand(12345);   // fixed seed so runs are reproducible

    for (s = 0; s < num_sizes; s++) {
        int e = exponents[s];
        size_t n = 0;
        float* A = NULL, * B = NULL;
        float result_c = 0.0f, result_asm = 0.0f;
        double avg_c, avg_asm;
        size_t i;

        // try to allocate, if 2^30 won't fit drop down to 2^29, 2^28, etc
        // (spec allows this if the machine can't handle 2^30)
        while (e >= 20) {
            n = (size_t)1 << e;
            A = (float*)malloc(n * sizeof(float));
            B = (float*)malloc(n * sizeof(float));
            if (A != NULL && B != NULL) break;
            free(A); free(B);
            A = B = NULL;
            printf("[!] Allocation for n = 2^%d failed, reducing to 2^%d...\n",
                e, e - 1);
            e--;
        }
        if (A == NULL || B == NULL) {
            printf("[!] Could not allocate memory. Skipping.\n");
            continue;
        }

        // fill both vectors with random values between 0 and 1
        for (i = 0; i < n; i++) {
            A[i] = (float)rand() / (float)RAND_MAX;
            B[i] = (float)rand() / (float)RAND_MAX;
        }

        printf("Vector size n = 2^%d (%zu elements)\n", e, n);
        printf("------------------------------------------------------------\n");

        // time both kernels on the exact same data
        avg_c = time_kernel(sdot_c, (int)n, A, B, &result_c);
        avg_asm = time_kernel(sdot_asm, (int)n, A, B, &result_asm);

        printf("  C      kernel : avg time = %12.6f ms | sdot = %f\n",
            avg_c, result_c);
        printf("  x86-64 kernel : avg time = %12.6f ms | sdot = %f\n",
            avg_asm, result_asm);

        // correctness check: asm output vs the C answer key
        // using a small tolerance instead of == because of float rounding
        {
            float diff = fabsf(result_asm - result_c);
            float tol = 1e-5f * fabsf(result_c) + 1e-5f;
            if (diff <= tol)
                printf("  Correctness   : PASS (x86-64 output matches C answer key, |diff| = %g)\n", diff);
            else
                printf("  Correctness   : FAIL (|diff| = %g exceeds tolerance %g)\n", diff, tol);
        }

        printf("  Speedup (C / x86-64) : %.3fx\n\n", avg_c / avg_asm);

        free(A);
        free(B);
    }

    printf("Done.\n");
    return 0;
}