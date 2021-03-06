; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -march=x86-64 -mtriple=x86_64-apple-darwin -mcpu=knl --show-mc-encoding| FileCheck %s

define i32 @test1(float %x) {
; CHECK-LABEL: test1:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovd %xmm0, %eax ## encoding: [0x62,0xf1,0x7d,0x08,0x7e,0xc0]
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %res = bitcast float %x to i32
   ret i32 %res
}

define <4 x i32> @test2(i32 %x) {
; CHECK-LABEL: test2:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovd %edi, %xmm0 ## encoding: [0x62,0xf1,0x7d,0x08,0x6e,0xc7]
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %res = insertelement <4 x i32>undef, i32 %x, i32 0
   ret <4 x i32>%res
}

define <2 x i64> @test3(i64 %x) {
; CHECK-LABEL: test3:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovq %rdi, %xmm0 ## encoding: [0x62,0xf1,0xfd,0x08,0x6e,0xc7]
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %res = insertelement <2 x i64>undef, i64 %x, i32 0
   ret <2 x i64>%res
}

define <4 x i32> @test4(i32* %x) {
; CHECK-LABEL: test4:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovd (%rdi), %xmm0 ## encoding: [0x62,0xf1,0x7d,0x08,0x6e,0x07]
; CHECK-NEXT:    ## xmm0 = mem[0],zero,zero,zero
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %y = load i32, i32* %x
   %res = insertelement <4 x i32>undef, i32 %y, i32 0
   ret <4 x i32>%res
}

define void @test5(float %x, float* %y) {
; CHECK-LABEL: test5:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovss %xmm0, (%rdi) ## encoding: [0x62,0xf1,0x7e,0x08,0x11,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
   store float %x, float* %y, align 4
   ret void
}

define void @test6(double %x, double* %y) {
; CHECK-LABEL: test6:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovsd %xmm0, (%rdi) ## encoding: [0x62,0xf1,0xff,0x08,0x11,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
   store double %x, double* %y, align 8
   ret void
}

define float @test7(i32* %x) {
; CHECK-LABEL: test7:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovss (%rdi), %xmm0 ## encoding: [0x62,0xf1,0x7e,0x08,0x10,0x07]
; CHECK-NEXT:    ## xmm0 = mem[0],zero,zero,zero
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %y = load i32, i32* %x
   %res = bitcast i32 %y to float
   ret float %res
}

define i32 @test8(<4 x i32> %x) {
; CHECK-LABEL: test8:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovd %xmm0, %eax ## encoding: [0x62,0xf1,0x7d,0x08,0x7e,0xc0]
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %res = extractelement <4 x i32> %x, i32 0
   ret i32 %res
}

define i64 @test9(<2 x i64> %x) {
; CHECK-LABEL: test9:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovq %xmm0, %rax ## encoding: [0x62,0xf1,0xfd,0x08,0x7e,0xc0]
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %res = extractelement <2 x i64> %x, i32 0
   ret i64 %res
}

define <4 x i32> @test10(i32* %x) {
; CHECK-LABEL: test10:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovd (%rdi), %xmm0 ## encoding: [0x62,0xf1,0x7d,0x08,0x6e,0x07]
; CHECK-NEXT:    ## xmm0 = mem[0],zero,zero,zero
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %y = load i32, i32* %x, align 4
   %res = insertelement <4 x i32>zeroinitializer, i32 %y, i32 0
   ret <4 x i32>%res
}

define <4 x float> @test11(float* %x) {
; CHECK-LABEL: test11:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovss (%rdi), %xmm0 ## encoding: [0x62,0xf1,0x7e,0x08,0x10,0x07]
; CHECK-NEXT:    ## xmm0 = mem[0],zero,zero,zero
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %y = load float, float* %x, align 4
   %res = insertelement <4 x float>zeroinitializer, float %y, i32 0
   ret <4 x float>%res
}

define <2 x double> @test12(double* %x) {
; CHECK-LABEL: test12:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovsd (%rdi), %xmm0 ## encoding: [0x62,0xf1,0xff,0x08,0x10,0x07]
; CHECK-NEXT:    ## xmm0 = mem[0],zero
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %y = load double, double* %x, align 8
   %res = insertelement <2 x double>zeroinitializer, double %y, i32 0
   ret <2 x double>%res
}

