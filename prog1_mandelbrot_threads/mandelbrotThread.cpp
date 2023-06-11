#include <stdio.h>
#include <thread>
#include <iostream>
#include <iomanip>

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

    // Compute the range of rows that this thread will do.
    int startRowCount = args->threadId * (args->height / args->numThreads);
    int endRowCount = startRowCount + (args->height / args->numThreads);

    // Check if we need to adjust for the last row when the image height is
    // not evenly divisible by the number of threads.
    if (args->threadId == args->numThreads - 1) {
        endRowCount = args->height;
    }

    double startTime = CycleTimer::currentSeconds();

    // Call mandelbrotSerial() to generate the corresponding portion of the image.
    mandelbrotSerial(args->x0, args->y0, args->x1, args->y1, args->width, args->height, startRowCount, endRowCount - startRowCount, args->maxIterations, args->output);

    double endTime = CycleTimer::currentSeconds();

    // Output thread execution time and finished information
    std::cout << "mandelbrotThread from thread " << args->threadId << std::setw(2) <<  " finished in " << std::fixed << std::setprecision(4) << (endTime - startTime) * 1000 << " ms. "  << std::endl;

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

        // Copy the same arguments for each thread
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

    // Spawn the worker threads.
    for (int i = 1; i < numThreads; i++) {
        workers[i] = std::thread(workerThreadStart, &args[i]);
    }

    workerThreadStart(&args[0]);

    // join worker threads
    for (int i = 1; i < numThreads; i++) {
        workers[i].join();
    }
}

