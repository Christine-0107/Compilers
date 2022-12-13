declare void @putint(i32)
define i32 @main(){
B24:
  %t26 = alloca i32, align 4
  %t25 = alloca i32, align 4
  store i32 56, i32* %t25, align 4
  store i32 4, i32* %t26, align 4
  %t5 = load i32, i32* %t25, align 4
  %t27 = sub i32 0, 4
  %t7 = sub i32 %t5, %t27
  %t8 = load i32, i32* %t26, align 4
  %t9 = add i32 %t7, %t8
  store i32 %t9, i32* %t25, align 4
  %t10 = load i32, i32* %t25, align 4
  %t31 = icmp ne i32 %t10, 0
  %t11 = xor i1 %t31, true
  %t12 = xor i1 %t11, true
  %t13 = xor i1 %t12, true
  %t32 = zext i1 %t13 to i32
  %t33 = sub i32 0, %t32
  %t34 = icmp ne i32 %t33, 0
  br i1 %t34, label %B28, label %B29
B28:                               	; preds = %B24
  %t35 = sub i32 0, 1
  %t36 = sub i32 0, %t35
  %t37 = sub i32 0, %t36
  store i32 %t37, i32* %t25, align 4
  br label %B30
B29:                               	; preds = %B24
  %t20 = load i32, i32* %t26, align 4
  %t21 = add i32 0, %t20
  store i32 %t21, i32* %t25, align 4
  br label %B30
B30:                               	; preds = %B28, %B29
  %t22 = load i32, i32* %t25, align 4
  call void @putint(i32 %t22)
  ret i32 0
}
