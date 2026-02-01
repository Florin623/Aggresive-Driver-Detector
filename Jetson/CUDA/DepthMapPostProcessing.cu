__global__ void depthSmoothingKernel(const float* input,
                                     float* output,
                                     int width,
                                     int height)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x <= 0 || y <= 0 || x >= width - 1 || y >= height - 1)
        return;

    float sum = 0.0f;

    for (int dy = -1; dy <= 1; ++dy)
    {
        for (int dx = -1; dx <= 1; ++dx)
        {
            int idx = (y + dy) * width + (x + dx);
            sum += input[idx];
        }
    }

    output[y * width + x] = sum / 9.0f;
}

__global__ void depthFusionKernel(const float* stereoDepth,
                                  const float* lidarDepth,
                                  float* fusedDepth,
                                  int width,
                                  int height,
                                  float alpha)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height)
        return;

    int idx = y * width + x;

    float zs = stereoDepth[idx];
    float zl = lidarDepth[idx];

    if (zl > 0.0f)
    {
        fusedDepth[idx] = alpha * zs + (1.0f - alpha) * zl;
    }
    else
    {
        fusedDepth[idx] = zs;
    }
}

void postProcessAndFuseCUDA(const float* d_stereo,
                            const float* d_lidar,
                            float* d_fused,
                            int width,
                            int height,
                            float alpha)
{
    float* d_smoothed;
    size_t bytes = width * height * sizeof(float);

    cudaMalloc(&d_smoothed, bytes);

    dim3 threads(16, 16);
    dim3 blocks((width + threads.x - 1) / threads.x,
                (height + threads.y - 1) / threads.y);

    depthSmoothingKernel<<<blocks, threads>>>(
        d_stereo, d_smoothed, width, height
    );

    depthFusionKernel<<<blocks, threads>>>(
        d_smoothed, d_lidar, d_fused,
        width, height, alpha
    );

    cudaFree(d_smoothed);
}

