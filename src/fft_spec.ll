; ModuleID = 'fft_spec.c'
source_filename = "fft_spec.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nofree nounwind uwtable
define dso_local void @fft(float* nocapture %data, i32 %N, i32 %logn, i32 %sign) local_unnamed_addr #0 {
entry:
  tail call fastcc void @bit_reverse(float* %data, i32 %N, i32 %logn)
  %cmp146 = icmp eq i32 %logn, 0
  br i1 %cmp146, label %for.end77, label %for.body.lr.ph

for.body.lr.ph:                                   ; preds = %entry
  %conv = sitofp i32 %sign to double
  %mul1 = fmul double %conv, 0x400921FB54442D18
  %cmp22139 = icmp eq i32 %N, 0
  br label %for.body

for.body:                                         ; preds = %for.end73, %for.body.lr.ph
  %transform_length.0148 = phi i32 [ 1, %for.body.lr.ph ], [ %mul74, %for.end73 ]
  %bit.0147 = phi i32 [ 0, %for.body.lr.ph ], [ %inc76, %for.end73 ]
  %conv2 = uitofp i32 %transform_length.0148 to float
  %conv3 = fpext float %conv2 to double
  %div = fdiv double %mul1, %conv3
  %conv4 = fptrunc double %div to float
  %conv5 = fpext float %conv4 to double
  %call6 = tail call double @sin(double %conv5) #4
  %conv7 = fptrunc double %call6 to float
  %mul9 = fmul double %conv5, 5.000000e-01
  %call10 = tail call double @sin(double %mul9) #4
  %conv11 = fptrunc double %call10 to float
  %conv12 = fpext float %conv11 to double
  %mul13 = fmul double %conv12, 2.000000e+00
  %mul15 = fmul double %mul13, %conv12
  %conv16 = fptrunc double %mul15 to float
  %cmp18141 = icmp eq i32 %transform_length.0148, 0
  br i1 %cmp18141, label %for.end73, label %for.cond21.preheader.lr.ph

for.cond21.preheader.lr.ph:                       ; preds = %for.body
  %mul62 = shl i32 %transform_length.0148, 1
  br label %for.cond21.preheader

for.cond21.preheader:                             ; preds = %for.end, %for.cond21.preheader.lr.ph
  %a.0144 = phi i32 [ 0, %for.cond21.preheader.lr.ph ], [ %inc, %for.end ]
  %w_imag.0143 = phi float [ 0.000000e+00, %for.cond21.preheader.lr.ph ], [ %add71, %for.end ]
  %w_real.0142 = phi float [ 1.000000e+00, %for.cond21.preheader.lr.ph ], [ %sub67, %for.end ]
  br i1 %cmp22139, label %for.end, label %for.body24

for.body24:                                       ; preds = %for.cond21.preheader, %for.body24
  %b.0140 = phi i32 [ %add63, %for.body24 ], [ 0, %for.cond21.preheader ]
  %add = add i32 %b.0140, %a.0144
  %add26 = add i32 %add, %transform_length.0148
  %mul27 = shl i32 %add26, 1
  %idxprom = zext i32 %mul27 to i64
  %arrayidx = getelementptr inbounds float, float* %data, i64 %idxprom
  %0 = load float, float* %arrayidx, align 4, !tbaa !2
  %add29 = or i32 %mul27, 1
  %idxprom30 = zext i32 %add29 to i64
  %arrayidx31 = getelementptr inbounds float, float* %data, i64 %idxprom30
  %1 = load float, float* %arrayidx31, align 4, !tbaa !2
  %mul32 = fmul float %w_real.0142, %0
  %mul33 = fmul float %w_imag.0143, %1
  %sub = fsub float %mul32, %mul33
  %mul34 = fmul float %w_real.0142, %1
  %mul35 = fmul float %w_imag.0143, %0
  %add36 = fadd float %mul35, %mul34
  %mul37 = shl i32 %add, 1
  %idxprom38 = zext i32 %mul37 to i64
  %arrayidx39 = getelementptr inbounds float, float* %data, i64 %idxprom38
  %2 = load float, float* %arrayidx39, align 4, !tbaa !2
  %sub40 = fsub float %2, %sub
  store float %sub40, float* %arrayidx, align 4, !tbaa !2
  %add45 = or i32 %mul37, 1
  %idxprom46 = zext i32 %add45 to i64
  %arrayidx47 = getelementptr inbounds float, float* %data, i64 %idxprom46
  %3 = load float, float* %arrayidx47, align 4, !tbaa !2
  %sub48 = fsub float %3, %add36
  store float %sub48, float* %arrayidx31, align 4, !tbaa !2
  %4 = load float, float* %arrayidx39, align 4, !tbaa !2
  %add56 = fadd float %sub, %4
  store float %add56, float* %arrayidx39, align 4, !tbaa !2
  %5 = load float, float* %arrayidx47, align 4, !tbaa !2
  %add61 = fadd float %add36, %5
  store float %add61, float* %arrayidx47, align 4, !tbaa !2
  %add63 = add i32 %b.0140, %mul62
  %cmp22 = icmp ult i32 %add63, %N
  br i1 %cmp22, label %for.body24, label %for.end