define <2 x i64> @test13(i64 %x) {
; CHECK-LABEL: test13:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovq %rdi, %xmm0 ## encoding: [0x62,0xf1,0xfd,0x08,0x6e,0xc7]
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %res = insertelement <2 x i64>zeroinitializer, i64 %x, i32 0
   ret <2 x i64>%res
}

define <4 x i32> @test14(i32 %x) {
; CHECK-LABEL: test14:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovd %edi, %xmm0 ## encoding: [0x62,0xf1,0x7d,0x08,0x6e,0xc7]
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %res = insertelement <4 x i32>zeroinitializer, i32 %x, i32 0
   ret <4 x i32>%res
}

define <4 x i32> @test15(i32* %x) {
; CHECK-LABEL: test15:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovd (%rdi), %xmm0 ## encoding: [0x62,0xf1,0x7d,0x08,0x6e,0x07]
; CHECK-NEXT:    ## xmm0 = mem[0],zero,zero,zero
; CHECK-NEXT:    retq ## encoding: [0xc3]
   %y = load i32, i32* %x, align 4
   %res = insertelement <4 x i32>zeroinitializer, i32 %y, i32 0
   ret <4 x i32>%res
}

define <16 x i32> @test16(i8 * %addr) {
; CHECK-LABEL: test16:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovdqu32 (%rdi), %zmm0 ## encoding: [0x62,0xf1,0x7e,0x48,0x6f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <16 x i32>*
  %res = load <16 x i32>, <16 x i32>* %vaddr, align 1
  ret <16 x i32>%res
}

define <16 x i32> @test17(i8 * %addr) {
; CHECK-LABEL: test17:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovdqa32 (%rdi), %zmm0 ## encoding: [0x62,0xf1,0x7d,0x48,0x6f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <16 x i32>*
  %res = load <16 x i32>, <16 x i32>* %vaddr, align 64
  ret <16 x i32>%res
}

define void @test18(i8 * %addr, <8 x i64> %data) {
; CHECK-LABEL: test18:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovdqa64 %zmm0, (%rdi) ## encoding: [0x62,0xf1,0xfd,0x48,0x7f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <8 x i64>*
  store <8 x i64>%data, <8 x i64>* %vaddr, align 64
  ret void
}

define void @test19(i8 * %addr, <16 x i32> %data) {
; CHECK-LABEL: test19:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovdqu32 %zmm0, (%rdi) ## encoding: [0x62,0xf1,0x7e,0x48,0x7f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <16 x i32>*
  store <16 x i32>%data, <16 x i32>* %vaddr, align 1
  ret void
}

define void @test20(i8 * %addr, <16 x i32> %data) {
; CHECK-LABEL: test20:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovdqa32 %zmm0, (%rdi) ## encoding: [0x62,0xf1,0x7d,0x48,0x7f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <16 x i32>*
  store <16 x i32>%data, <16 x i32>* %vaddr, align 64
  ret void
}

define  <8 x i64> @test21(i8 * %addr) {
; CHECK-LABEL: test21:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovdqa64 (%rdi), %zmm0 ## encoding: [0x62,0xf1,0xfd,0x48,0x6f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <8 x i64>*
  %res = load <8 x i64>, <8 x i64>* %vaddr, align 64
  ret <8 x i64>%res
}

define void @test22(i8 * %addr, <8 x i64> %data) {
; CHECK-LABEL: test22:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovdqu64 %zmm0, (%rdi) ## encoding: [0x62,0xf1,0xfe,0x48,0x7f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <8 x i64>*
  store <8 x i64>%data, <8 x i64>* %vaddr, align 1
  ret void
}

define <8 x i64> @test23(i8 * %addr) {
; CHECK-LABEL: test23:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovdqu64 (%rdi), %zmm0 ## encoding: [0x62,0xf1,0xfe,0x48,0x6f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <8 x i64>*
  %res = load <8 x i64>, <8 x i64>* %vaddr, align 1
  ret <8 x i64>%res
}

define void @test24(i8 * %addr, <8 x double> %data) {
; CHECK-LABEL: test24:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovapd %zmm0, (%rdi) ## encoding: [0x62,0xf1,0xfd,0x48,0x29,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <8 x double>*
  store <8 x double>%data, <8 x double>* %vaddr, align 64
  ret void
}

define <8 x double> @test25(i8 * %addr) {
; CHECK-LABEL: test25:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovapd (%rdi), %zmm0 ## encoding: [0x62,0xf1,0xfd,0x48,0x28,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <8 x double>*
  %res = load <8 x double>, <8 x double>* %vaddr, align 64
  ret <8 x double>%res
}

