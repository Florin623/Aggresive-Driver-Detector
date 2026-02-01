#include <cuda_runtime.h>

void profilePerceptionPipelineCUDA(
    const uint8_t* d_leftImage,
    const uint8_t* d_rightImage,
    const float* d_lidarDepth,
    float* d_fusedDepth,
    int width,
    int height,
    int maxDisparity,
    float alpha,
    double& elapsed_ms)
{
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);

    // 1. Stereo disparity computation
    dim3 threads(16, 16);
    dim3 blocks((width + threads.x - 1) / threads.x,
                (height + threads.y - 1) / threads.y);

    stereoDisparityKernel<<<blocks, threads>>>(
        d_leftImage, d_rightImage,
        d_fusedDepth, width, height, maxDisparity
    );

    // 2. Depth post-processing (smoothing)
    depthSmoothingKernel<<<blocks, threads>>>(
        d_fusedDepth, d_fusedDepth,
        width, height
    );

    // 3. Depth–LiDAR fusion
    depthFusionKernel<<<blocks, threads>>>(
        d_fusedDepth, d_lidarDepth,
        d_fusedDepth,
        width, height, alpha
    );

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float ms = 0.0f;
    cudaEventElapsedTime(&ms, start, stop);
    elapsed_ms = static_cast<double>(ms);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);
}

