# x86-to-C Interface Programming Project: Dot Product Kernel (Spec 1, Single-Precision)

**Prepared by:**
- Suerte, Lorenzo Enrique Evangelista
- Obcena, Hans Gabriel

This project implements a dot product kernel

```
sdot = A[0]*B[0] + A[1]*B[1] + ... + A[n-1]*B[n-1]
```

in two versions:

1. **C** (`sdot_c` in `Source.c`), which serves as the answer key for the correctness check
2. **x86-64 assembly** (`sdot_asm` in `Source.asm`, written in NASM), which uses the required **scalar SIMD registers** (XMM) and **scalar SIMD floating-point instructions** (`xorps`, `movss`, `mulss`, `addss`)

Since we were given Spec 1, vectors A and B and the result `sdot` are all **single-precision floats**. Our C main program fills both vectors with random values, calls both kernels, and times **only the kernel portion** using `QueryPerformanceCounter`. Each kernel is run 20 times to get the average execution time, for vector sizes n = 2^20, 2^24, and 2^30. The x86-64 output is then checked for correctness against the C output.

- **Machine used:** 32 GB DDR4 RAM (2^30 ran at full size, no fallback needed)
- **Build:** Visual Studio, x64, tested in both **Debug** and **Release** configurations
- **Video (source code, compilation, execution):** [WATCH HERE](https://youtu.be/UAW1UcU2r70)

---

## i.) Comparative Execution Time and Analysis

### Average kernel execution time (20 runs each)

| Vector size | Debug (C) | Debug (x86-64) | Release (C) | Release (x86-64) |
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

**1. Our assembly kernel is around 4x faster than C in Debug mode, but the two are basically tied in Release mode.**
The reason is that Debug mode compiles the C kernel with no optimization: the accumulator stays in memory, the array index is recomputed on every loop, and there is extra overhead per iteration. Our hand-written assembly has none of that, so it wins by roughly 4x. In Release mode, however, the compiler optimizes (/O2) and ends up producing essentially the same scalar SSE loop that we wrote by hand. This is why the C time dropped by about 4x (3126 ms to 778 ms at 2^30) while our assembly barely changed (786 ms to 775 ms), since NASM outputs the same instructions regardless of build mode. The remaining speedup in Release is only 1.005x to 1.059x. Notably, Release C at 2^30 (778 ms) is almost identical to Debug assembly (786 ms), which shows that the compiler converged on the same code we wrote manually.

**2. The bigger the vector, the smaller the speedup, because the kernel becomes memory-bound instead of CPU-bound.**
At 2^20, the two vectors are only 8 MB total, so part of the data fits in cache and instruction efficiency still matters slightly (1.059x). At 2^30, the program has to stream 8 GB from main memory, and at that point the bottleneck is DRAM bandwidth rather than the CPU. Both kernels end up waiting on memory equally, so their times converge (1.005x). This shows that hand-optimizing instructions gives diminishing returns once the workload is limited by memory.

**3. The sdot at 2^30 gets stuck at exactly 16,777,216, which is a single-precision accumulation limitation.**
Mathematically, the expected dot product of 2^30 random pairs should be around 2.7×10^8, yet both kernels output exactly 16,777,216.000000, which is exactly 2^24, and that is not a coincidence. Once the running sum reaches 2^24, the gap between consecutive representable float values becomes 2.0, so adding any product smaller than 1.0 rounds back to the same number. Every addition after that point is lost, and the sum stays stuck at 2^24. Both kernels accumulate in the same order with the same precision, so they hit the same ceiling and still match each other exactly (|diff| = 0). The correctness check passes as the spec requires, while also demonstrating a known weakness of naive single-precision accumulation. (Production BLAS libraries avoid this using techniques like pairwise summation or wider accumulators.)

---

## ii.) Program Output with Correctness Check (C version)

The program prints the C kernel's sdot alongside the x86-64 kernel's sdot, then validates the x86-64 output against the C "answer key." All sizes PASS with |diff| = 0.

**Debug mode:**

<img width="1483" height="762" alt="Debug_CMD" src="https://github.com/user-attachments/assets/7bc16216-75ab-4164-8a3e-7c0a7fa83cb9" />

**Release mode:**

<img width="1483" height="762" alt="Release_CMD" src="https://github.com/user-attachments/assets/d60a54e9-a180-4201-8e5e-d4155ea089f2" />

## iii.) Program Output with Correctness Check (x86-64 version)

**Debug mode:**

<img width="2559" height="1439" alt="Debug" src="https://github.com/user-attachments/assets/13cec891-172b-4442-b40d-2c7305dca95c" />

**Release mode:**

<img width="2555" height="1439" alt="Release" src="https://github.com/user-attachments/assets/a28543ff-66f5-454e-a455-12fa68611f58" />

## iv.) Short Video

Here is our video (5-10 mins) showing the source code, compilation, and execution of both the C and x86-64 versions:

**[VIDEO](https://youtu.be/UAW1UcU2r70)**

---

## Project Structure

```
├── Test/
│   ├── Source.asm
│   ├── Source.c
│   ├── Test.vcxproj
│   └── Test.vcxproj.filters
├── screenshots/
│   ├── Debug.png
│   ├── Debug_CMD.png
│   ├── Release.png
│   └── Release_CMD.png
├── specs/
│   └── x86-to-C interface programming project - Specs 1.png
├── README.md
└── Test.slnx
```

## How to Build and Run

1. Open `Test.slnx` in Visual Studio.
2. Set the configuration to **x64** and pick either **Debug** or **Release**.
3. `Source.asm` is assembled through a Custom Build Tool step using NASM (`nasm -f win64`), already configured for both Debug and Release.

   > **Note:** The custom build step points to NASM using an absolute path (`C:\Users\Admin\Documents\NASM\nasm.exe`). If NASM is installed somewhere else on your machine, update the path under *Source.asm > Properties > Custom Build Tool > Command Line* (for both Debug and Release configurations) before building, or the assembly file will not compile.

4. Build the solution, then run it with the Local Windows Debugger (or run `Test.exe` directly).
5. The program goes through all three vector sizes automatically. Note that the 2^30 phase allocates around 8 GB of RAM and takes a few minutes in Debug mode, so it may look stuck even when it is running normally. If the machine cannot allocate that much memory, the program automatically falls back to 2^29 / 2^28, which the specs allow.