define void @test26(i8 * %addr, <16 x float> %data) {
; CHECK-LABEL: test26:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovaps %zmm0, (%rdi) ## encoding: [0x62,0xf1,0x7c,0x48,0x29,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <16 x float>*
  store <16 x float>%data, <16 x float>* %vaddr, align 64
  ret void
}

define <16 x float> @test27(i8 * %addr) {
; CHECK-LABEL: test27:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovaps (%rdi), %zmm0 ## encoding: [0x62,0xf1,0x7c,0x48,0x28,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <16 x float>*
  %res = load <16 x float>, <16 x float>* %vaddr, align 64
  ret <16 x float>%res
}

define void @test28(i8 * %addr, <8 x double> %data) {
; CHECK-LABEL: test28:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovupd %zmm0, (%rdi) ## encoding: [0x62,0xf1,0xfd,0x48,0x11,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <8 x double>*
  store <8 x double>%data, <8 x double>* %vaddr, align 1
  ret void
}

define <8 x double> @test29(i8 * %addr) {
; CHECK-LABEL: test29:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovupd (%rdi), %zmm0 ## encoding: [0x62,0xf1,0xfd,0x48,0x10,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <8 x double>*
  %res = load <8 x double>, <8 x double>* %vaddr, align 1
  ret <8 x double>%res
}

define void @test30(i8 * %addr, <16 x float> %data) {
; CHECK-LABEL: test30:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovups %zmm0, (%rdi) ## encoding: [0x62,0xf1,0x7c,0x48,0x11,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <16 x float>*
  store <16 x float>%data, <16 x float>* %vaddr, align 1
  ret void
}

define <16 x float> @test31(i8 * %addr) {
; CHECK-LABEL: test31:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vmovups (%rdi), %zmm0 ## encoding: [0x62,0xf1,0x7c,0x48,0x10,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %vaddr = bitcast i8* %addr to <16 x float>*
  %res = load <16 x float>, <16 x float>* %vaddr, align 1
  ret <16 x float>%res
}

define <16 x i32> @test32(i8 * %addr, <16 x i32> %old, <16 x i32> %mask1) {
; CHECK-LABEL: test32:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm2, %zmm2, %zmm2 ## encoding: [0x62,0xf1,0x6d,0x48,0xef,0xd2]
; CHECK-NEXT:    vpcmpneqd %zmm2, %zmm1, %k1 ## encoding: [0x62,0xf3,0x75,0x48,0x1f,0xca,0x04]
; CHECK-NEXT:    vpblendmd (%rdi), %zmm0, %zmm0 {%k1} ## encoding: [0x62,0xf2,0x7d,0x49,0x64,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = icmp ne <16 x i32> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <16 x i32>*
  %r = load <16 x i32>, <16 x i32>* %vaddr, align 64
  %res = select <16 x i1> %mask, <16 x i32> %r, <16 x i32> %old
  ret <16 x i32>%res
}

define <16 x i32> @test33(i8 * %addr, <16 x i32> %old, <16 x i32> %mask1) {
; CHECK-LABEL: test33:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm2, %zmm2, %zmm2 ## encoding: [0x62,0xf1,0x6d,0x48,0xef,0xd2]
; CHECK-NEXT:    vpcmpneqd %zmm2, %zmm1, %k1 ## encoding: [0x62,0xf3,0x75,0x48,0x1f,0xca,0x04]
; CHECK-NEXT:    vpblendmd (%rdi), %zmm0, %zmm0 {%k1} ## encoding: [0x62,0xf2,0x7d,0x49,0x64,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = icmp ne <16 x i32> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <16 x i32>*
  %r = load <16 x i32>, <16 x i32>* %vaddr, align 1
  %res = select <16 x i1> %mask, <16 x i32> %r, <16 x i32> %old
  ret <16 x i32>%res
}

define <16 x i32> @test34(i8 * %addr, <16 x i32> %mask1) {
; CHECK-LABEL: test34:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm1, %zmm1, %zmm1 ## encoding: [0x62,0xf1,0x75,0x48,0xef,0xc9]
; CHECK-NEXT:    vpcmpneqd %zmm1, %zmm0, %k1 ## encoding: [0x62,0xf3,0x7d,0x48,0x1f,0xc9,0x04]
; CHECK-NEXT:    vmovdqa32 (%rdi), %zmm0 {%k1} {z} ## encoding: [0x62,0xf1,0x7d,0xc9,0x6f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = icmp ne <16 x i32> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <16 x i32>*
  %r = load <16 x i32>, <16 x i32>* %vaddr, align 64
  %res = select <16 x i1> %mask, <16 x i32> %r, <16 x i32> zeroinitializer
  ret <16 x i32>%res
}

