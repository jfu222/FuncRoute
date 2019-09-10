#include <stdio.h>

#define CHECK_CUDA_ERROR_RETURN(err, msg)      if (err != CUDA_SUCCESS) { printf("error = %d\n", err); return -1; }

#define DEFAULT_A           (10)
#define DEFAULT_B        (10)

#define RETURN_IF_FAILED(condition, ret)                                                      \
do                                                                                        \
{                                                                                         \
if (condition)                                                                        \
{                                                                                     \
	printf("%s(%d): %s: Error: ret=%d;\n", __FILE__, __LINE__, __FUNCTION__, ret);    \
	return ret;                                                                       \
}                                                                                     \
} while (0)


#define AA(x)    #@x         //对单字符加单引号,例如：A(x) 表示 'x',A(abcd)则无效
#define BB(x)    #x          //加双引号，即将x转换成字符串，例如：B(hello)，表示 "hello"
#define CC(x)    hello##x    //把标识符连接起来，犹如胶水一样，将两个单词粘起来，例如：C(_world)，表示hello_world


using namespace std;

typedef struct _ST_AAAA_
{
	unsigned char *a;
public:
	unsigned int b;
private:
	long long c;

public:
	int set_ABC()
	{
		return -2;
	}
}ST_AAAA;


class A
{
public:
	int m_a;
	ST_AAAA m_st_aaaa;

public:
	A(){}
	~A(){}

	unsigned int set(int a)
	{
		m_a = a;
		if (a > 0)
		{
			printf("INFO: aaaabbbb;\n");
		}
		return 0;
	}unsigned long long * set(int a, int a2) //函数头不换行
	{
		m_a = a;
		if (a > 0)
		{
			printf("INFO: aaaabbbb;\n");
		}
		return 0;
	}

	virtual unsigned int get(int &a) const
	{
		a = m_a;
		unsigned int ret = A::init();
		return 0;
	}

	virtual unsigned int get2(int &a) = 0; //纯虚函数
	static unsigned int init()
	{
		printf("INFO: void init();\n");
		return 0;
	}
};


//-----这是单行注释-----------
class B : public A
{
public:
	int m_b;
	static int m_b2;

public:
	B();
	~B();
	unsigned int get2(int &a);
	ST_AAAA returnClass(int a); //返回值为C++类
	static int init();
};


/*这是多行注释，
构造函数没有返回值*/B::B(/*这是多行注释，构造函数不需要任何参数*/)
{

}

B::~B()
{

}


unsigned int //这是单行注释，返回值单独一行
B::get2(int &a)
{
	int ret = m_st_aaaa.set_ABC();
	return -1;
}

/* 这是多行注释
A B::returnClass(int a)
{
return -1;
}
*/

ST_AAAA /*这是多行注释，返回值
		单独一行*/
		B::returnClass(int a) /*这是多行注释，返回值为C++类
							  static B::returnClass(int *a) /* 这是多行注释 * /{ //这是单行注释
							  }
							  改行开头是两个制表符(Tab键)*/
{
			ST_AAAA st_a;
			return st_a;
		}

int B::init()
{
	if (m_b2 == 1)
	{
		printf("%s: yes\n", __FUNCTION__);
	}
	return -2;
}

/*
这是多行注释
这是多行注释
*/
int main(int argc, char *argv[])
{
	int ret = 0;
	B classB;
	B * pClassB2 = new B;

	ret = B::init(); //测试"::"调用函数

	ret = B
		::
		init(
		)
		; //测试"::"调用函数

	if (ret != 0)
	{
		printf("%s: Error: ret=%d;\n", __FUNCTION__, ret);
	}

	ret = classB.set(1); //测试"."调用函数
	ret = classB.set(2, 34); //测试"."调用函数
	if (ret != 0)
	{
		printf("%s: Error: ret=%d;\n", __FUNCTION__, ret);
	}

	ret = pClassB2->set(3); //测试"->"调用函数
	ret = pClassB2
		->
		set(  4
		)
		; //测试"->"调用函数

	if (ret != 0)
	{
		printf("%s: Error: ret=%d;\n", __FUNCTION__, ret);
	}

	return 0;//这是单行注释
}
