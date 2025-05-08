#include <stdio.h>


__global__ void test(int *data) {
    int tid = threadIdx.x;
    int a,b=0,c=tid;
    if(tid == 0) {
        a=10;
    }
    else {
        a=20;
    }
    data[a*tid] = tid;
    __syncthreads();  // Redundant: no access after this depends on data written before
    data[tid] = tid;
        
}


int main() {
    int *d_data;
    cudaMalloc(&d_data, sizeof(int) * 256);
    test<<<1, 256>>>(d_data);
    cudaDeviceSynchronize();
    cudaFree(d_data);
    return 0;
}
