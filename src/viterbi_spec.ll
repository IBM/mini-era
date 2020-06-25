; ModuleID = 'viterbi_spec.c'
source_filename = "viterbi_spec.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%union.branchtab27_u = type { [32 x i8] }

@d_branchtab27_generic = common dso_local local_unnamed_addr global [2 x %union.branchtab27_u] zeroinitializer, align 16

; Function Attrs: nounwind uwtable
define dso_local void @do_decoding(i32 %in_n_data_bits, i32 %in_cbps, i32 %in_ntraceback, i8* nocapture readonly %inMemory, i8* nocapture %outMemory) local_unnamed_addr #0 {
entry:
  %l_metric0_generic = alloca [64 x i8], align 16
  %l_metric0_generic1184 = getelementptr inbounds [64 x i8], [64 x i8]* %l_metric0_generic, i64 0, i64 0
  %l_metric1_generic = alloca [64 x i8], align 16
  %l_metric1_generic1185 = getelementptr inbounds [64 x i8], [64 x i8]* %l_metric1_generic, i64 0, i64 0
  %l_path0_generic = alloca [64 x i8], align 16
  %l_path0_generic1172 = getelementptr inbounds [64 x i8], [64 x i8]* %l_path0_generic, i64 0, i64 0
  %l_path1_generic = alloca [64 x i8], align 16
  %l_path1_generic1186 = getelementptr inbounds [64 x i8], [64 x i8]* %l_path1_generic, i64 0, i64 0
  %l_mmresult = alloca [64 x i8], align 16
  %l_mmresult1187 = getelementptr inbounds [64 x i8], [64 x i8]* %l_mmresult, i64 0, i64 0
  %l_ppresult = alloca [24 x [64 x i8]], align 16
  %m0 = alloca [16 x i8], align 16
  %m1 = alloca [16 x i8], align 16
  %m2 = alloca [16 x i8], align 16
  %m3 = alloca [16 x i8], align 16
  %decision0 = alloca [16 x i8], align 16
  %decision1 = alloca [16 x i8], align 16
  %survivor0 = alloca [16 x i8], align 16
  %survivor1 = alloca [16 x i8], align 16
  %metsv = alloca [16 x i8], align 16
  %metsvm = alloca [16 x i8], align 16
  %shift0 = alloca [16 x i8], align 16
  %shift1 = alloca [16 x i8], align 16
  %tmp0 = alloca [16 x i8], align 16
  %tmp1 = alloca [16 x i8], align 16
  %sym0v = alloca [16 x i8], align 16
  %sym1v = alloca [16 x i8], align 16
  %arrayidx1 = getelementptr inbounds i8, i8* %inMemory, i64 32
  %0 = getelementptr inbounds [64 x i8], [64 x i8]* %l_metric0_generic, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 64, i8* nonnull %0) #2
  %1 = getelementptr inbounds [64 x i8], [64 x i8]* %l_metric1_generic, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 64, i8* nonnull %1) #2
  %2 = getelementptr inbounds [64 x i8], [64 x i8]* %l_path0_generic, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 64, i8* nonnull %2) #2
  %3 = getelementptr inbounds [64 x i8], [64 x i8]* %l_path1_generic, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 64, i8* nonnull %3) #2
  %4 = getelementptr inbounds [64 x i8], [64 x i8]* %l_mmresult, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 64, i8* nonnull %4) #2
  %5 = getelementptr inbounds [24 x [64 x i8]], [24 x [64 x i8]]* %l_ppresult, i64 0, i64 0, i64 0
  call void @llvm.lifetime.start.p0i8(i64 1536, i8* nonnull %5) #2
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %l_metric0_generic1184, i8 0, i64 64, i1 false)
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %l_path0_generic1172, i8 0, i64 64, i1 false)
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %l_metric1_generic1185, i8 0, i64 64, i1 false)
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %l_path1_generic1186, i8 0, i64 64, i1 false)
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %l_mmresult1187, i8 0, i64 64, i1 false)
  br label %for.body

while.cond.preheader:                             ; preds = %for.cond.cleanup16
  %sym0v1065 = getelementptr inbounds [16 x i8], [16 x i8]* %sym0v, i64 0, i64 0
  %sym1v1066 = getelementptr inbounds [16 x i8], [16 x i8]* %sym1v, i64 0, i64 0
  %arrayidx3 = getelementptr inbounds i8, i8* %inMemory, i64 72
  %cmp251052 = icmp sgt i32 %in_n_data_bits, 0
  br i1 %cmp251052, label %while.body.lr.ph, label %while.end

while.body.lr.ph:                                 ; preds = %while.cond.preheader
  %6 = getelementptr inbounds [16 x i8], [16 x i8]* %m0, i64 0, i64 0
  %7 = getelementptr inbounds [16 x i8], [16 x i8]* %m1, i64 0, i64 0
  %8 = getelementptr inbounds [16 x i8], [16 x i8]* %m2, i64 0, i64 0
  %9 = getelementptr inbounds [16 x i8], [16 x i8]* %m3, i64 0, i64 0
  %10 = getelementptr inbounds [16 x i8], [16 x i8]* %decision0, i64 0, i64 0
  %11 = getelementptr inbounds [16 x i8], [16 x i8]* %decision1, i64 0, i64 0
  %12 = getelementptr inbounds [16 x i8], [16 x i8]* %survivor0, i64 0, i64 0
  %13 = getelementptr inbounds [16 x i8], [16 x i8]* %survivor1, i64 0, i64 0
  %14 = getelementptr inbounds [16 x i8], [16 x i8]* %metsv, i64 0, i64 0
  %15 = getelementptr inbounds [16 x i8], [16 x i8]* %metsvm, i64 0, i64 0
  %16 = getelementptr inbounds [16 x i8], [16 x i8]* %shift0, i64 0, i64 0
  %17 = getelementptr inbounds [16 x i8], [16 x i8]* %shift1, i64 0, i64 0
  %18 = getelementptr inbounds [16 x i8], [16 x i8]* %tmp0, i64 0, i64 0
  %19 = getelementptr inbounds [16 x i8], [16 x i8]* %tmp1, i64 0, i64 0
  %20 = getelementptr inbounds [16 x i8], [16 x i8]* %sym0v, i64 0, i64 0
  %21 = getelementptr inbounds [16 x i8], [16 x i8]* %sym1v, i64 0, i64 0
  %cmp6421042 = icmp sgt i32 %in_ntraceback, 1
  %sub651 = add i32 %in_ntraceback, -1
  br label %while.body

