#pragma once
#include <vector>
#include <set>
#include <map>
#include <ctime>
#include "Graph.h"
#include "sortHelper.h"

using namespace std;

class IterationServer
{
public:
	Graph *g;                       //保存图相关的信息
	Graph *bestg;
	vector<int> bestSolution;      // 当前最好解
	vector<int> serverType;        //最好解对应的服务器位置
	vector<int> bestinflow;
	vector<int> bestflow;
	int bestCost;                  //最优解对应的最小的总费用
	unsigned long beginTime;
	int iterCount;                 //多少次迭代后计时一次
	int iterMax;                   //迭代多少次计算一下时间
	int iterTime;
	less<sortHelper<int> >compare;
	IterationServer();
	IterationServer(Graph *g);
	~IterationServer();

	unsigned long getRunTime();     //已经运行的毫秒数
	unsigned long getmillitm();

	int runFlow(vector<int> &cur,vector<int>& serverType,vector<int> &flow,vector<int>& inFlow,map<int,int>&demand);
	bool getInitSolutionAndW2(vector<int>& w2);
	bool getInitSolution(vector<int> &w2,int *skip); //通过删除一些节点获取一个较好的初始解
	int exchangeAndAdd(const vector<int> &sol,const vector<int> &flow);   //通过交换和添加获取更好的解
	
	int del(const vector<int> &initsol,vector<int> &flow,int*skip);           //删除，从非直连点开始,如果得到更好解返回流
	bool searchNeighborhood(vector<int> &cur,vector<int> &others,vector<int>& flow); //搜索邻域中的最好解
	bool searchForBetter(vector<int> &cur,vector<int> &others,vector<int>& flow); //搜索邻域中的一个较好解
	void getSortedSolAndO(vector<int>& sol,vector<int>&others,const vector<int> &w2,const vector<int> &flow,bool*in);//将当前解排序，并得到与其交换的others集
	const char* iteration();
	const char* iteration2();
	void addTrajectory(vector<vector<int> >&  Trajectory,int pos,int beginID,vector<Edge *>&  Tra,vector<int>&inflow);
	const char *showTrajectory();
	int minCostFlowSPFA(vector<int> &serverLoaction,vector<int>& serverType,vector<int> &flow,vector<int>& inFlow, vector<vector<Edge*>* >& edgeGraph,map<int,int>&demand);
	void keepg();
	void printSolution(const vector<int>&sol,const vector<int>&serverType,vector<int> &inFlow,int*skip);
	void sortByFlow(vector<int> &sol,vector<int>&flow,vector<sortHelper<int> >&sorts); //按流量进行排序
	int runAndSearchServerType(vector<int>&sol,vector<int> &inFlow,vector<int>& oldServerType,vector<int>& newServerType,int curCost); //调整流量，贪心算法，每次降低一个服务器的级别


	void getServerTypeByFlow(const vector<int>&inFlow,vector<int> &serverType);
	bool adjustServerType(const vector<int>&sol,vector<int> inFlow,const vector<int>& oldServerType,vector<int>& newServerType); //将那些很多流量剩余的服务器降级
	int Aug(int beginIdx,int flow,const vector<int>&dis,vector<vector<Edge*>* > &outEdge, bool*isVisit,map<int,int> &demand);
	bool Aug(const vector<int>& sol,const vector<int>& initSupply,vector<int>& supply,const vector<int>&dis,vector<vector<Edge*>* > &outEdge, bool*isVisit,map<int,int> &demand); //zkw 尝试增光函数
	bool modlabel(vector<int> &dis,vector<vector<Edge*>* > &outEdge,bool*isVisit);
	int minCostFlowZKW(vector<int> &serverLoaction,vector<int>& serverType,vector<int> &flow,vector<int>& inFlow, vector<vector<Edge*>* >& edgeGraph,map<int,int>&demand);
};

