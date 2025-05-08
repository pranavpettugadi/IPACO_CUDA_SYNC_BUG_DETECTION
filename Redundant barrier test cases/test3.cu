#include <stdio.h>

__global__ void test(int *data) {
    // give for loop a with redundant barrier
        for (int i = 0; i < 256; i++) {
            __syncthreads();
            data[i] = i;
            __syncthreads();
        }
        // give for loop a without redundant barrier
        for (int i = 0; i < 256; i++) {
            data[i] = i;
        }
        // give for loop b with redundant barrier
        for (int i = 0; i < 256; i++) {
            __syncthreads();
            data[i] = i;
            __syncthreads();
        }
        // give for loop b without redundant barrier
        for (int i = 0; i < 256; i++) {
            data[i] = i;
        }
        
    }

int main() {
    int *d_data;
    cudaMalloc(&d_data, sizeof(int) * 256);
    test<<<1, 256>>>(d_data);
    cudaDeviceSynchronize();
    cudaFree(d_data);
    return 0;
}

