#pragma once
#include <map>
#include <vector>
using namespace std;
#define MAX_EDGE_NUM    (2000 * 20)
struct Edge
{
	int outVertex;      // 出边
	int capicity;       //边的带宽
	int cost;           // 单位流量的花费值
	int capCopy;        //带宽值的拷贝，用来重置
	Edge *reverseEdge;        // 指向反向边，方便求最小费用最大流时更新反向边信息
	Edge(){}
	Edge(int outVertex,int capicity,int cost){
		this->outVertex=outVertex;
		this->capicity=capicity;
		this->cost=cost;
		this->reverseEdge=NULL;
		this->capCopy=capicity;
	}
	Edge(const Edge & edge){
		this->outVertex=edge.outVertex;
		this->capicity=edge.capicity;
		this->cost=edge.cost;
		this->reverseEdge=edge.reverseEdge;
		this->capCopy=edge.capCopy;
	}
};

class Graph  // 对私有变量不进行私有化封装，方便外部访问
{
public:
	int netNodeNum;                       //网络节点个数
	int edgeNum;                          // 无向边个数
	int consumerEdgeNum;                  // 消费节点个数
	int serverTypeNum;                    //服务器类型数
	int *serverSuply;                     //每种类型服务器最大提供的流
	int *serverCost;                      //每种服务器的成本
	int *deployCost;                      //在网络节点i 部署视频服务器需要的价格

	vector<int> maxOutFlow;             //最多能输出的流,节点旁边的边的流量限制
	vector<vector<Edge*>* > outEdge;    //存储边和节点信息，第一个参数表示顶点，后边Edge 表示出边
	//map<int,Edge>  consumerEdge;        //与消费节点直连的边，第一个为链路节点边编号，第二个为消费节点边信息，cost=0
	vector<Edge*> consumerEdge;           //空间换时间，和上面作用一样
	int *consumerIdx;                   //消费节点对应的网络节点
	bool *isConnectWithConsumer;        //链路节点是否与消费节点直接相连

	vector<vector<int> > net2consumer;         // 网络节点如果是视频服务器， 适合服务的消费节点

	Graph();
	void readLine(char* line,int *d,int n);       //从一行中读取n个整数，并保存在d中
	Graph(char * topo[MAX_EDGE_NUM],int line_num);

	
	void print();                                 //打印相关信息，调试用
	void addEdge(int inV,int outV,int u,int c);  //加双向边，u:带宽  c:费用
	void addEdge(int inV,int outV,int u);        //加单向边  u:带宽
	void printFlow();
	void dijkstra(int s,vector<int> &distance);    //一个节点到其它点距离，不记录路径，主要用来求每个非与消费节点直连网络节点-->与消费节点直连网络节点 距离
	void SPFA(int s,vector<int>&distance);//使用SPFA算法求解最短距离
	void getNet2ConmerByOrder(int k);         //通过次序统计获取 net2consumer 变量的值
	void copyG(vector<vector<Edge*>* > &bestg);
	void reset();
	~Graph();
};

