# x86-to-C Interface Programming Project — Dot Product Kernel (Spec 1, Single-Precision)

This project implements a dot product kernel

```
sdot = A[0]*B[0] + A[1]*B[1] + ... + A[n-1]*B[n-1]
```

in two versions:

1. **C** (`sdot_c` in `Source.c`) — serves as the sanity-check answer key
2. **x86-64 assembly** (`sdot_asm` in `Source.asm`, NASM) — uses **scalar SIMD registers** (XMM) and **scalar SIMD floating-point instructions** (`xorps`, `movss`, `mulss`, `addss`)

Vectors A and B and the result `sdot` are all **single-precision floats**. The C `main` initializes both vectors with random values, calls both kernels, times **only the kernel portion** using `QueryPerformanceCounter`, and reports the **average of 20 runs** per kernel for each vector size n = 2^20, 2^24, and 2^30. The x86-64 output is checked for correctness against the C output.

- **Machine used:** 32 GB DDR4 RAM (2^30 runs at full size, no fallback needed)
- **Build:** Visual Studio, x64, tested in both **Debug** and **Release** configurations
- **Video (source code, compilation, execution):** [WATCH HERE](PASTE_VIDEO_LINK_HERE)

---

## i.) Comparative Execution Time and Analysis

### Average kernel execution time (20 runs each)

| Vector size | Debug — C | Debug — x86-64 | Release — C | Release — x86-64 |
|---|---|---|---|---|
| 2^20 (1,048,576) | 3.302285 ms | 0.812860 ms | 0.809880 ms | 0.764400 ms |
| 2^24 (16,777,216) | 47.851320 ms | 12.380535 ms | 12.725035 ms | 12.385220 ms |
| 2^30 (1,073,741,824) | 3126.473470 ms | 786.214895 ms | 778.342880 ms | 774.772925 ms |

### Speedup (C time ÷ x86-64 time)

| Vector size | Debug | Release |
|---|---|---|
| 2^20 | 4.063x | 1.059x |
| 2^24 | 3.865x | 1.027x |
| 2^30 | 3.977x | 1.005x |

### Short Analysis

**1. The assembly kernel is ~4x faster than C in Debug mode, but the gap almost disappears in Release mode.**
In Debug mode, MSVC compiles the C kernel with no optimization: the accumulator lives in memory, the array index is recomputed every iteration, and there is extra overhead on every loop pass. The hand-written assembly has none of that, so it wins by roughly 4x across all sizes. In Release mode (/O2), the compiler keeps the accumulator in an XMM register and emits a tight scalar SSE loop — essentially the same instructions written by hand in `Source.asm`. As a result, the C kernel's time drops by ~4x (e.g., 3126 ms → 778 ms at 2^30) while the assembly kernel's time barely changes (786 ms → 775 ms), since NASM assembles the exact same instructions regardless of build configuration. The remaining speedup in Release is only 1.005x–1.059x. Notably, Release C at 2^30 (778 ms) is nearly identical to Debug assembly (786 ms), which is direct evidence that the optimizer converged on the same code we wrote manually.

**2. The speedup shrinks as n grows because the kernel becomes memory-bound.**
At 2^20, both vectors (8 MB total) partially fit in cache, so instruction efficiency still matters and the Release speedup is 1.059x. At 2^30, the kernel must stream 8 GB from main memory, and DRAM bandwidth — not the CPU — becomes the bottleneck. Both kernels wait on memory equally, so their times converge (1.005x). This is why hand-optimizing instructions gives diminishing returns for large, streaming workloads.

**3. The sdot result at 2^30 exposes single-precision accumulation saturation.**
At n = 2^30, both kernels output sdot = 16,777,216.000000 — exactly 2^24. The mathematically expected value (~2.7×10^8 for random values in [0,1]) is far larger. What happens: once the running sum reaches 2^24, the gap between consecutive representable single-precision floats becomes 2.0, so adding any product smaller than 1.0 rounds back to the same value. Every addition after that point is lost, and the sum is stuck at exactly 2^24. Both kernels accumulate in the same order with the same precision, so they hit the same ceiling and still agree bit-for-bit (|diff| = 0) — the correctness check passes as required, while demonstrating a real limitation of naive single-precision accumulation. (Production BLAS libraries avoid this using pairwise summation or wider accumulators.)

---

## ii.) Program Output with Correctness Check — C version

The program prints the C kernel's sdot alongside the x86-64 kernel's sdot, and validates the x86-64 output against the C "answer key." All sizes PASS with |diff| = 0.

**Debug mode:**

<img width="1483" height="762" alt="Debug_CMD" src="https://github.com/user-attachments/assets/4b989000-c72e-4ae0-9692-8e1951103879" />

**Release mode:**

<img width="1483" height="762" alt="Release_CMD" src="https://github.com/user-attachments/assets/7ac1c737-01a6-4705-b7ce-1bbe6ed6ae5d" />

## iii.) Program Output with Correctness Check — x86-64 version

**Debug mode:**

<img width="2559" height="1439" alt="Debug" src="https://github.com/user-attachments/assets/dc2773ae-8d1b-44de-bc41-34d8dbfc2738" />

**Release mode:**

<img width="2555" height="1439" alt="Release" src="https://github.com/user-attachments/assets/fb27d754-e9fb-462a-ac00-7c3416aab204" />


## iv.) Short Video

The video (5–10 mins) shows the source code walkthrough, compilation, and execution of both the C and x86-64 versions:

**[PASTE_VIDEO_LINK_HERE](PASTE_VIDEO_LINK_HERE)**

---

## Project Structure

```
├── Test.sln                  # Visual Studio solution
├── Test/
│   ├── Test.vcxproj          # project file (x64, Debug & Release)
│   ├── Source.c              # C main + C kernel (sdot_c) + timing + correctness check
│   └── Source.asm            # x86-64 NASM kernel (sdot_asm)
└── README.md
```

## How to Build and Run

1. Open `Test.sln` in Visual Studio.
2. Select **x64** and either **Debug** or **Release**.
3. `Source.asm` builds via a Custom Build Tool step using NASM (`nasm -f win64`), configured for both configurations.
4. Build the solution, then run with the Local Windows Debugger (or run `Test.exe` directly).
5. The program runs all three vector sizes automatically. Note: the 2^30 phase allocates ~8 GB of RAM and takes a few minutes in Debug mode; if allocation fails on your machine, the program automatically falls back to 2^29 / 2^28 as permitted by the specs.