for.body:                                         ; preds = %for.cond.cleanup16, %entry
  %indvars.iv1181 = phi i64 [ 0, %entry ], [ %indvars.iv.next1182, %for.cond.cleanup16 ]
  br label %for.body17

for.cond.cleanup16:                               ; preds = %for.body17
  %indvars.iv.next1182 = add nuw nsw i64 %indvars.iv1181, 1
  %exitcond1183 = icmp eq i64 %indvars.iv.next1182, 64
  br i1 %exitcond1183, label %while.cond.preheader, label %for.body

for.body17:                                       ; preds = %for.body17, %for.body
  %indvars.iv1178 = phi i64 [ 0, %for.body ], [ %indvars.iv.next1179, %for.body17 ]
  %arrayidx21 = getelementptr inbounds [24 x [64 x i8]], [24 x [64 x i8]]* %l_ppresult, i64 0, i64 %indvars.iv1178, i64 %indvars.iv1181
  store i8 0, i8* %arrayidx21, align 1, !tbaa !2
  %indvars.iv.next1179 = add nuw nsw i64 %indvars.iv1178, 1
  %exitcond1180 = icmp eq i64 %indvars.iv.next1179, 24
  br i1 %exitcond1180, label %for.cond.cleanup16, label %for.body17

while.body:                                       ; preds = %while.body.lr.ph, %if.end716
  %in_count.01058 = phi i32 [ 0, %while.body.lr.ph ], [ %inc717, %if.end716 ]
  %out_count.01056 = phi i32 [ 0, %while.body.lr.ph ], [ %out_count.1, %if.end716 ]
  %n_decoded.01055 = phi i32 [ 0, %while.body.lr.ph ], [ %n_decoded.3, %if.end716 ]
  %l_store_pos.01054 = phi i32 [ 0, %while.body.lr.ph ], [ %l_store_pos.1, %if.end716 ]
  %rem = and i32 %in_count.01058, 3
  %cmp26 = icmp eq i32 %rem, 0
  br i1 %cmp26, label %for.cond48.preheader, label %if.end716

for.cond48.preheader:                             ; preds = %while.body
  %and = and i32 %in_count.01058, 2147483644
  %idxprom30 = zext i32 %and to i64
  %arrayidx31 = getelementptr inbounds i8, i8* %arrayidx3, i64 %idxprom30
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %6) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %7) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %8) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %9) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %10) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %11) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %12) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %13) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %14) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %15) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %16) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %17) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %18) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %19) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %20) #2
  call void @llvm.lifetime.start.p0i8(i64 16, i8* nonnull %21) #2
  %22 = load i8, i8* %arrayidx31, align 1, !tbaa !2
  %arrayidx42 = getelementptr inbounds i8, i8* %arrayidx31, i64 1
  %23 = load i8, i8* %arrayidx42, align 1, !tbaa !2
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %sym0v1065, i8 %22, i64 16, i1 false)
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %sym1v1066, i8 %23, i64 16, i1 false)
  %arrayidx547 = getelementptr inbounds i8, i8* %arrayidx31, i64 2
  %24 = load i8, i8* %arrayidx547, align 1, !tbaa !2
  %arrayidx551 = getelementptr inbounds i8, i8* %arrayidx31, i64 3
  %25 = load i8, i8* %arrayidx551, align 1, !tbaa !2
  br label %for.cond53.preheader

for.cond53.preheader:                             ; preds = %for.cond.cleanup544, %for.cond48.preheader
  %s.01035 = phi i32 [ 0, %for.cond48.preheader ], [ %inc558, %for.cond.cleanup544 ]
  %second_symbol.01034 = phi i64 [ 1, %for.cond48.preheader ], [ 3, %for.cond.cleanup544 ]
  %first_symbol.01033 = phi i64 [ 0, %for.cond48.preheader ], [ 2, %for.cond.cleanup544 ]
  %path1.01032 = phi i8* [ %3, %for.cond48.preheader ], [ %2, %for.cond.cleanup544 ]
  %path0.01031 = phi i8* [ %2, %for.cond48.preheader ], [ %3, %for.cond.cleanup544 ]
  %metric1.01030 = phi i8* [ %1, %for.cond48.preheader ], [ %0, %for.cond.cleanup544 ]
  %metric0.01029 = phi i8* [ %0, %for.cond48.preheader ], [ %1, %for.cond.cleanup544 ]
  %arrayidx58 = getelementptr inbounds i8, i8* %arrayidx31, i64 %first_symbol.01033
  %26 = load i8, i8* %arrayidx58, align 1, !tbaa !2
  %cmp59 = icmp eq i8 %26, 2
  %arrayidx89 = getelementptr inbounds i8, i8* %arrayidx31, i64 %second_symbol.01034
  br label %for.body56

for.cond.cleanup50:                               ; preds = %for.cond.cleanup544
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %21) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %20) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %19) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %18) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %17) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %16) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %15) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %14) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %13) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %12) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %11) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %10) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %9) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %8) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %7) #2
  call void @llvm.lifetime.end.p0i8(i64 16, i8* nonnull %6) #2
  %rem563 = and i32 %in_count.01058, 15
  %cmp564 = icmp eq i32 %rem563, 8
  br i1 %cmp564, label %if.then566, label %if.end716

for.body56:                                       ; preds = %for.cond.cleanup513, %for.cond53.preheader
  %indvars.iv1133 = phi i64 [ 0, %for.cond53.preheader ], [ %indvars.iv.next1134, %for.cond.cleanup513 ]
  br i1 %cmp59, label %for.cond63.preheader, label %if.else

