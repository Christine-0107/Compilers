; 输出字符串
@outputstr = private unnamed_addr constant [8 x i8] c"res:%f\0A\00", align 1
; 常量BOUND
@BOUND = dso_local constant float 5.000000e+00, align 4
; 全局变量count
@count = dso_local global i32 0, align 4

; init函数
define dso_local void @init([4 x float]* %0) #0 {
  ; 数组a
  %2 = alloca [4 x float]*, align 8
  ; it 
  %3 = alloca float, align 4
  ; i
  %4 = alloca i32, align 4
  ; j
  %5 = alloca i32, align 4
  store [4 x float]* %0, [4 x float]** %2, align 8
  ; it = 0.0
  store float 0.000000e+00, float* %3, align 4
  ; i = 0
  store i32 0, i32* %4, align 4
  br label %6

; preds = %1
; 第一层while循环循环条件
6:
  ; i值存储在%4
  %7 = load i32, i32* %4, align 4
  ; i<4结果存储在%8
  %8 = icmp slt i32 %7, 4
  ; 若i<4则执行第一层while循环循环体，否则跳出循环
  br i1 %8, label %9, label %37

; preds = %6
; if语句判断条件
9:
  ; i值存储在%10
  %10 = load i32, i32* %4, align 4
  ; i<0结果存储在%11
  %11 = icmp slt i32 %10, 0
  ; 若i<0则执行if体，否则不执行if体
  br i1 %11, label %12, label %13

; preds = %9
12:
  ; break
  br label %37

; preds = %12
13:
  ; j=0
  store i32 0, i32* %5, align 4
  ; 执行第二层while循环条件判断
  br label %14

; preds = %13
; 第二层while循环条件判断
14:
  %15 = load i32, i32* %5, align 4
  ; j<4结果存储在%16
  %16 = icmp slt i32 %15, 4
  ; 若j<4则执行第二层while循环循环体，否则跳出循环
  br i1 %16, label %17, label %34

; preds=%14
; if语句判断条件
17:
  %18 = load i32, i32* %5, align 4
  ; j<0结果存储在%19
  %19 = icmp slt i32 %18, 0
  ; 若j<0则执行if体，否则不执行if体
  br i1 %19, label %20, label %21

; preds=%17
20:
; break
  br label %34

; preds=%17
21:
  ; 加载it到%22
  %22 = load float, float* %3, align 4
  ; a
  %23 = load [4 x float]*, [4 x float]** %2, align 8
  ; i
  %24 = load i32, i32* %4, align 4
  ; 类型转换
  %25 = sext i32 %24 to i64
  ; 获取a[i]指针存储在%26
  %26 = getelementptr inbounds [4 x float], [4 x float]* %23, i64 %25
  ; j
  %27 = load i32, i32* %5, align 4
  ; 类型转换
  %28 = sext i32 %27 to i64
  ; 获取a[i][j]指针存储在%29
  %29 = getelementptr inbounds [4 x float], [4 x float]* %26, i64 0, i64 %28
  ; a[i][j]=it
  store float %22, float* %29, align 4
  ; 加载it到%30
  %30 = load float, float* %3, align 4
  ; it=it+1
  %31 = fadd float %30, 1.000000e+00
  ; 将中间结果存储到it
  store float %31, float* %3, align 4
  ; 加载j到%32
  %32 = load i32, i32* %5, align 4
  ; j=j+1
  %33 = add nsw i32 %32, 1
  ; 将中间结果存储到j
  store i32 %33, i32* %5, align 4
  ; 执行内层while循环下一轮
  br label %14

34:
  ; 加载i到%35
  %35 = load i32, i32* %4, align 4
  ; i=i+1
  %36 = add nsw i32 %35, 1
  ; 将中间结果存储到i
  store i32 %36, i32* %4, align 4
  ; 执行外层while循环下一轮
  br label %6

37:
  ret void
}

