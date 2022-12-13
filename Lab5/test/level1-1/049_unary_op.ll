define i32 @main(){
B13:
  %t14 = alloca i32, align 4
  store i32 10, i32* %t14, align 4
  %t2 = load i32, i32* %t14, align 4
  %t18 = icmp ne i32 %t2, 0
  %t3 = xor i1 %t18, true
  %t4 = xor i1 %t3, true
  %t5 = xor i1 %t4, true
  %t19 = zext i1 %t5 to i32
  %t20 = sub i32 0, %t19
  %t21 = icmp ne i32 %t20, 0
  br i1 %t21, label %B15, label %B16
B15:                               	; preds = %B13
  %t22 = sub i32 0, 1
  %t23 = sub i32 0, %t22
  %t24 = sub i32 0, %t23
  store i32 %t24, i32* %t14, align 4
  br label %B17
B16:                               	; preds = %B13
  store i32 0, i32* %t14, align 4
  br label %B17
B17:                               	; preds = %B15, %B16
  %t12 = load i32, i32* %t14, align 4
  ret i32 %t12
}
