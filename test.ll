; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @test(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  store i32 %0, ptr %3, align 4
  store i32 10, ptr %4, align 4
  %6 = call i32 (...) @read()
  store i32 %6, ptr %5, align 4
  %7 = load i32, ptr %3, align 4
  %8 = load i32, ptr %4, align 4
  %9 = icmp slt i32 %7, %8
  br i1 %9, label %10, label %16

10:                                               ; preds = %1
  %11 = load i32, ptr %5, align 4
  %12 = load i32, ptr %4, align 4
  %13 = add nsw i32 %11, %12
  store i32 %13, ptr %5, align 4
  %14 = load i32, ptr %5, align 4
  call void @print(i32 noundef %14)
  %15 = load i32, ptr %5, align 4
  store i32 %15, ptr %2, align 4
  br label %22

16:                                               ; preds = %1
  %17 = load i32, ptr %5, align 4
  %18 = load i32, ptr %3, align 4
  %19 = add nsw i32 %17, %18
  store i32 %19, ptr %5, align 4
  %20 = load i32, ptr %5, align 4
  call void @print(i32 noundef %20)
  %21 = load i32, ptr %5, align 4
  store i32 %21, ptr %2, align 4
  br label %22

22:                                               ; preds = %16, %10
  %23 = load i32, ptr %2, align 4
  ret i32 %23
}

declare i32 @read(...) #1

declare void @print(i32 noundef) #1

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 15.0.7"}
