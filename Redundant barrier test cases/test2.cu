#include <stdio.h>


__global__ void test(int *data) {
    int tid = threadIdx.x;
    data[3]= 1;
    __syncthreads();  // Redundant: no access after this depends on data written before
    data[1] = tid;
    
    __syncthreads();  // Redundant: no access after this depends on data written before
    int a= data[2];
    __syncthreads();  // Redundant: no access after this depends on data written before
    int b= data[3];
        
}


int main() {
    int *d_data;
    cudaMalloc(&d_data, sizeof(int) * 256);
    test<<<1, 256>>>(d_data);
    cudaDeviceSynchronize();
    cudaFree(d_data);
    return 0;
}
