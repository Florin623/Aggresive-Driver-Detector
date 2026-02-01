#include <chrono>

void preprocessCPU(const uint8_t* input,
                   float* output,
                   int width,
                   int height,
                   double& elapsed_ms)
{
    int size = width * height;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < size; ++i)
    {
        output[i] = static_cast<float>(input[i]) / 255.0f;
    }

    auto end = std::chrono::high_resolution_clock::now();
    elapsed_ms =
        std::chrono::duration<double, std::milli>(end - start).count();
}

#include <cuda_runtime.h>

__global__ void preprocessKernel(const uint8_t* input,
                                 float* output,
                                 int size)
{
    int idx = blockIdx.x * blockDim.x + threadIdx.x;

    if (idx < size)
    {
        output[idx] = static_cast<float>(input[idx]) / 255.0f;
    }
}

void preprocessCUDA(const uint8_t* h_input,
                     float* h_output,
                     int width,
                     int height,
                     double& elapsed_ms)
{
    int size = width * height;

    size_t inputBytes = size * sizeof(uint8_t);
    size_t outputBytes = size * sizeof(float);

    uint8_t* d_input = nullptr;
    float* d_output = nullptr;

    cudaMalloc(&d_input, inputBytes);
    cudaMalloc(&d_output, outputBytes);

    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    cudaEventRecord(start);

    cudaMemcpy(d_input, h_input, inputBytes, cudaMemcpyHostToDevice);

    int threadsPerBlock = 256;
    int blocks = (size + threadsPerBlock - 1) / threadsPerBlock;

    preprocessKernel<<<blocks, threadsPerBlock>>>(d_input, d_output, size);

    cudaMemcpy(h_output, d_output, outputBytes, cudaMemcpyDeviceToHost);

    cudaEventRecord(stop);
    cudaEventSynchronize(stop);

    float ms = 0.0f;
    cudaEventElapsedTime(&ms, start, stop);
    elapsed_ms = static_cast<double>(ms);

    cudaEventDestroy(start);
    cudaEventDestroy(stop);

    cudaFree(d_input);
    cudaFree(d_output);
}


#include <iostream>
#include <vector>

int main()
{
    const int width = 640;
    const int height = 480;
    const int size = width * height;

    std::vector<uint8_t> input(size, 128);
    std::vector<float> output_cpu(size);
    std::vector<float> output_cuda(size);

    double cpu_time = 0.0;
    double cuda_time = 0.0;

    preprocessCPU(input.data(), output_cpu.data(),
                  width, height, cpu_time);

    preprocessCUDA(input.data(), output_cuda.data(),
                   width, height, cuda_time);

    std::cout << "CPU preprocessing time:  "
              << cpu_time << " ms\n";

    std::cout << "CUDA preprocessing time: "
              << cuda_time << " ms\n";

    return 0;
}

