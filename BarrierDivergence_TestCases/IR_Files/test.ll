; ModuleID = 'test.cu'
source_filename = "test.cu"
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
  store float* %0, float** %4, align 8
  store float* %1, float** %5, align 8
  store float* %2, float** %6, align 8
  %11 = call i32 @llvm.nvvm.read.ptx.sreg.ctaid.x() #3, !range !6
  %12 = call i32 @llvm.nvvm.read.ptx.sreg.ntid.x() #3, !range !7
  %13 = mul i32 %11, %12
  %14 = call i32 @llvm.nvvm.read.ptx.sreg.tid.x() #3, !range !8
  %15 = add i32 %13, %14
  store i32 %15, i32* %7, align 4
  store i32 3, i32* %8, align 4
  store i32 3, i32* %9, align 4
  %16 = load float*, float** %6, align 8
  %17 = load i32, i32* %7, align 4
  %18 = sext i32 %17 to i64
  %19 = getelementptr inbounds float, float* %16, i64 %18
  %20 = load float, float* %19, align 4
  %21 = load i32, i32* %9, align 4
  %22 = sitofp i32 %21 to float
  %23 = fcmp contract olt float %20, %22
  br i1 %23, label %24, label %40

24:                                               ; preds = %3
  %25 = load float*, float** %4, align 8
  %26 = load i32, i32* %7, align 4
  %27 = sext i32 %26 to i64
  %28 = getelementptr inbounds float, float* %25, i64 %27
  %29 = load float, float* %28, align 4
  %30 = load float*, float** %5, align 8
  %31 = load i32, i32* %7, align 4
  %32 = sext i32 %31 to i64
  %33 = getelementptr inbounds float, float* %30, i64 %32
  %34 = load float, float* %33, align 4
  %35 = fadd contract float %29, %34
  %36 = load float*, float** %6, align 8
  %37 = load i32, i32* %7, align 4
  %38 = sext i32 %37 to i64
  %39 = getelementptr inbounds float, float* %36, i64 %38
  store float %35, float* %39, align 4
  call void @llvm.nvvm.barrier0()
  br label %65

40:                                               ; preds = %3
  %41 = load i32, i32* %7, align 4
  %42 = load i32, i32* %9, align 4
  %43 = icmp slt i32 %41, %42
  br i1 %43, label %44, label %54

44:                                               ; preds = %40
  %45 = load float*, float** %4, align 8
  %46 = load i32, i32* %7, align 4
  %47 = sext i32 %46 to i64
  %48 = getelementptr inbounds float, float* %45, i64 %47
  %49 = load float, float* %48, align 4
  %50 = load float*, float** %6, align 8
  %51 = load i32, i32* %7, align 4
  %52 = sext i32 %51 to i64
  %53 = getelementptr inbounds float, float* %50, i64 %52
  store float %49, float* %53, align 4
  call void @llvm.nvvm.barrier0()
  br label %64

54:                                               ; preds = %40
  %55 = load i32, i32* %8, align 4
  %56 = icmp sle i32 %55, 100
  br i1 %56, label %57, label %58

57:                                               ; preds = %54
  call void @llvm.nvvm.barrier0()
  br label %63

58:                                               ; preds = %54
  %59 = load i32, i32* %7, align 4
  %60 = icmp sge i32 %59, 2
  br i1 %60, label %61, label %62

61:                                               ; preds = %58
  br label %62

62:                                               ; preds = %61, %58
  br label %63

63:                                               ; preds = %62, %57
  br label %64

64:                                               ; preds = %63, %44
  br label %65

65:                                               ; preds = %64, %24
  %66 = load i32, i32* %7, align 4
  store i32 %66, i32* %10, align 4
  %67 = load i32, i32* %10, align 4
  %68 = add nsw i32 %67, 1
  store i32 %68, i32* %10, align 4
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