for.end:                                          ; preds = %for.body24, %for.cond21.preheader
  %mul64 = fmul float %w_imag.0143, %conv7
  %mul65 = fmul float %w_real.0142, %conv16
  %add66 = fadd float %mul65, %mul64
  %sub67 = fsub float %w_real.0142, %add66
  %mul68 = fmul float %w_real.0142, %conv7
  %mul69 = fmul float %w_imag.0143, %conv16
  %sub70 = fsub float %mul68, %mul69
  %add71 = fadd float %w_imag.0143, %sub70
  %inc = add nuw i32 %a.0144, 1
  %exitcond = icmp eq i32 %inc, %transform_length.0148
  br i1 %exitcond, label %for.end73, label %for.cond21.preheader

for.end73:                                        ; preds = %for.end, %for.body
  %mul74 = shl i32 %transform_length.0148, 1
  %inc76 = add nuw i32 %bit.0147, 1
  %exitcond150 = icmp eq i32 %inc76, %logn
  br i1 %exitcond150, label %for.end77, label %for.body

for.end77:                                        ; preds = %for.end73, %entry
  ret void
}

; Function Attrs: nofree norecurse nounwind uwtable
define internal fastcc void @bit_reverse(float* nocapture %w, i32 %N, i32 %bits) unnamed_addr #1 {
entry:
  %add = sub i32 32, %bits
  %cmp2 = icmp eq i32 %N, 0
  br i1 %cmp2, label %for.end, label %for.body.preheader

for.body.preheader:                               ; preds = %entry
  %wide.trip.count = zext i32 %N to i64
  br label %for.body

for.body:                                         ; preds = %if.end, %for.body.preheader
  %indvars.iv = phi i64 [ 0, %for.body.preheader ], [ %indvars.iv.next, %if.end ]
  %0 = trunc i64 %indvars.iv to i32
  %call = tail call fastcc i32 @_rev(i32 %0)
  %shr = lshr i32 %call, %add
  %1 = zext i32 %shr to i64
  %cmp1 = icmp ult i64 %indvars.iv, %1
  br i1 %cmp1, label %if.then, label %if.end

if.then:                                          ; preds = %for.body
  %2 = trunc i64 %indvars.iv to i32
  %mul = shl i32 %2, 1
  %idxprom = zext i32 %mul to i64
  %arrayidx = getelementptr inbounds float, float* %w, i64 %idxprom
  %3 = bitcast float* %arrayidx to i32*
  %4 = load i32, i32* %3, align 4, !tbaa !2
  %add3 = or i32 %mul, 1
  %idxprom4 = zext i32 %add3 to i64
  %arrayidx5 = getelementptr inbounds float, float* %w, i64 %idxprom4
  %5 = bitcast float* %arrayidx5 to i32*
  %6 = load i32, i32* %5, align 4, !tbaa !2
  %mul6 = shl i32 %shr, 1
  %idxprom7 = zext i32 %mul6 to i64
  %arrayidx8 = getelementptr inbounds float, float* %w, i64 %idxprom7
  %7 = bitcast float* %arrayidx8 to i32*
  %8 = load i32, i32* %7, align 4, !tbaa !2
  store i32 %8, i32* %3, align 4, !tbaa !2
  %add13 = or i32 %mul6, 1
  %idxprom14 = zext i32 %add13 to i64
  %arrayidx15 = getelementptr inbounds float, float* %w, i64 %idxprom14
  %9 = bitcast float* %arrayidx15 to i32*
  %10 = load i32, i32* %9, align 4, !tbaa !2
  store i32 %10, i32* %5, align 4, !tbaa !2
  store i32 %4, i32* %7, align 4, !tbaa !2
  store i32 %6, i32* %9, align 4, !tbaa !2
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, %wide.trip.count
  br i1 %exitcond, label %for.end, label %for.body

for.end:                                          ; preds = %if.end, %entry
  ret void
}

; Function Attrs: nofree nounwind
declare dso_local double @sin(double) local_unnamed_addr #2

; Function Attrs: norecurse nounwind readnone uwtable
define internal fastcc i32 @_rev(i32 %v) unnamed_addr #3 {
entry:
  %v.addr.013 = lshr i32 %v, 1
  %tobool14 = icmp eq i32 %v.addr.013, 0
  br i1 %tobool14, label %for.end, label %for.body

for.body:                                         ; preds = %entry, %for.body
  %v.addr.017 = phi i32 [ %v.addr.0, %for.body ], [ %v.addr.013, %entry ]
  %s.016 = phi i32 [ %dec, %for.body ], [ 31, %entry ]
  %r.015 = phi i32 [ %or, %for.body ], [ %v, %entry ]
  %shl = shl i32 %r.015, 1
  %and = and i32 %v.addr.017, 1
  %or = or i32 %shl, %and
  %dec = add nsw i32 %s.016, -1
  %v.addr.0 = lshr i32 %v.addr.017, 1
  %tobool = icmp eq i32 %v.addr.0, 0
  br i1 %tobool, label %for.end, label %for.body

for.end:                                          ; preds = %for.body, %entry
  %r.0.lcssa = phi i32 [ %v, %entry ], [ %or, %for.body ]
  %s.0.lcssa = phi i32 [ 31, %entry ], [ %dec, %for.body ]
  %shl2 = shl i32 %r.0.lcssa, %s.0.lcssa
  ret i32 %shl2
}

attributes #0 = { nofree nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nofree norecurse nounwind uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nofree nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { norecurse nounwind readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 9.0.0 (git@gitlab.engr.illinois.edu:llvm/hpvm.git a9f09dd1d7c769b6e9bef9dca334a9c0761f2136)"}
!2 = !{!3, !3, i64 0}
!3 = !{!"float", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