for.cond63.preheader:                             ; preds = %for.body56
  %27 = shl i64 %indvars.iv1133, 4
  br label %for.body67

for.body67:                                       ; preds = %for.body67, %for.cond63.preheader
  %indvars.iv1072 = phi i64 [ 0, %for.cond63.preheader ], [ %indvars.iv.next1073, %for.body67 ]
  %28 = add nuw nsw i64 %indvars.iv1072, %27
  %arrayidx71 = getelementptr inbounds i8, i8* %arrayidx1, i64 %28
  %29 = load i8, i8* %arrayidx71, align 1, !tbaa !2
  %arrayidx74 = getelementptr inbounds [16 x i8], [16 x i8]* %sym1v, i64 0, i64 %indvars.iv1072
  %30 = load i8, i8* %arrayidx74, align 1, !tbaa !2
  %xor1004 = xor i8 %30, %29
  %arrayidx78 = getelementptr inbounds [16 x i8], [16 x i8]* %metsvm, i64 0, i64 %indvars.iv1072
  store i8 %xor1004, i8* %arrayidx78, align 1, !tbaa !2
  %sub = sub i8 1, %xor1004
  %arrayidx84 = getelementptr inbounds [16 x i8], [16 x i8]* %metsv, i64 0, i64 %indvars.iv1072
  store i8 %sub, i8* %arrayidx84, align 1, !tbaa !2
  %indvars.iv.next1073 = add nuw nsw i64 %indvars.iv1072, 1
  %exitcond1075 = icmp eq i64 %indvars.iv.next1073, 16
  br i1 %exitcond1075, label %if.end164, label %for.body67

if.else:                                          ; preds = %for.body56
  %31 = load i8, i8* %arrayidx89, align 1, !tbaa !2
  %cmp91 = icmp eq i8 %31, 2
  %32 = shl i64 %indvars.iv1133, 4
  br i1 %cmp91, label %for.body99, label %for.body129

for.body99:                                       ; preds = %if.else, %for.body99
  %indvars.iv1068 = phi i64 [ %indvars.iv.next1069, %for.body99 ], [ 0, %if.else ]
  %33 = add nuw nsw i64 %indvars.iv1068, %32
  %arrayidx104 = getelementptr inbounds i8, i8* %inMemory, i64 %33
  %34 = load i8, i8* %arrayidx104, align 1, !tbaa !2
  %arrayidx107 = getelementptr inbounds [16 x i8], [16 x i8]* %sym0v, i64 0, i64 %indvars.iv1068
  %35 = load i8, i8* %arrayidx107, align 1, !tbaa !2
  %xor1091003 = xor i8 %35, %34
  %arrayidx112 = getelementptr inbounds [16 x i8], [16 x i8]* %metsvm, i64 0, i64 %indvars.iv1068
  store i8 %xor1091003, i8* %arrayidx112, align 1, !tbaa !2
  %sub116 = sub i8 1, %xor1091003
  %arrayidx119 = getelementptr inbounds [16 x i8], [16 x i8]* %metsv, i64 0, i64 %indvars.iv1068
  store i8 %sub116, i8* %arrayidx119, align 1, !tbaa !2
  %indvars.iv.next1069 = add nuw nsw i64 %indvars.iv1068, 1
  %exitcond1071 = icmp eq i64 %indvars.iv.next1069, 16
  br i1 %exitcond1071, label %if.end164, label %for.body99

for.body129:                                      ; preds = %if.else, %for.body129
  %indvars.iv = phi i64 [ %indvars.iv.next, %for.body129 ], [ 0, %if.else ]
  %36 = add nuw nsw i64 %indvars.iv, %32
  %arrayidx134 = getelementptr inbounds i8, i8* %inMemory, i64 %36
  %37 = load i8, i8* %arrayidx134, align 1, !tbaa !2
  %arrayidx137 = getelementptr inbounds [16 x i8], [16 x i8]* %sym0v, i64 0, i64 %indvars.iv
  %38 = load i8, i8* %arrayidx137, align 1, !tbaa !2
  %xor1391001 = xor i8 %38, %37
  %arrayidx144 = getelementptr inbounds i8, i8* %arrayidx1, i64 %36
  %39 = load i8, i8* %arrayidx144, align 1, !tbaa !2
  %arrayidx147 = getelementptr inbounds [16 x i8], [16 x i8]* %sym1v, i64 0, i64 %indvars.iv
  %40 = load i8, i8* %arrayidx147, align 1, !tbaa !2
  %xor1491002 = xor i8 %40, %39
  %add150 = add i8 %xor1491002, %xor1391001
  %arrayidx153 = getelementptr inbounds [16 x i8], [16 x i8]* %metsvm, i64 0, i64 %indvars.iv
  store i8 %add150, i8* %arrayidx153, align 1, !tbaa !2
  %sub157 = sub i8 2, %add150
  %arrayidx160 = getelementptr inbounds [16 x i8], [16 x i8]* %metsv, i64 0, i64 %indvars.iv
  store i8 %sub157, i8* %arrayidx160, align 1, !tbaa !2
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 16
  br i1 %exitcond, label %if.end164, label %for.body129

if.end164:                                        ; preds = %for.body129, %for.body99, %for.body67
  %41 = shl i64 %indvars.iv1133, 4
  br label %for.body170