define <16 x i32> @test35(i8 * %addr, <16 x i32> %mask1) {
; CHECK-LABEL: test35:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm1, %zmm1, %zmm1 ## encoding: [0x62,0xf1,0x75,0x48,0xef,0xc9]
; CHECK-NEXT:    vpcmpneqd %zmm1, %zmm0, %k1 ## encoding: [0x62,0xf3,0x7d,0x48,0x1f,0xc9,0x04]
; CHECK-NEXT:    vmovdqu32 (%rdi), %zmm0 {%k1} {z} ## encoding: [0x62,0xf1,0x7e,0xc9,0x6f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = icmp ne <16 x i32> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <16 x i32>*
  %r = load <16 x i32>, <16 x i32>* %vaddr, align 1
  %res = select <16 x i1> %mask, <16 x i32> %r, <16 x i32> zeroinitializer
  ret <16 x i32>%res
}

define <8 x i64> @test36(i8 * %addr, <8 x i64> %old, <8 x i64> %mask1) {
; CHECK-LABEL: test36:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm2, %zmm2, %zmm2 ## encoding: [0x62,0xf1,0x6d,0x48,0xef,0xd2]
; CHECK-NEXT:    vpcmpneqq %zmm2, %zmm1, %k1 ## encoding: [0x62,0xf3,0xf5,0x48,0x1f,0xca,0x04]
; CHECK-NEXT:    vpblendmq (%rdi), %zmm0, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xfd,0x49,0x64,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = icmp ne <8 x i64> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <8 x i64>*
  %r = load <8 x i64>, <8 x i64>* %vaddr, align 64
  %res = select <8 x i1> %mask, <8 x i64> %r, <8 x i64> %old
  ret <8 x i64>%res
}

define <8 x i64> @test37(i8 * %addr, <8 x i64> %old, <8 x i64> %mask1) {
; CHECK-LABEL: test37:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm2, %zmm2, %zmm2 ## encoding: [0x62,0xf1,0x6d,0x48,0xef,0xd2]
; CHECK-NEXT:    vpcmpneqq %zmm2, %zmm1, %k1 ## encoding: [0x62,0xf3,0xf5,0x48,0x1f,0xca,0x04]
; CHECK-NEXT:    vpblendmq (%rdi), %zmm0, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xfd,0x49,0x64,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = icmp ne <8 x i64> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <8 x i64>*
  %r = load <8 x i64>, <8 x i64>* %vaddr, align 1
  %res = select <8 x i1> %mask, <8 x i64> %r, <8 x i64> %old
  ret <8 x i64>%res
}

define <8 x i64> @test38(i8 * %addr, <8 x i64> %mask1) {
; CHECK-LABEL: test38:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm1, %zmm1, %zmm1 ## encoding: [0x62,0xf1,0x75,0x48,0xef,0xc9]
; CHECK-NEXT:    vpcmpneqq %zmm1, %zmm0, %k1 ## encoding: [0x62,0xf3,0xfd,0x48,0x1f,0xc9,0x04]
; CHECK-NEXT:    vmovdqa64 (%rdi), %zmm0 {%k1} {z} ## encoding: [0x62,0xf1,0xfd,0xc9,0x6f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = icmp ne <8 x i64> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <8 x i64>*
  %r = load <8 x i64>, <8 x i64>* %vaddr, align 64
  %res = select <8 x i1> %mask, <8 x i64> %r, <8 x i64> zeroinitializer
  ret <8 x i64>%res
}

define <8 x i64> @test39(i8 * %addr, <8 x i64> %mask1) {
; CHECK-LABEL: test39:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm1, %zmm1, %zmm1 ## encoding: [0x62,0xf1,0x75,0x48,0xef,0xc9]
; CHECK-NEXT:    vpcmpneqq %zmm1, %zmm0, %k1 ## encoding: [0x62,0xf3,0xfd,0x48,0x1f,0xc9,0x04]
; CHECK-NEXT:    vmovdqu64 (%rdi), %zmm0 {%k1} {z} ## encoding: [0x62,0xf1,0xfe,0xc9,0x6f,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = icmp ne <8 x i64> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <8 x i64>*
  %r = load <8 x i64>, <8 x i64>* %vaddr, align 1
  %res = select <8 x i1> %mask, <8 x i64> %r, <8 x i64> zeroinitializer
  ret <8 x i64>%res
}

