define i32 @main(){
B22:
  %t31 = alloca i32, align 4
  %t24 = alloca i32, align 4
  %t23 = alloca i32, align 4
  store i32 0, i32* %t23, align 4
  store i32 0, i32* %t24, align 4
  br label %B25
B25:                               	; preds = %B22, %B34
  %t2 = load i32, i32* %t24, align 4
  %t3 = icmp slt i32 %t2, 10
  br i1 %t3, label %B26, label %B29
B26:                               	; preds = %B25
  store i32 10, i32* %t31, align 4
  br label %B32
B29:                               	; preds = %B25
  br label %B27
B32:                               	; preds = %B26, %B42
  %t5 = load i32, i32* %t31, align 4
  %t38 = icmp ne i32 %t5, 0
  br i1 %t38, label %B35, label %B36
B27:                               	; preds = %B29
  ret i32 0
B35:                               	; preds = %B32
  %t39 = icmp ne i32 1, 0
  br i1 %t39, label %B33, label %B36
B36:                               	; preds = %B32, %B35
  br label %B34
B33:                               	; preds = %B35
  %t6 = load i32, i32* %t24, align 4
  %t46 = icmp ne i32 %t6, 0
  br i1 %t46, label %B43, label %B44
B34:                               	; preds = %B36
  %t19 = load i32, i32* %t24, align 4
  %t20 = add i32 %t19, 1
  store i32 %t20, i32* %t24, align 4
  br label %B25
B43:                               	; preds = %B33
  %t7 = load i32, i32* %t31, align 4
  %t47 = icmp ne i32 %t7, 0
  br i1 %t47, label %B41, label %B44
B44:                               	; preds = %B33, %B43
  br label %B42
B41:                               	; preds = %B43
  %t10 = load i32, i32* %t23, align 4
  %t11 = load i32, i32* %t24, align 4
  %t12 = add i32 %t10, %t11
  %t13 = load i32, i32* %t31, align 4
  %t14 = add i32 %t12, %t13
  store i32 %t14, i32* %t23, align 4
  br label %B42
B42:                               	; preds = %B41, %B44
  %t16 = load i32, i32* %t31, align 4
  %t17 = sub i32 %t16, 1
  store i32 %t17, i32* %t31, align 4
  br label %B32
}
