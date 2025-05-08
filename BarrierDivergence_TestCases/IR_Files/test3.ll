; ModuleID = 'test3.cu'
source_filename = "test3.cu"
target datalayout = "e-i64:64-i128:128-v16:16-v32:32-n16:32:64"
target triple = "nvptx64-nvidia-cuda"

%struct.__cuda_builtin_blockIdx_t = type { i8 }
%struct.__cuda_builtin_blockDim_t = type { i8 }
%struct.__cuda_builtin_threadIdx_t = type { i8 }

@blockIdx = extern_weak dso_local addrspace(1) global %struct.__cuda_builtin_blockIdx_t, align 1
@blockDim = extern_weak dso_local addrspace(1) global %struct.__cuda_builtin_blockDim_t, align 1
@threadIdx = extern_weak dso_local addrspace(1) global %struct.__cuda_builtin_threadIdx_t, align 1

; Function Attrs: convergent noinline norecurse nounwind optnone
define dso_local void @_Z9vectorAddPKfS0_Pf(float* %0, float* %1, float* %2) #0 {
  %4 = alloca float*, align 8
  %5 = alloca float*, align 8
  %6 = alloca float*, align 8
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  %9 = alloca i32, align 4
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca i32, align 4
  store float* %0, float** %4, align 8
  store float* %1, float** %5, align 8
  store float* %2, float** %6, align 8
  %13 = call i32 @llvm.nvvm.read.ptx.sreg.ctaid.x() #3, !range !6
  %14 = call i32 @llvm.nvvm.read.ptx.sreg.ntid.x() #3, !range !7
  %15 = mul i32 %13, %14
  %16 = call i32 @llvm.nvvm.read.ptx.sreg.tid.x() #3, !range !8
  %17 = add i32 %15, %16
  store i32 %17, i32* %7, align 4
  store i32 3, i32* %8, align 4
  store i32 5, i32* %9, align 4
  store i32 4, i32* %10, align 4
  store i32 1, i32* %11, align 4
  %18 = load i32, i32* %9, align 4
  %19 = load i32, i32* %7, align 4
  %20 = mul nsw i32 %18, %19
  %21 = add nsw i32 %20, 4
  %22 = load i32, i32* %11, align 4
  %23 = sdiv i32 %21, %22
  store i32 %23, i32* %12, align 4
  %24 = load i32, i32* %12, align 4
  %25 = mul nsw i32 2, %24
  %26 = icmp sge i32 %25, -32
  br i1 %26, label %27, label %28

27:                                               ; preds = %3
  call void @llvm.nvvm.barrier0()
  br label %28

28:                                               ; preds = %27, %3
  ret void
}

; Function Attrs: convergent nounwind
declare void @llvm.nvvm.barrier0() #1

; Function Attrs: nounwind readnone
declare i32 @llvm.nvvm.read.ptx.sreg.ctaid.x() #2

; Function Attrs: nounwind readnone
declare i32 @llvm.nvvm.read.ptx.sreg.ntid.x() #2

; Function Attrs: nounwind readnone
declare i32 @llvm.nvvm.read.ptx.sreg.tid.x() #2

attributes #0 = { convergent noinline norecurse nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="sm_35" "target-features"="+ptx70,+sm_35" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { convergent nounwind }
attributes #2 = { nounwind readnone }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0, !1, !2}
!nvvm.annotations = !{!3}
!llvm.ident = !{!4, !5}

!0 = !{i32 2, !"SDK Version", [2 x i32] [i32 11, i32 0]}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{i32 4, !"nvvm-reflect-ftz", i32 0}
!3 = !{void (float*, float*, float*)* @_Z9vectorAddPKfS0_Pf, !"kernel", i32 1}
!4 = !{!"Debian clang version 11.0.1-2"}
!5 = !{!"clang version 3.8.0 (tags/RELEASE_380/final)"}
!6 = !{i32 0, i32 2147483647}
!7 = !{i32 1, i32 1025}
!8 = !{i32 0, i32 1024}