for.body170:                                      ; preds = %for.body170, %if.end164
  %indvars.iv1076 = phi i64 [ 0, %if.end164 ], [ %indvars.iv.next1077, %for.body170 ]
  %42 = add nuw nsw i64 %indvars.iv1076, %41
  %arrayidx174 = getelementptr inbounds i8, i8* %metric0.01029, i64 %42
  %43 = load i8, i8* %arrayidx174, align 1, !tbaa !2
  %arrayidx177 = getelementptr inbounds [16 x i8], [16 x i8]* %metsv, i64 0, i64 %indvars.iv1076
  %44 = load i8, i8* %arrayidx177, align 1, !tbaa !2
  %add179 = add i8 %44, %43
  %arrayidx182 = getelementptr inbounds [16 x i8], [16 x i8]* %m0, i64 0, i64 %indvars.iv1076
  store i8 %add179, i8* %arrayidx182, align 1, !tbaa !2
  %45 = add nuw nsw i64 %indvars.iv1076, %41
  %46 = add nuw nsw i64 %45, 32
  %arrayidx187 = getelementptr inbounds i8, i8* %metric0.01029, i64 %46
  %47 = load i8, i8* %arrayidx187, align 1, !tbaa !2
  %arrayidx190 = getelementptr inbounds [16 x i8], [16 x i8]* %metsvm, i64 0, i64 %indvars.iv1076
  %48 = load i8, i8* %arrayidx190, align 1, !tbaa !2
  %add192 = add i8 %48, %47
  %arrayidx195 = getelementptr inbounds [16 x i8], [16 x i8]* %m1, i64 0, i64 %indvars.iv1076
  store i8 %add192, i8* %arrayidx195, align 1, !tbaa !2
  %add204 = add i8 %48, %43
  %arrayidx207 = getelementptr inbounds [16 x i8], [16 x i8]* %m2, i64 0, i64 %indvars.iv1076
  store i8 %add204, i8* %arrayidx207, align 1, !tbaa !2
  %add217 = add i8 %47, %44
  %arrayidx220 = getelementptr inbounds [16 x i8], [16 x i8]* %m3, i64 0, i64 %indvars.iv1076
  store i8 %add217, i8* %arrayidx220, align 1, !tbaa !2
  %indvars.iv.next1077 = add nuw nsw i64 %indvars.iv1076, 1
  %exitcond1081 = icmp eq i64 %indvars.iv.next1077, 16
  br i1 %exitcond1081, label %for.body229, label %for.body170

for.cond295.preheader:                            ; preds = %for.body229
  %49 = shl i64 %indvars.iv1133, 4
  %50 = add nuw nsw i64 %49, 32
  br label %for.body299

for.body229:                                      ; preds = %for.body170, %for.body229
  %indvars.iv1082 = phi i64 [ %indvars.iv.next1083, %for.body229 ], [ 0, %for.body170 ]
  %arrayidx231 = getelementptr inbounds [16 x i8], [16 x i8]* %m0, i64 0, i64 %indvars.iv1082
  %51 = load i8, i8* %arrayidx231, align 1, !tbaa !2
  %arrayidx234 = getelementptr inbounds [16 x i8], [16 x i8]* %m1, i64 0, i64 %indvars.iv1082
  %52 = load i8, i8* %arrayidx234, align 1, !tbaa !2
  %cmp237 = icmp ugt i8 %51, %52
  %conv239 = sext i1 %cmp237 to i8
  %arrayidx241 = getelementptr inbounds [16 x i8], [16 x i8]* %decision0, i64 0, i64 %indvars.iv1082
  store i8 %conv239, i8* %arrayidx241, align 1, !tbaa !2
  %arrayidx243 = getelementptr inbounds [16 x i8], [16 x i8]* %m2, i64 0, i64 %indvars.iv1082
  %53 = load i8, i8* %arrayidx243, align 1, !tbaa !2
  %arrayidx246 = getelementptr inbounds [16 x i8], [16 x i8]* %m3, i64 0, i64 %indvars.iv1082
  %54 = load i8, i8* %arrayidx246, align 1, !tbaa !2
  %cmp249 = icmp ugt i8 %53, %54
  %conv252 = sext i1 %cmp249 to i8
  %arrayidx254 = getelementptr inbounds [16 x i8], [16 x i8]* %decision1, i64 0, i64 %indvars.iv1082
  store i8 %conv252, i8* %arrayidx254, align 1, !tbaa !2
  %55 = select i1 %cmp237, i8 %51, i8 %52
  %arrayidx271 = getelementptr inbounds [16 x i8], [16 x i8]* %survivor0, i64 0, i64 %indvars.iv1082
  store i8 %55, i8* %arrayidx271, align 1, !tbaa !2
  %56 = select i1 %cmp249, i8 %53, i8 %54
  %arrayidx290 = getelementptr inbounds [16 x i8], [16 x i8]* %survivor1, i64 0, i64 %indvars.iv1082
  store i8 %56, i8* %arrayidx290, align 1, !tbaa !2
  %indvars.iv.next1083 = add nuw nsw i64 %indvars.iv1082, 1
  %exitcond1084 = icmp eq i64 %indvars.iv.next1083, 16
  br i1 %exitcond1084, label %for.cond295.preheader, label %for.body229

