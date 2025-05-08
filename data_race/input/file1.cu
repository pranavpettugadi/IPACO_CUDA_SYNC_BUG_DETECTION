#include <cuda_runtime.h>
#include <iostream>

__global__ void myKernel(int *a, int x, int y) {
    int tid = threadIdx.x;
    a[tid] = x;
    a[tid + 1] = y; // Data Race
}

int main() {
    const int N = 10;
    int *d_a, h_a[N];

    // Allocate memory on the device
    cudaMalloc(&d_a, N * sizeof(int));

    // Launch kernel with 1 block and 1 thread
    myKernel<<<1, 1>>>(d_a, 5, 10);

    // Copy results back to host
    cudaMemcpy(h_a, d_a, N * sizeof(int), cudaMemcpyDeviceToHost);

    // Print the first few elements
    for (int i = 0; i < 4; ++i)
        std::cout << "a[" << i << "] = " << h_a[i] << std::endl;

    // Free device memory
    cudaFree(d_a);
    return 0;
}