define <16 x float> @test40(i8 * %addr, <16 x float> %old, <16 x float> %mask1) {
; CHECK-LABEL: test40:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm2, %zmm2, %zmm2 ## encoding: [0x62,0xf1,0x6d,0x48,0xef,0xd2]
; CHECK-NEXT:    vcmpordps %zmm2, %zmm1, %k1 ## encoding: [0x62,0xf1,0x74,0x48,0xc2,0xca,0x07]
; CHECK-NEXT:    vcmpneqps %zmm2, %zmm1, %k1 {%k1} ## encoding: [0x62,0xf1,0x74,0x49,0xc2,0xca,0x04]
; CHECK-NEXT:    vblendmps (%rdi), %zmm0, %zmm0 {%k1} ## encoding: [0x62,0xf2,0x7d,0x49,0x65,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = fcmp one <16 x float> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <16 x float>*
  %r = load <16 x float>, <16 x float>* %vaddr, align 64
  %res = select <16 x i1> %mask, <16 x float> %r, <16 x float> %old
  ret <16 x float>%res
}

define <16 x float> @test41(i8 * %addr, <16 x float> %old, <16 x float> %mask1) {
; CHECK-LABEL: test41:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm2, %zmm2, %zmm2 ## encoding: [0x62,0xf1,0x6d,0x48,0xef,0xd2]
; CHECK-NEXT:    vcmpordps %zmm2, %zmm1, %k1 ## encoding: [0x62,0xf1,0x74,0x48,0xc2,0xca,0x07]
; CHECK-NEXT:    vcmpneqps %zmm2, %zmm1, %k1 {%k1} ## encoding: [0x62,0xf1,0x74,0x49,0xc2,0xca,0x04]
; CHECK-NEXT:    vblendmps (%rdi), %zmm0, %zmm0 {%k1} ## encoding: [0x62,0xf2,0x7d,0x49,0x65,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = fcmp one <16 x float> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <16 x float>*
  %r = load <16 x float>, <16 x float>* %vaddr, align 1
  %res = select <16 x i1> %mask, <16 x float> %r, <16 x float> %old
  ret <16 x float>%res
}

define <16 x float> @test42(i8 * %addr, <16 x float> %mask1) {
; CHECK-LABEL: test42:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm1, %zmm1, %zmm1 ## encoding: [0x62,0xf1,0x75,0x48,0xef,0xc9]
; CHECK-NEXT:    vcmpordps %zmm1, %zmm0, %k1 ## encoding: [0x62,0xf1,0x7c,0x48,0xc2,0xc9,0x07]
; CHECK-NEXT:    vcmpneqps %zmm1, %zmm0, %k1 {%k1} ## encoding: [0x62,0xf1,0x7c,0x49,0xc2,0xc9,0x04]
; CHECK-NEXT:    vmovaps (%rdi), %zmm0 {%k1} {z} ## encoding: [0x62,0xf1,0x7c,0xc9,0x28,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = fcmp one <16 x float> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <16 x float>*
  %r = load <16 x float>, <16 x float>* %vaddr, align 64
  %res = select <16 x i1> %mask, <16 x float> %r, <16 x float> zeroinitializer
  ret <16 x float>%res
}

define <16 x float> @test43(i8 * %addr, <16 x float> %mask1) {
; CHECK-LABEL: test43:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm1, %zmm1, %zmm1 ## encoding: [0x62,0xf1,0x75,0x48,0xef,0xc9]
; CHECK-NEXT:    vcmpordps %zmm1, %zmm0, %k1 ## encoding: [0x62,0xf1,0x7c,0x48,0xc2,0xc9,0x07]
; CHECK-NEXT:    vcmpneqps %zmm1, %zmm0, %k1 {%k1} ## encoding: [0x62,0xf1,0x7c,0x49,0xc2,0xc9,0x04]
; CHECK-NEXT:    vmovups (%rdi), %zmm0 {%k1} {z} ## encoding: [0x62,0xf1,0x7c,0xc9,0x10,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = fcmp one <16 x float> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <16 x float>*
  %r = load <16 x float>, <16 x float>* %vaddr, align 1
  %res = select <16 x i1> %mask, <16 x float> %r, <16 x float> zeroinitializer
  ret <16 x float>%res
}