for.body299:                                      ; preds = %for.cond295.preheader, %for.body299
  %indvars.iv1085 = phi i64 [ 0, %for.cond295.preheader ], [ %indvars.iv.next1086, %for.body299 ]
  %57 = add nuw nsw i64 %indvars.iv1085, %49
  %arrayidx303 = getelementptr inbounds i8, i8* %path0.01031, i64 %57
  %58 = load i8, i8* %arrayidx303, align 1, !tbaa !2
  %59 = or i64 %indvars.iv1085, 1
  %60 = add nuw nsw i64 %59, %49
  %arrayidx309 = getelementptr inbounds i8, i8* %path0.01031, i64 %60
  %61 = load i8, i8* %arrayidx309, align 1, !tbaa !2
  %conv310 = zext i8 %61 to i16
  %shl = shl nuw i16 %conv310, 8
  %conv311 = zext i8 %58 to i16
  %or312 = or i16 %shl, %conv311
  %shl315 = shl i8 %58, 1
  %arrayidx319 = getelementptr inbounds [16 x i8], [16 x i8]* %shift0, i64 0, i64 %indvars.iv1085
  store i8 %shl315, i8* %arrayidx319, align 2, !tbaa !2
  %62 = lshr i16 %or312, 7
  %conv321 = trunc i16 %62 to i8
  %arrayidx324 = getelementptr inbounds [16 x i8], [16 x i8]* %shift0, i64 0, i64 %59
  store i8 %conv321, i8* %arrayidx324, align 1, !tbaa !2
  %63 = add nuw nsw i64 %indvars.iv1085, %50
  %arrayidx329 = getelementptr inbounds i8, i8* %path0.01031, i64 %63
  %64 = load i8, i8* %arrayidx329, align 1, !tbaa !2
  %65 = add nuw nsw i64 %59, %50
  %arrayidx336 = getelementptr inbounds i8, i8* %path0.01031, i64 %65
  %66 = load i8, i8* %arrayidx336, align 1, !tbaa !2
  %conv337 = zext i8 %66 to i16
  %shl338 = shl nuw i16 %conv337, 8
  %conv339 = zext i8 %64 to i16
  %or340 = or i16 %shl338, %conv339
  %shl343 = shl i8 %64, 1
  %arrayidx347 = getelementptr inbounds [16 x i8], [16 x i8]* %shift1, i64 0, i64 %indvars.iv1085
  store i8 %shl343, i8* %arrayidx347, align 2, !tbaa !2
  %67 = lshr i16 %or340, 7
  %conv350 = trunc i16 %67 to i8
  %arrayidx353 = getelementptr inbounds [16 x i8], [16 x i8]* %shift1, i64 0, i64 %59
  store i8 %conv350, i8* %arrayidx353, align 1, !tbaa !2
  %indvars.iv.next1086 = add nuw nsw i64 %indvars.iv1085, 2
  %cmp296 = icmp ult i64 %indvars.iv.next1086, 16
  br i1 %cmp296, label %for.body299, label %for.body362

for.cond374.preheader:                            ; preds = %for.body362
  %68 = shl nsw i64 %indvars.iv1133, 5
  br label %for.body378

for.body362:                                      ; preds = %for.body299, %for.body362
  %indvars.iv1092 = phi i64 [ %indvars.iv.next1093, %for.body362 ], [ 0, %for.body299 ]
  %arrayidx364 = getelementptr inbounds [16 x i8], [16 x i8]* %shift1, i64 0, i64 %indvars.iv1092
  %69 = load i8, i8* %arrayidx364, align 1, !tbaa !2
  %add366 = add i8 %69, 1
  store i8 %add366, i8* %arrayidx364, align 1, !tbaa !2
  %indvars.iv.next1093 = add nuw nsw i64 %indvars.iv1092, 1
  %exitcond1094 = icmp eq i64 %indvars.iv.next1093, 16
  br i1 %exitcond1094, label %for.cond374.preheader, label %for.body362

for.body378:                                      ; preds = %for.body378, %for.cond374.preheader
  %indvars.iv1097 = phi i64 [ 0, %for.cond374.preheader ], [ %indvars.iv.next1098, %for.body378 ]
  %indvars.iv1095 = phi i64 [ 0, %for.cond374.preheader ], [ %indvars.iv.next1096, %for.body378 ]
  %arrayidx380 = getelementptr inbounds [16 x i8], [16 x i8]* %survivor0, i64 0, i64 %indvars.iv1095
  %70 = load i8, i8* %arrayidx380, align 1, !tbaa !2
  %71 = add nuw nsw i64 %indvars.iv1097, %68
  %arrayidx385 = getelementptr inbounds i8, i8* %metric1.01030, i64 %71
  store i8 %70, i8* %arrayidx385, align 1, !tbaa !2
  %arrayidx387 = getelementptr inbounds [16 x i8], [16 x i8]* %survivor1, i64 0, i64 %indvars.iv1095
  %72 = load i8, i8* %arrayidx387, align 1, !tbaa !2
  %73 = or i64 %indvars.iv1097, 1
  %74 = add nuw nsw i64 %73, %68
  %arrayidx393 = getelementptr inbounds i8, i8* %metric1.01030, i64 %74
  store i8 %72, i8* %arrayidx393, align 1, !tbaa !2
  %indvars.iv.next1098 = add nuw nsw i64 %indvars.iv1097, 2
  %indvars.iv.next1096 = add nuw nsw i64 %indvars.iv1095, 1
  %exitcond1102 = icmp eq i64 %indvars.iv.next1096, 8
  br i1 %exitcond1102, label %for.body403, label %for.body378

for.cond428.preheader:                            ; preds = %for.body403
  %75 = shl i64 %indvars.iv1133, 5
  %76 = or i64 %75, 16
  br label %for.body432

for.body403:                                      ; preds = %for.body378, %for.body403
  %indvars.iv1103 = phi i64 [ %indvars.iv.next1104, %for.body403 ], [ 0, %for.body378 ]
  %arrayidx405 = getelementptr inbounds [16 x i8], [16 x i8]* %decision0, i64 0, i64 %indvars.iv1103
  %77 = load i8, i8* %arrayidx405, align 1, !tbaa !2
  %arrayidx408 = getelementptr inbounds [16 x i8], [16 x i8]* %shift0, i64 0, i64 %indvars.iv1103
  %78 = load i8, i8* %arrayidx408, align 1, !tbaa !2
  %and410998 = and i8 %78, %77
  %neg414 = xor i8 %77, -1
  %arrayidx416 = getelementptr inbounds [16 x i8], [16 x i8]* %shift1, i64 0, i64 %indvars.iv1103
  %79 = load i8, i8* %arrayidx416, align 1, !tbaa !2
  %and418 = and i8 %79, %neg414
  %or419 = or i8 %and418, %and410998
  %arrayidx422 = getelementptr inbounds [16 x i8], [16 x i8]* %tmp0, i64 0, i64 %indvars.iv1103
  store i8 %or419, i8* %arrayidx422, align 1, !tbaa !2
  %indvars.iv.next1104 = add nuw nsw i64 %indvars.iv1103, 1
  %exitcond1105 = icmp eq i64 %indvars.iv.next1104, 16
  br i1 %exitcond1105, label %for.cond428.preheader, label %for.body403

