#include <cuda_runtime.h>
#include <cmath>

__global__ void stereoDisparityKernel(const uint8_t* left,
                                      const uint8_t* right,
                                      float* disparity,
                                      int width,
                                      int height,
                                      int maxDisparity)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height)
        return;

    int idx = y * width + x;

    int bestDisp = 0;
    int minCost = INT_MAX;

    for (int d = 0; d < maxDisparity; ++d)
    {
        int xr = x - d;
        if (xr < 0)
            break;

        int idxR = y * width + xr;
        int cost = abs(left[idx] - right[idxR]);

        if (cost < minCost)
        {
            minCost = cost;
            bestDisp = d;
        }
    }

    disparity[idx] = static_cast<float>(bestDisp);
}

void computeStereoDisparityCUDA(const uint8_t* h_left,
                                const uint8_t* h_right,
                                float* h_disparity,
                                int width,
                                int height,
                                int maxDisparity)
{
    size_t imgBytes = width * height * sizeof(uint8_t);
    size_t dispBytes = width * height * sizeof(float);

    uint8_t *d_left, *d_right;
    float* d_disp;

    cudaMalloc(&d_left, imgBytes);
    cudaMalloc(&d_right, imgBytes);
    cudaMalloc(&d_disp, dispBytes);

    cudaMemcpy(d_left, h_left, imgBytes, cudaMemcpyHostToDevice);
    cudaMemcpy(d_right, h_right, imgBytes, cudaMemcpyHostToDevice);

    dim3 threads(16, 16);
    dim3 blocks((width + threads.x - 1) / threads.x,
                (height + threads.y - 1) / threads.y);

    stereoDisparityKernel<<<blocks, threads>>>(
        d_left, d_right, d_disp,
        width, height, maxDisparity
    );

    cudaMemcpy(h_disparity, d_disp, dispBytes, cudaMemcpyDeviceToHost);

    cudaFree(d_left);
    cudaFree(d_right);
    cudaFree(d_disp);
}

