#include "FuncRoute2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "CommonData.h"



//---------_FUNC_INDEX_------------------------
_FUNC_INDEX_::_FUNC_INDEX_()
{

}


_FUNC_INDEX_::~_FUNC_INDEX_()
{

}


bool _FUNC_INDEX_::isRecursiveFunction(int funcIndex) //是否是递归函数
{
	std::vector<_FUNC_INDEX_ *> stack;
	std::vector<int> stackIndexes;
	stack.push_back(this);

	while (stack.empty() == false)
	{
		_FUNC_INDEX_ *node = stack.front();

		stack.erase(stack.begin());

		int len1 = node->parentIndexs.size();

		if (len1 == 0)
		{
			return false;
		}

		for (int i = 0; i < len1; ++i)
		{
			if ((node != this && node->funcIndex == funcIndex) || node->parentIndexs[i]->funcIndex == funcIndex)
			{
				return true;
			}

			int len2 = stackIndexes.size();
			int flag = 0;
			for (int j = 0; j < len2; ++j)
			{
				if (stackIndexes[j] == node->parentIndexs[i]->funcIndex)
				{
					flag = 1;
					break;
				}
			}

			if (flag == 0)
			{
				stackIndexes.push_back(node->parentIndexs[i]->funcIndex);
				stack.push_back(node->parentIndexs[i]);
			}
		}
	}

	return false;
}


int _FUNC_INDEX_::freeMem()
{
	int len1 = childrenIndexs.size();
	for (int i = 0; i < len1; ++i)
	{
		if (childrenIndexs[i])
		{
			childrenIndexs[i]->freeMem();
			free(childrenIndexs[i]);
			childrenIndexs[i] = NULL;
		}
	}
	return 0;
}


int _FUNC_INDEX_::printInfo()
{
	int ret = 0;

	std::vector<_FUNC_INDEX_ *> stack;
	std::vector<int> stackParent;
	std::vector<int> stackDepth;
	std::vector<std::vector<_FUNC_INDEX_ *>> stackUncles;
	std::vector<std::vector<_FUNC_INDEX_ *>> stackUnclesLast;
	std::vector<_FUNC_INDEX_ *> stackTemp;
	int parentCnt = 1;

	//-----------多叉树按层次遍历----------------------
	//先遍历第一层根节点层，再接着遍历第二层，第三层
	//注意：跟二叉树的 前序遍历，中序遍历，后序遍历 是不一样的
	printf("==========\n");
	stack.push_back(this);
	stackParent.push_back(parentCnt++);
	stackDepth.push_back(1);

	int lastI = 0;
	int lastJ = 0;
	int flag = 0;

	while (stack.empty() == false)
	{
		_FUNC_INDEX_ *node = stack.front();
		int parentIndex = stackParent.front();
		int depth = stackDepth.front();

		stack.erase(stack.begin());
		stackParent.erase(stackParent.begin());
		stackDepth.erase(stackDepth.begin());

		flag = 0;
		for (int i = lastI; i < stackUnclesLast.size(); ++i)
		{
			for (int j = lastJ; j < stackUnclesLast[i].size(); ++j)
			{
				if (stackUnclesLast[i][j]->funcIndex == parentIndex)
				{
					lastI = i;
					lastJ = j;
					flag = 1;
					break;
				}
			}

			if (flag == 1)
			{
				break;
			}
			lastJ = 0;
		}

		node->treeDepth = depth;
		node->lastI = lastI + 1;
		node->lastJ = lastJ + 1;
		node->parentIndexTemp = parentIndex;

		if (stack.empty() == true)
		{
			printf("%d(%d;%d/%d)\n", node->funcIndex, parentIndex, lastI + 1, lastJ + 1); //换行

			lastI = 0;
			lastJ = 0;
			stackTemp.push_back(node);
			stackUncles.push_back(stackTemp);
			stackTemp.clear();
			stackUnclesLast = stackUncles;
			stackUncles.clear();
		}
		else
		{
			int depth2 = stackDepth[0];

			//---------------------
			if (parentIndex == depth2) //共同的父节点
			{
				printf("%d-", node->funcIndex);
				stackTemp.push_back(node);
			}
			else //非共同的父节点
			{
				if (depth == depth2) //深度相等
				{
					printf("%d(%d;%d/%d) ", node->funcIndex, parentIndex, lastI + 1, lastJ + 1);

					stackTemp.push_back(node);
					stackUncles.push_back(stackTemp);
					stackTemp.clear();
				}
				else
				{
					printf("%d(%d;%d/%d)\n", node->funcIndex, parentIndex, lastI + 1, lastJ + 1); //换行

					lastI = 0;
					lastJ = 0;
					stackTemp.push_back(node);
					stackUncles.push_back(stackTemp);
					stackTemp.clear();
					stackUnclesLast = stackUncles;
					stackUncles.clear();
				}
			}
		}

		bool bRet = node->isRecursiveFunction(node->funcIndex);
		if (bRet == false) //不是递归函数
		{
			int len = node->childrenIndexs.size();
			for (int i = 0; i < len; ++i)
			{
				stack.push_back(node->childrenIndexs[i]);
				stackParent.push_back(node->funcIndex);
				stackDepth.push_back(depth + 1);
			}
		}
	}
	printf("\n");
	return 0;
}


int _FUNC_INDEX_::printInfoFuncRoute(std::vector<_FUNC_INDEX_ *> &funcs)
{
	int ret = 0;

	int len1 = this->childrenIndexs.size();
	int len2 = funcs.size();

	bool bRet = this->isRecursiveFunction(this->funcIndex);

	if (len1 == 0 || bRet == true)
	{
		if (len2 != 0)
		{
			printf("[");
			for (int i = 0; i < len2; ++i)
			{
				if (i == len2 - 1)
				{
					printf("%d", funcs[i]->funcIndex);
				}
				else
				{
					printf("%d-", funcs[i]->funcIndex);
				}
			}
			printf("]\n");
		}
		else
		{
			printf("[%d]\n", this->funcIndex);
		}
	}
	else
	{
		if (bRet == false)
		{
			for (int i = 0; i < len1; ++i)
			{
				funcs.push_back(this->childrenIndexs[i]);
				this->childrenIndexs[i]->printInfoFuncRoute(funcs);
				funcs.pop_back();
			}
		}
	}

	return 0;
}
