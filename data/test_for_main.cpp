
#include <stdio.h>

#define CHECK_CUDA_ERROR_RETURN(err, msg)      if (err != CUDA_SUCCESS) { printf("error = %d\n", err); return -1; }

#define DEFAULT_A           (10)
#define DEFAULT_B        (10)

using namespace std;

typedef struct _ST_AAAA_
{
	unsigned char *a;
	unsigned int b;
	long long c;
}ST_AAAA;


class A
{
public:
	int m_a;

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
	}unsigned long long * set(int a) //函数头不换行
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
		return 0;
	}

	virtual unsigned int get2(int &a) = 0; //纯虚函数
};


//-----这是单行注释-----------
class B : public A
{
public:
	int m_b;

public:
	B();
	~B();
	unsigned int get2(int &a);
	ST_AAAA returnClass(int a); //返回值为C++类
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


/*
这是多行注释
这是多行注释
*/
int main(int argc, char *argv[])
{
	return 0;//这是单行注释
}