; arraysummation函数
define dso_local float @arraysummation([4 x float]* %0, i32 %1) #0 {
  ; 开辟临时寄存器%3存放返回值
  %3 = alloca float, align 4
  ; %4存数组a
  %4 = alloca [4 x float]*, align 8
  ; %5存flag
  %5 = alloca i32, align 4
  ; %6存sum变量
  %6 = alloca float, align 4
  ; %7存变量i
  %7 = alloca i32, align 4
  ; %8存变量j
  %8 = alloca i32, align 4
  ; 将传来的数组参数保存到%4中
  store [4 x float]* %0, [4 x float]** %4, align 8
  ; 将传来的flag参数保存到%5中
  store i32 %1, i32* %5, align 4
  ; 将flag的值保存到%9中，后续进行比较
  %9 = load i32, i32* %5, align 4
  ; 比较flag和0是否相等，若相等%10为真
  %10 = icmp eq i32 %9, 0
  ; 判断%10是否为真，若为真执行label11，否则执行label12
  br i1 %10, label %11, label %12

; preds = %2
; if(flag==0)语句体
11:                                               
  ; 将0.0存入%3
  store float 0.000000e+00, float* %3, align 4
  br label %53

; preds = %2
; 继续执行arraysummation函数
12:                                               
  ; 为sum变量赋值，sum为float类型，直接将0转为0.0
  store float 0.000000e+00, float* %6, align 4
  ; 为变量i赋值为0
  store i32 0, i32* %7, align 4
  br label %13

; preds = %12
; preds = %48（继续循环）
; 外层while的判断条件
13:                                               
  ; %14保存i
  %14 = load i32, i32* %7, align 4
  ; 比较i是否小于4，若i<4则%15为真
  %15 = icmp slt i32 %14, 4
  ; 若i<4执行label16，否则执行label51
  br i1 %15, label %16, label %51

; preds = %13
; 给j赋值
16:                                               
  ; 给j赋值为0
  store i32 0, i32* %8, align 4
  br label %17

; preds = %16（赋值）
; preds = %30（continue）
; preds = %33（内层执行完，循环继续）
; 进行内层while条件判断
17:                                               
  ; 将j赋值给%18
  %18 = load i32, i32* %8, align 4
  ; 判断j是否小于4
  %19 = icmp slt i32 %18, 4
  ; 若j<4，执行label20，否则执行label%48
  br i1 %19, label %20, label %48

; preds = %17
; 执行内层循环
20:
  ; %21存数组a                                               
  %21 = load [4 x float]*, [4 x float]** %4, align 8
  ; %22存i
  %22 = load i32, i32* %7, align 4
  ; 类型转换
  %23 = sext i32 %22 to i64
  ; 将a[i]指针存储在%24
  %24 = getelementptr inbounds [4 x float], [4 x float]* %21, i64 %23
  ; %25存j
  %25 = load i32, i32* %8, align 4
  ; 类型转换
  %26 = sext i32 %25 to i64
  ; 将a[i][j]指针存%27
  %27 = getelementptr inbounds [4 x float], [4 x float]* %24, i64 0, i64 %26
  ; 使用%28保存a[i][j]，进行后续比较
  %28 = load float, float* %27, align 4
  ; 执行float类型的比较，比较a[i][j]和常量BOUND的大小
  %29 = fcmp ole float %28, 5.000000e+00
  ; 若a[i][j]<=BOUND，执行label30，否则执行label33
  br i1 %29, label %30, label %33

; preds = %20
; 执行if(a[i][j]<=BOUND)语句体内容
30:                                               
  ; %31存j
  %31 = load i32, i32* %8, align 4
  ; %32存j+1的值
  %32 = add nsw i32 %31, 1
  ; 将j+1存回j
  store i32 %32, i32* %8, align 4
  ; continue语句，继续执行内层循环，回到label17
  br label %17

