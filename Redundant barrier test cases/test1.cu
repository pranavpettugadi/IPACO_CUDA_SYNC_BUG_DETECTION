#include <stdio.h>


__global__ void test0(int *data) {
    int tid = threadIdx.x;
    __syncthreads();  // Redundant: no access after this depends on data written before
    data[tid] = tid;
    
}

__global__ void test1(int *data) {
    int tid = threadIdx.x;
    data[tid] = tid;
    __syncthreads();  // Redundant: no access after this depends on data written before
    int x = tid + 1;
}

__global__ void test2(int *data) {
    int tid = threadIdx.x;
    data[tid] = tid;
    __syncthreads();  // Necessary: next line reads from data written by other threads
    int val = data[(tid + 1) %256];  // Accesses data written by other threads
}

__global__ void test3(int *data) {
    int tid = threadIdx.x;
   int a = data[tid + 1];
__syncthreads();    // redundant
int b = data[tid + 1];

}

__global__ void test4(int *data) {
    int tid = threadIdx.x;
   int a = data[tid + 1];
__syncthreads();   // redundant
data[tid + 1] = tid;

}
__global__ void test5(int *data) {
    int tid = threadIdx.x;
    data[tid + 1] = tid;
__syncthreads();    // redundant
int b = data[tid + 1];

}

__global__ void test6(int *data) {
    int tid = threadIdx.x;
   data[tid + 1] = 1;
__syncthreads();    // redundant
data[tid + 1] = 2;
}


__global__ void test7(int *data) {
    int tid = threadIdx.x;
   int a = data[tid +1];
__syncthreads();    // redundant
int b = data[2*tid + 1];

}

__global__ void test8(int *data) {
    int tid = threadIdx.x;
   int a = data[tid + 1];
__syncthreads();    // redundant
data[2*tid + 1] = tid;

}
__global__ void test9(int *data) {
    int tid = threadIdx.x;
    data[tid + 1] = tid;
__syncthreads();    // neccesary
int b = data[2*tid + 1];

}

__global__ void test10(int *data) {
    int tid = threadIdx.x;
   data[tid + 1] = 1;
__syncthreads();    // neccesary
data[2*tid + 1] = 2;
}


__global__ void test11(int *data,int *data2) {
    int tid = threadIdx.x;
    data[tid] = tid;
    data2[tid] = tid + 1;
    __syncthreads();  // Necessary: data2 is written by other threads
    int val = data2[tid];
    int val2 = data[tid+1];
    __syncthreads();  // neccesary: data is written by other threads
    data[tid] = val * 2;
}

int main() {
    int *d_data;
    cudaMalloc(&d_data, sizeof(int) * 256);
    test1<<<1, 256>>>(d_data);
    test2<<<1, 256>>>(d_data);
    test3<<<1, 256>>>(d_data);
    test4<<<1, 256>>>(d_data);
    test5<<<1, 256>>>(d_data);  
    test6<<<1, 256>>>(d_data);
    test7<<<1, 256>>>(d_data);
    test8<<<1, 256>>>(d_data);
    test9<<<1, 256>>>(d_data);
    test10<<<1, 256>>>(d_data);
    int *d_data2;
    cudaMalloc(&d_data2, sizeof(int) * 256);
    test11<<<1, 256>>>(d_data, d_data2);
    cudaDeviceSynchronize();
    cudaFree(d_data);
    return 0;
}