define <8 x double> @test44(i8 * %addr, <8 x double> %old, <8 x double> %mask1) {
; CHECK-LABEL: test44:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm2, %zmm2, %zmm2 ## encoding: [0x62,0xf1,0x6d,0x48,0xef,0xd2]
; CHECK-NEXT:    vcmpordpd %zmm2, %zmm1, %k1 ## encoding: [0x62,0xf1,0xf5,0x48,0xc2,0xca,0x07]
; CHECK-NEXT:    vcmpneqpd %zmm2, %zmm1, %k1 {%k1} ## encoding: [0x62,0xf1,0xf5,0x49,0xc2,0xca,0x04]
; CHECK-NEXT:    vblendmpd (%rdi), %zmm0, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xfd,0x49,0x65,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = fcmp one <8 x double> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <8 x double>*
  %r = load <8 x double>, <8 x double>* %vaddr, align 64
  %res = select <8 x i1> %mask, <8 x double> %r, <8 x double> %old
  ret <8 x double>%res
}

define <8 x double> @test45(i8 * %addr, <8 x double> %old, <8 x double> %mask1) {
; CHECK-LABEL: test45:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm2, %zmm2, %zmm2 ## encoding: [0x62,0xf1,0x6d,0x48,0xef,0xd2]
; CHECK-NEXT:    vcmpordpd %zmm2, %zmm1, %k1 ## encoding: [0x62,0xf1,0xf5,0x48,0xc2,0xca,0x07]
; CHECK-NEXT:    vcmpneqpd %zmm2, %zmm1, %k1 {%k1} ## encoding: [0x62,0xf1,0xf5,0x49,0xc2,0xca,0x04]
; CHECK-NEXT:    vblendmpd (%rdi), %zmm0, %zmm0 {%k1} ## encoding: [0x62,0xf2,0xfd,0x49,0x65,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = fcmp one <8 x double> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <8 x double>*
  %r = load <8 x double>, <8 x double>* %vaddr, align 1
  %res = select <8 x i1> %mask, <8 x double> %r, <8 x double> %old
  ret <8 x double>%res
}

define <8 x double> @test46(i8 * %addr, <8 x double> %mask1) {
; CHECK-LABEL: test46:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm1, %zmm1, %zmm1 ## encoding: [0x62,0xf1,0x75,0x48,0xef,0xc9]
; CHECK-NEXT:    vcmpordpd %zmm1, %zmm0, %k1 ## encoding: [0x62,0xf1,0xfd,0x48,0xc2,0xc9,0x07]
; CHECK-NEXT:    vcmpneqpd %zmm1, %zmm0, %k1 {%k1} ## encoding: [0x62,0xf1,0xfd,0x49,0xc2,0xc9,0x04]
; CHECK-NEXT:    vmovapd (%rdi), %zmm0 {%k1} {z} ## encoding: [0x62,0xf1,0xfd,0xc9,0x28,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = fcmp one <8 x double> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <8 x double>*
  %r = load <8 x double>, <8 x double>* %vaddr, align 64
  %res = select <8 x i1> %mask, <8 x double> %r, <8 x double> zeroinitializer
  ret <8 x double>%res
}

define <8 x double> @test47(i8 * %addr, <8 x double> %mask1) {
; CHECK-LABEL: test47:
; CHECK:       ## BB#0:
; CHECK-NEXT:    vpxord %zmm1, %zmm1, %zmm1 ## encoding: [0x62,0xf1,0x75,0x48,0xef,0xc9]
; CHECK-NEXT:    vcmpordpd %zmm1, %zmm0, %k1 ## encoding: [0x62,0xf1,0xfd,0x48,0xc2,0xc9,0x07]
; CHECK-NEXT:    vcmpneqpd %zmm1, %zmm0, %k1 {%k1} ## encoding: [0x62,0xf1,0xfd,0x49,0xc2,0xc9,0x04]
; CHECK-NEXT:    vmovupd (%rdi), %zmm0 {%k1} {z} ## encoding: [0x62,0xf1,0xfd,0xc9,0x10,0x07]
; CHECK-NEXT:    retq ## encoding: [0xc3]
  %mask = fcmp one <8 x double> %mask1, zeroinitializer
  %vaddr = bitcast i8* %addr to <8 x double>*
  %r = load <8 x double>, <8 x double>* %vaddr, align 1
  %res = select <8 x i1> %mask, <8 x double> %r, <8 x double> zeroinitializer
  ret <8 x double>%res
}