; preds = %20
; 继续执行内层循环
33:                                               
  ; %34存变量sum
  %34 = load float, float* %6, align 4
  ; %35存数组a
  %35 = load [4 x float]*, [4 x float]** %4, align 8
  ; %36存i
  %36 = load i32, i32* %7, align 4
  ; 对i进行类型转换，存到%37中
  %37 = sext i32 %36 to i64
  ; %38存a[i]指针
  %38 = getelementptr inbounds [4 x float], [4 x float]* %35, i64 %37
  ; %39存j
  %39 = load i32, i32* %8, align 4
  ; 对j进行类型转换，存到%40中
  %40 = sext i32 %39 to i64
  ; %41存a[i][j]指针
  %41 = getelementptr inbounds [4 x float], [4 x float]* %38, i64 0, i64 %40
  ; %42存a[i][j]
  %42 = load float, float* %41, align 4
  ; 进行浮点数加法，将sum和a[i][j]相加，结果存到%43中
  %43 = fadd float %34, %42
  ; 将加法结果存回sum中
  store float %43, float* %6, align 4
  ; %44存全局变量count
  %44 = load i32, i32* @count, align 4
  ; 将count+1的结果存到%45中
  %45 = add nsw i32 %44, 1
  ; 将count+1的结果存回count变量
  store i32 %45, i32* @count, align 4
  ; %46存j
  %46 = load i32, i32* %8, align 4
  ; %47存j+1的结果
  %47 = add nsw i32 %46, 1
  ; 将j+1的结果存回j
  store i32 %47, i32* %8, align 4
  ; 进行循环
  br label %17

; preds = %17
; 内层条件不满足，结束内层循环。执行i=i+1
48:                                               
  ; %49存i
  %49 = load i32, i32* %7, align 4
  ; %50存i+1结果
  %50 = add nsw i32 %49, 1
  ; 将i+1的结果存回i
  store i32 %50, i32* %7, align 4
  ; 进行外层循环
  br label %13

; preds = %13
; 外层循环条件不满足，结束循环
51:                                               
  ; %52存sum
  %52 = load float, float* %6, align 4
  ; 将返回值存到%3
  store float %52, float* %3, align 4
  br label %53

; preds = %11（return 0.0）
; preds = %51（return sum）
53:                                 
  %54 = load float, float* %3, align 4
  ret float %54
}

; main函数
define dso_local i32 @main() #0 {
  ; 数组a,size=4*4
  %1 = alloca [4 x [4 x float]], align 16
  ; sum
  %2 = alloca float, align 4
  ; res
  %3 = alloca float, align 4
  ; 将指针转为i8*
  %4 = bitcast [4 x [4 x float]]* %1 to i8*
  ; 调用memset初始化内存为0
  call void @llvm.memset.p0i8.i64(i8* align 16 %4, i8 0, i64 64, i1 false)
  ; 获得a[0][0]地址
  %5 = getelementptr inbounds [4 x [4 x float]], [4 x [4 x float]]* %1, i64 0, i64 0
  ; 调用init函数
  call void @init([4 x float]* %5)
  ; 获得a[0][0]地址
  %6 = getelementptr inbounds [4 x [4 x float]], [4 x [4 x float]]* %1, i64 0, i64 0
  ; 调用arraysummation函数，结果储存在%7
  %7 = call float @arraysummation([4 x float]* %6, i32 1)
  ; sum结果存在%2
  store float %7, float* %2, align 4
  ; res=0.0
  store float 0.000000e+00, float* %3, align 4
  ; %8存储count
  %8 = load i32, i32* @count, align 4
  ; %9存储count>0的判断结果
  %9 = icmp sgt i32 %8, 0
  ; count<=0则直接执行return 0,否则进行下一步判断(或短路运算)
  br i1 %9, label %10, label %18

; preds = %0
; if语句中count<=16条件判断
10:  
  ; %11存储count
  %11 = load i32, i32* @count, align 4
  ; %12存储count<=16的判断结果
  %12 = icmp sle i32 %11, 16
  ; count<16则直接执行return 0,否则进行if体内操作
  br i1 %12, label %13, label %18

; preds = %10
; if体内操作
13:     
  ; sum 
  %14 = load float, float* %2, align 4
  ; %15存储count
  %15 = load i32, i32* @count, align 4
  ; 类型转换int->float
  %16 = sitofp i32 %15 to float
  ;res = sum / count
  %17 = fdiv float %14, %16
  ;将中间结果保存到res中
  store float %17, float* %3, align 4
  br label %18

;preds = %0,%10,%13
; 程序结束
18:    
  %19 = load float, float* %3, align 4
  %20 = fpext float %19 to double
  %21 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([8 x i8], [8 x i8]* @outputstr, i64 0, i64 0), double %20)
  ; return 0
  ret i32 0
}

declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1 immarg) #1
declare dso_local i32 @printf(i8*, ...) #2