for.body432:                                      ; preds = %for.body432, %for.cond428.preheader
  %indvars.iv1108 = phi i64 [ 0, %for.cond428.preheader ], [ %indvars.iv.next1109, %for.body432 ]
  %indvars.iv1106 = phi i64 [ 8, %for.cond428.preheader ], [ %indvars.iv.next1107, %for.body432 ]
  %arrayidx434 = getelementptr inbounds [16 x i8], [16 x i8]* %survivor0, i64 0, i64 %indvars.iv1106
  %80 = load i8, i8* %arrayidx434, align 1, !tbaa !2
  %81 = add nuw nsw i64 %indvars.iv1108, %76
  %arrayidx440 = getelementptr inbounds i8, i8* %metric1.01030, i64 %81
  store i8 %80, i8* %arrayidx440, align 1, !tbaa !2
  %arrayidx442 = getelementptr inbounds [16 x i8], [16 x i8]* %survivor1, i64 0, i64 %indvars.iv1106
  %82 = load i8, i8* %arrayidx442, align 1, !tbaa !2
  %83 = or i64 %indvars.iv1108, 1
  %84 = add nuw nsw i64 %83, %76
  %arrayidx449 = getelementptr inbounds i8, i8* %metric1.01030, i64 %84
  store i8 %82, i8* %arrayidx449, align 1, !tbaa !2
  %indvars.iv.next1109 = add nuw nsw i64 %indvars.iv1108, 2
  %indvars.iv.next1107 = add nuw nsw i64 %indvars.iv1106, 1
  %exitcond1113 = icmp eq i64 %indvars.iv.next1107, 16
  br i1 %exitcond1113, label %for.body459, label %for.body432

for.cond484.preheader:                            ; preds = %for.body459
  %85 = shl nsw i64 %indvars.iv1133, 5
  br label %for.body488

for.body459:                                      ; preds = %for.body432, %for.body459
  %indvars.iv1114 = phi i64 [ %indvars.iv.next1115, %for.body459 ], [ 0, %for.body432 ]
  %arrayidx461 = getelementptr inbounds [16 x i8], [16 x i8]* %decision1, i64 0, i64 %indvars.iv1114
  %86 = load i8, i8* %arrayidx461, align 1, !tbaa !2
  %arrayidx464 = getelementptr inbounds [16 x i8], [16 x i8]* %shift0, i64 0, i64 %indvars.iv1114
  %87 = load i8, i8* %arrayidx464, align 1, !tbaa !2
  %and466997 = and i8 %87, %86
  %neg470 = xor i8 %86, -1
  %arrayidx472 = getelementptr inbounds [16 x i8], [16 x i8]* %shift1, i64 0, i64 %indvars.iv1114
  %88 = load i8, i8* %arrayidx472, align 1, !tbaa !2
  %and474 = and i8 %88, %neg470
  %or475 = or i8 %and474, %and466997
  %arrayidx478 = getelementptr inbounds [16 x i8], [16 x i8]* %tmp1, i64 0, i64 %indvars.iv1114
  store i8 %or475, i8* %arrayidx478, align 1, !tbaa !2
  %indvars.iv.next1115 = add nuw nsw i64 %indvars.iv1114, 1
  %exitcond1116 = icmp eq i64 %indvars.iv.next1115, 16
  br i1 %exitcond1116, label %for.cond484.preheader, label %for.body459

for.cond510.preheader:                            ; preds = %for.body488
  %89 = shl i64 %indvars.iv1133, 5
  %90 = or i64 %89, 16
  br label %for.body514

for.body488:                                      ; preds = %for.body488, %for.cond484.preheader
  %indvars.iv1119 = phi i64 [ 0, %for.cond484.preheader ], [ %indvars.iv.next1120, %for.body488 ]
  %indvars.iv1117 = phi i64 [ 0, %for.cond484.preheader ], [ %indvars.iv.next1118, %for.body488 ]
  %arrayidx490 = getelementptr inbounds [16 x i8], [16 x i8]* %tmp0, i64 0, i64 %indvars.iv1117
  %91 = load i8, i8* %arrayidx490, align 1, !tbaa !2
  %92 = add nuw nsw i64 %indvars.iv1119, %85
  %arrayidx495 = getelementptr inbounds i8, i8* %path1.01032, i64 %92
  store i8 %91, i8* %arrayidx495, align 1, !tbaa !2
  %arrayidx497 = getelementptr inbounds [16 x i8], [16 x i8]* %tmp1, i64 0, i64 %indvars.iv1117
  %93 = load i8, i8* %arrayidx497, align 1, !tbaa !2
  %94 = or i64 %indvars.iv1119, 1
  %95 = add nuw nsw i64 %94, %85
  %arrayidx503 = getelementptr inbounds i8, i8* %path1.01032, i64 %95
  store i8 %93, i8* %arrayidx503, align 1, !tbaa !2
  %indvars.iv.next1120 = add nuw nsw i64 %indvars.iv1119, 2
  %indvars.iv.next1118 = add nuw nsw i64 %indvars.iv1117, 1
  %exitcond1124 = icmp eq i64 %indvars.iv.next1118, 8
  br i1 %exitcond1124, label %for.cond510.preheader, label %for.body488

for.cond.cleanup513:                              ; preds = %for.body514
  %indvars.iv.next1134 = add nuw nsw i64 %indvars.iv1133, 1
  %exitcond1146 = icmp eq i64 %indvars.iv.next1134, 2
  br i1 %exitcond1146, label %for.cond.cleanup544, label %for.body56

