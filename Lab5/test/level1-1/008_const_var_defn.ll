@a = dso_local global i32 10
define i32 @main(){
B2:
  %t1 = load i32, i32* @a, align 4
  ret i32 %t1
}
