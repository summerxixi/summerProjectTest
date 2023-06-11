#include <stdio.h>
#include <thread>
#include <iostream>
#include <iomanip>
#include <chrono>


#include "CycleTimer.h"

typedef struct {
    float x0, x1;
    float y0, y1;
    unsigned int width;
    unsigned int height;
    int maxIterations;
    int* output;
    int threadId;
    int numThreads;
} WorkerArgs;


extern void mandelbrotSerial(
    float x0, float y0, float x1, float y1,
    int width, int height,
    int startRow, int numRows,
    int maxIterations,
    int output[]);


//
// workerThreadStart --
//
// Thread entrypoint.
void workerThreadStart(WorkerArgs* const args) {

    // Compute the part of the image assigned to this thread
    int numRows = args->height / args->numThreads;
    int startRow = args->threadId * numRows;

    auto startTime = std::chrono::high_resolution_clock::now();

    mandelbrotSerial(args->x0, args->y0, args->x1, args->y1,
        args->width, args->height,
        startRow, numRows,
        args->maxIterations,
        args->output);

    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Thread " << args->threadId << ": done in " << std::fixed << std::setprecision(4)
              << (std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()) / 1000.0
              << " seconds. Computing rows from " << startRow << " to " << startRow + numRows - 1 << std::endl;

}

//
// MandelbrotThread --
//
// Multi-threaded implementation of mandelbrot set image generation.
// Threads of execution are created by spawning std::threads.
void mandelbrotThread(
    int numThreads,
    float x0, float y0, float x1, float y1,
    int width, int height,
    int maxIterations, int output[])
{
    static constexpr int MAX_THREADS = 32;

    if (numThreads > MAX_THREADS)
    {
        fprintf(stderr, "Error: Max allowed threads is %d\n", MAX_THREADS);
        exit(1);
    }

    // Creates thread objects that do not yet represent a thread.
    std::thread workers[MAX_THREADS];
    WorkerArgs args[MAX_THREADS];

    for (int i = 0; i < numThreads; i++) {

        args[i].x0 = x0;
        args[i].y0 = y0;
        args[i].x1 = x1;
        args[i].y1 = y1;
        args[i].width = width;
        args[i].height = height;
        args[i].maxIterations = maxIterations;
        args[i].numThreads = numThreads;
        args[i].output = output;

        args[i].threadId = i;
    }

    // Spawn the worker threads
    auto startTime = std::chrono::high_resolution_clock::now();
    for (int i = 1; i < numThreads; i++) {
        workers[i] = std::thread(workerThreadStart, &args[i]);
    }

    workerThreadStart(&args[0]);

    // join worker threads
    for (int i = 1; i < numThreads; i++) {
        workers[i].join();
    }
    auto endTime = std::chrono::high_resolution_clock::now();

    std::cout << "MandelbrotThread " << numThreads << " threads : Time = "
              << (std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()) / 1000.0
              << " secs." << std::endl;
}

// Test different thread counts and measure their runtimes
void testThreadCounts() {
    const int width = 2048;
    const int height = 2048;
    const int maxIterations = 256;
    const float left = -2.0;
    const float right = 1.0;
    const float top = -1.0;
    const float bottom = 1.0;

    int* outputSerial = new int[width * height];
    int* outputParallel = new int[width * height];

    // Compute the reference serial version
    auto startTime = std::chrono::high_resolution_clock::now();
    mandelbrotSerial(left, top, right, bottom,
        width, height,
        0, height,
        maxIterations,
        outputSerial);
    auto endTime = std::chrono::high_resolution_clock::now();
    std::cout << "Serial version : Time = "
              << (std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count()) / 1000.0
              << " secs." << std::endl;

    // Test various thread counts and measure their runtimes
    for (int numThreads = 2; numThreads <= 8; numThreads++) {
        memset(outputParallel, 0, width * height * sizeof(int));
        mandelbrotThread(numThreads, left, top, right, bottom,
            width, height,
            maxIterations,
            outputParallel);

        // Check output against reference serial version
        bool passed = true;
        for (int i = 0; i < width * height; i++) {
            if (outputSerial[i] != outputParallel[i]) {
                printf("Mismatch at index %d\n", i);
                passed = false;
                break;
            }
        }

        if (passed) {
            std::cout << "Passed correctness check for " << numThreads << " threads" << std::endl;
        }
        else {
            std::cout << "Failed correctness check for " << numThreads << " threads" << std::endl;
        }
    }

    delete[] outputSerial;
    delete[] outputParallel;
}

int main() {
    testThreadCounts();
    return 0;
}