for.body514:                                      ; preds = %for.body514, %for.cond510.preheader
  %indvars.iv1127 = phi i64 [ 0, %for.cond510.preheader ], [ %indvars.iv.next1128, %for.body514 ]
  %indvars.iv1125 = phi i64 [ 8, %for.cond510.preheader ], [ %indvars.iv.next1126, %for.body514 ]
  %arrayidx516 = getelementptr inbounds [16 x i8], [16 x i8]* %tmp0, i64 0, i64 %indvars.iv1125
  %96 = load i8, i8* %arrayidx516, align 1, !tbaa !2
  %97 = add nuw nsw i64 %indvars.iv1127, %90
  %arrayidx522 = getelementptr inbounds i8, i8* %path1.01032, i64 %97
  store i8 %96, i8* %arrayidx522, align 1, !tbaa !2
  %arrayidx524 = getelementptr inbounds [16 x i8], [16 x i8]* %tmp1, i64 0, i64 %indvars.iv1125
  %98 = load i8, i8* %arrayidx524, align 1, !tbaa !2
  %99 = or i64 %indvars.iv1127, 1
  %100 = add nuw nsw i64 %99, %90
  %arrayidx531 = getelementptr inbounds i8, i8* %path1.01032, i64 %100
  store i8 %98, i8* %arrayidx531, align 1, !tbaa !2
  %indvars.iv.next1128 = add nuw nsw i64 %indvars.iv1127, 2
  %indvars.iv.next1126 = add nuw nsw i64 %indvars.iv1125, 1
  %exitcond1132 = icmp eq i64 %indvars.iv.next1126, 16
  br i1 %exitcond1132, label %for.cond.cleanup513, label %for.body514

for.cond.cleanup544:                              ; preds = %for.cond.cleanup513
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %sym0v1065, i8 %24, i64 16, i1 false)
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %sym1v1066, i8 %25, i64 16, i1 false)
  %inc558 = add nuw nsw i32 %s.01035, 1
  %exitcond1150 = icmp eq i32 %inc558, 2
  br i1 %exitcond1150, label %for.cond.cleanup50, label %for.cond53.preheader

if.then566:                                       ; preds = %for.cond.cleanup50
  %add573 = add nsw i32 %l_store_pos.01054, 1
  %rem574 = srem i32 %add573, %in_ntraceback
  %idxprom595 = sext i32 %rem574 to i64
  br label %for.inc604

for.inc604:                                       ; preds = %if.then566, %for.inc604
  %indvar = phi i64 [ 0, %if.then566 ], [ %indvar.next, %for.inc604 ]
  %101 = shl nuw nsw i64 %indvar, 4
  %scevgep1156 = getelementptr [24 x [64 x i8]], [24 x [64 x i8]]* %l_ppresult, i64 0, i64 %idxprom595, i64 %101
  %scevgep1157 = getelementptr [64 x i8], [64 x i8]* %l_path0_generic, i64 0, i64 %101
  %102 = shl nuw nsw i64 %indvar, 4
  %scevgep = getelementptr [64 x i8], [64 x i8]* %l_mmresult, i64 0, i64 %102
  %scevgep1155 = getelementptr [64 x i8], [64 x i8]* %l_metric0_generic, i64 0, i64 %102
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep, i8* align 16 %scevgep1155, i64 16, i1 false)
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep1156, i8* align 16 %scevgep1157, i64 16, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond1158 = icmp eq i64 %indvar.next, 4
  br i1 %exitcond1158, label %for.end606, label %for.inc604

for.end606:                                       ; preds = %for.inc604
  %103 = load i8, i8* %4, align 16, !tbaa !2
  %conv609 = zext i8 %103 to i32
  br label %for.body616

for.cond640.preheader:                            ; preds = %for.body616
  %idxprom6451043 = sext i32 %rem574 to i64
  %idxprom6471044 = sext i32 %spec.select1005 to i64
  %arrayidx6481045 = getelementptr inbounds [24 x [64 x i8]], [24 x [64 x i8]]* %l_ppresult, i64 0, i64 %idxprom6451043, i64 %idxprom6471044
  %104 = load i8, i8* %arrayidx6481045, align 1, !tbaa !2
  br i1 %cmp6421042, label %for.body644, label %for.cond661.preheader

for.body616:                                      ; preds = %for.body616, %for.end606
  %indvars.iv1159 = phi i64 [ 1, %for.end606 ], [ %indvars.iv.next1160, %for.body616 ]
  %beststate.01041 = phi i32 [ 0, %for.end606 ], [ %spec.select1005, %for.body616 ]
  %minmetric.01040 = phi i32 [ %conv609, %for.end606 ], [ %minmetric.1, %for.body616 ]
  %bestmetric.01039 = phi i32 [ %conv609, %for.end606 ], [ %spec.select, %for.body616 ]
  %arrayidx618 = getelementptr inbounds [64 x i8], [64 x i8]* %l_mmresult, i64 0, i64 %indvars.iv1159
  %105 = load i8, i8* %arrayidx618, align 1, !tbaa !2
  %conv619 = zext i8 %105 to i32
  %cmp620 = icmp ult i32 %bestmetric.01039, %conv619
  %spec.select = select i1 %cmp620, i32 %conv619, i32 %bestmetric.01039
  %106 = trunc i64 %indvars.iv1159 to i32
  %spec.select1005 = select i1 %cmp620, i32 %106, i32 %beststate.01041
  %cmp630 = icmp sgt i32 %minmetric.01040, %conv619
  %minmetric.1 = select i1 %cmp630, i32 %conv619, i32 %minmetric.01040
  %indvars.iv.next1160 = add nuw nsw i64 %indvars.iv1159, 1
  %exitcond1161 = icmp eq i64 %indvars.iv.next1160, 64
  br i1 %exitcond1161, label %for.cond640.preheader, label %for.body616

for.cond661.preheader:                            ; preds = %for.body644, %for.cond640.preheader
  %.lcssa = phi i8 [ %104, %for.cond640.preheader ], [ %110, %for.body644 ]
  %107 = trunc i32 %minmetric.1 to i8
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %l_path0_generic1172, i8 0, i64 64, i1 false)
  br label %for.cond665.preheader

