//常量声明
const float BOUND = 5.0;
//全局变量
int count = 0;
/*
	函数sum
	参数类型为float数组及flag标识
	返回值为float
*/
void init(float a[][4])
{
	float it=0.0;
	int i=0,j;
	while(i<4)
	{
		if(i<0)
		{
			break;
		}
		j=0;
		while(j<4)
		{
			if(j<0)
			{
				break;
			}
			a[i][j]=it;
			it = it + 1;
			j = j + 1;
		}
		i = i + 1;
	}
}
float arraysummation(float a[][4],int flag)
{
	//if语句
	if(flag==0)
		return 0.0;
	//隐式类型转换
	float sum=0;
	//变量声明，i初始化，j未进行初始化
	int i=0,j;
	//while语句
	while(i<4)
	{
		j=0;
		while(j<4)
		{
			if(a[i][j]<=BOUND)
			{
				j = j + 1;
				//continue语句
				continue;
			}
			sum = sum + a[i][j];
			count = count + 1;
			j = j + 1;
		}
		i = i + 1;
	}
	//return 语句
	return sum;
}
int main()
{
	//多维数组初始化
	float a[4][4]={};
	init(a);
	//调用函数
	float sum=arraysummation(a,1);
	float res = 0.0;
	if((count>0)&&(count<=16))
		res = sum / count;
	return 0;
}