for.body644:                                      ; preds = %for.cond640.preheader, %for.body644
  %108 = phi i8 [ %110, %for.body644 ], [ %104, %for.cond640.preheader ]
  %pos.01047 = phi i32 [ %rem653, %for.body644 ], [ %rem574, %for.cond640.preheader ]
  %i571.21046 = phi i32 [ %inc655, %for.body644 ], [ 0, %for.cond640.preheader ]
  %109 = lshr i8 %108, 2
  %add652 = add i32 %sub651, %pos.01047
  %rem653 = srem i32 %add652, %in_ntraceback
  %inc655 = add nuw nsw i32 %i571.21046, 1
  %idxprom645 = sext i32 %rem653 to i64
  %idxprom647 = zext i8 %109 to i64
  %arrayidx648 = getelementptr inbounds [24 x [64 x i8]], [24 x [64 x i8]]* %l_ppresult, i64 0, i64 %idxprom645, i64 %idxprom647
  %110 = load i8, i8* %arrayidx648, align 1, !tbaa !2
  %exitcond1162 = icmp eq i32 %inc655, %sub651
  br i1 %exitcond1162, label %for.cond661.preheader, label %for.body644

for.cond665.preheader:                            ; preds = %for.inc687, %for.cond661.preheader
  %indvar1167 = phi i64 [ 0, %for.cond661.preheader ], [ %indvar.next1168, %for.inc687 ]
  %111 = shl i64 %indvar1167, 4
  br label %for.body668

for.body668:                                      ; preds = %for.body668, %for.cond665.preheader
  %indvars.iv1163 = phi i64 [ 0, %for.cond665.preheader ], [ %indvars.iv.next1164, %for.body668 ]
  %112 = add nuw nsw i64 %indvars.iv1163, %111
  %arrayidx676 = getelementptr inbounds [64 x i8], [64 x i8]* %l_metric0_generic, i64 0, i64 %112
  %113 = load i8, i8* %arrayidx676, align 1, !tbaa !2
  %conv679 = sub i8 %113, %107
  store i8 %conv679, i8* %arrayidx676, align 1, !tbaa !2
  %indvars.iv.next1164 = add nuw nsw i64 %indvars.iv1163, 1
  %exitcond1166 = icmp eq i64 %indvars.iv.next1164, 16
  br i1 %exitcond1166, label %for.inc687, label %for.body668

for.inc687:                                       ; preds = %for.body668
  %indvar.next1168 = add nuw nsw i64 %indvar1167, 1
  %exitcond1171 = icmp eq i64 %indvar.next1168, 4
  br i1 %exitcond1171, label %for.end689, label %for.cond665.preheader

for.end689:                                       ; preds = %for.inc687
  %cmp690 = icmp slt i32 %out_count.01056, %in_ntraceback
  br i1 %cmp690, label %if.end713, label %for.cond694.preheader

for.cond694.preheader:                            ; preds = %for.end689
  %conv699 = zext i8 %.lcssa to i32
  %sub704 = sub nsw i32 %out_count.01056, %in_ntraceback
  %mul705 = shl i32 %sub704, 3
  %114 = sext i32 %mul705 to i64
  br label %for.body698

for.body698:                                      ; preds = %for.body698, %for.cond694.preheader
  %indvars.iv1173 = phi i64 [ 0, %for.cond694.preheader ], [ %indvars.iv.next1174, %for.body698 ]
  %115 = trunc i64 %indvars.iv1173 to i32
  %116 = sub i32 7, %115
  %shr701 = lshr i32 %conv699, %116
  %117 = trunc i32 %shr701 to i8
  %conv703 = and i8 %117, 1
  %118 = add nuw nsw i64 %indvars.iv1173, %114
  %arrayidx708 = getelementptr inbounds i8, i8* %outMemory, i64 %118
  store i8 %conv703, i8* %arrayidx708, align 1, !tbaa !2
  %indvars.iv.next1174 = add nuw nsw i64 %indvars.iv1173, 1
  %exitcond1177 = icmp eq i64 %indvars.iv.next1174, 8
  br i1 %exitcond1177, label %if.end713.loopexit, label %for.body698

if.end713.loopexit:                               ; preds = %for.body698
  %119 = add i32 %n_decoded.01055, 8
  br label %if.end713

if.end713:                                        ; preds = %if.end713.loopexit, %for.end689
  %n_decoded.2 = phi i32 [ %n_decoded.01055, %for.end689 ], [ %119, %if.end713.loopexit ]
  %inc714 = add nsw i32 %out_count.01056, 1
  br label %if.end716

if.end716:                                        ; preds = %for.cond.cleanup50, %if.end713, %while.body
  %l_store_pos.1 = phi i32 [ %rem574, %if.end713 ], [ %l_store_pos.01054, %for.cond.cleanup50 ], [ %l_store_pos.01054, %while.body ]
  %n_decoded.3 = phi i32 [ %n_decoded.2, %if.end713 ], [ %n_decoded.01055, %for.cond.cleanup50 ], [ %n_decoded.01055, %while.body ]
  %out_count.1 = phi i32 [ %inc714, %if.end713 ], [ %out_count.01056, %for.cond.cleanup50 ], [ %out_count.01056, %while.body ]
  %inc717 = add nuw nsw i32 %in_count.01058, 1
  %cmp25 = icmp slt i32 %n_decoded.3, %in_n_data_bits
  br i1 %cmp25, label %while.body, label %while.end

while.end:                                        ; preds = %if.end716, %while.cond.preheader
  call void @llvm.lifetime.end.p0i8(i64 1536, i8* nonnull %5) #2
  call void @llvm.lifetime.end.p0i8(i64 64, i8* nonnull %4) #2
  call void @llvm.lifetime.end.p0i8(i64 64, i8* nonnull %3) #2
  call void @llvm.lifetime.end.p0i8(i64 64, i8* nonnull %2) #2
  call void @llvm.lifetime.end.p0i8(i64 64, i8* nonnull %1) #2
  call void @llvm.lifetime.end.p0i8(i64 64, i8* nonnull %0) #2
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.start.p0i8(i64 immarg, i8* nocapture) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.lifetime.end.p0i8(i64 immarg, i8* nocapture) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg) #1

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1 immarg) #1

attributes #0 = { nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { argmemonly nounwind }
attributes #2 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 9.0.0 (git@gitlab.engr.illinois.edu:llvm/hpvm.git a9f09dd1d7c769b6e9bef9dca334a9c0761f2136)"}
!2 = !{!3, !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C/C++ TBAA"}
