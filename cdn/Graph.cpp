#include "Graph.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <queue>

using namespace std;

Graph::Graph(){
	consumerIdx=nullptr;
	isConnectWithConsumer=nullptr;
}

void Graph::readLine(char* line,int *d,int n)
{
	int j=0,k=0;
	fill(d,d+n,0);
	while(true){
		while(line[j]>='0'&&line[j]<='9'){
			d[k]=d[k]*10+line[j]-'0';
			j+=1;
		}
		while(line[j]==' ')
			j+=1;
		k+=1;
		if(k==n)
			break;
	}
}

Graph::Graph(char * topo[MAX_EDGE_NUM],int line_num)
{
	int i,j,n;
	char *line=topo[0];
	int tmp[4];
	readLine(line,tmp,3);  //获取网络节点数量,边的数量，消费节点个数
	this->netNodeNum=tmp[0];
	this->edgeNum=tmp[1];
	this->consumerEdgeNum=tmp[2];
	isConnectWithConsumer=new bool[netNodeNum];
	consumerIdx=new int[consumerEdgeNum];
	outEdge.resize(this->netNodeNum);
	consumerEdge.resize(this->netNodeNum);
	for(i=0;i<this->netNodeNum;i++)
	{
		isConnectWithConsumer[i]=false;
		outEdge[i]=nullptr;
		consumerEdge[i]=nullptr;
	}
	i=j=2;   //开始获取 服务器的类型，服务器最大供应流量，服务器价格
	n=10;     //最多10种服务器类型
	this->serverCost=new int[n];
	this->serverSuply=new int[n];
	while(true){
		line=topo[i];
		if(line[0]=='\r'||line[0]=='\n')  //服务器数据结束
			break;
		readLine(line,tmp,3);
		serverSuply[i-2]=tmp[1];
		serverCost[i-2]=tmp[2];
		i+=1;
	}
	serverTypeNum=i-j;
	i+=1;
	this->deployCost=new int[this->netNodeNum];
	for(j=i;j<i+this->netNodeNum;j++){  //获取每个网络节点的部署成本
		line=topo[j];
		readLine(line,tmp,2);
		deployCost[tmp[0]]=tmp[1];
		//deployCost[tmp[0]]=0;
	}
	i=j+1;
	for(j=i;j<edgeNum+i;j++){          //获取网络链路相关信息
		line=topo[j];
		readLine(line,tmp,4);
		addEdge(tmp[0],tmp[1],tmp[2],tmp[3]);
	}
	i=j+1;
	for(j=i;j<consumerEdgeNum+i;j++){ //获取消费节点相关数据
		line=topo[j];
		readLine(line,tmp,3);
		Edge *e=new Edge(tmp[0],tmp[2],0);
		consumerEdge[tmp[1]]=e;
		consumerIdx[tmp[0]]=tmp[1];
		isConnectWithConsumer[tmp[1]]=true;
	}
	//getNet2ConmerByOrder(4);
	maxOutFlow.insert(maxOutFlow.begin(),netNodeNum,0);
	vector<Edge*>* pedges;
	Edge*pedge;
	for(i=0;i<netNodeNum;i++){
		if(isConnectWithConsumer[i])
			maxOutFlow[i]+=consumerEdge[i]->capicity;
		pedges=outEdge[i];
		for(j=0;j<pedges->size();j++){
			pedge=pedges->at(j);
			maxOutFlow[i]+=pedge->capicity;
		}
	}
}

Graph::~Graph()
{
	int i;
	if(this->isConnectWithConsumer!=nullptr)
		delete [] this->isConnectWithConsumer;
	if(this->consumerIdx!=nullptr)
		delete [] consumerIdx;
	for(i=0;i<outEdge.size();i++)
	{
		vector<Edge *>* pedges=outEdge[i];
		if(pedges==nullptr)
			continue;
		for(int i=0;i<pedges->size();i++)
				delete (*pedges)[i];
		delete pedges;
	}
}

void Graph::addEdge(int inV,int outV,int u,int c)
{
	Edge *e1=new Edge(outV,u,c);
	Edge *e2=new Edge(inV,u,c);

	e1->reverseEdge=e2;
	e2->reverseEdge=e1;
	if(outEdge[inV]==nullptr)       // 边(inv,outV)不存在
	{
		vector<Edge*>* vec=new vector<Edge*>();
		vec->reserve(20);
		vec->push_back(e1);
		outEdge[inV]=vec;
	}else{                      // 边(inv,outV)已经存在
		outEdge[inV]->push_back(e1);
	}
	if(outEdge[outV]==nullptr)       // 该边不存在
	{
		vector<Edge*> *vec2=new vector<Edge*>();
		vec2->reserve(20);
		vec2->push_back(e2);
		outEdge[outV]=vec2;
	}else{                      // 边已经存在
		outEdge[outV]->push_back(e2);
	}
}

void Graph::addEdge(int inV,int outV,int u) //用在最后求轨迹的时候，整理流量
{
	Edge *e=new Edge(outV,u,0);
	if(outEdge[inV]==nullptr)       // 边(inv,outV)不存在
	{
		vector<Edge*> *vec=new vector<Edge*>();
		vec->reserve(20);
		vec->push_back(e);
		outEdge[inV]=vec;
	}else{                      // 边(inv,outV)已经存在
		outEdge[inV]->push_back(e);
	}
}

void Graph::dijkstra(int s,vector<int> &distance)  //outEdge
{
	if(distance.size()<this->netNodeNum){
		distance.resize(netNodeNum);
	}
	bool *isVisited=new bool[netNodeNum];
	int i,j;
	for(i=0;i<netNodeNum;i++)
	{
		isVisited[i]=false;
		distance[i]=1e10;
	}
	int curMinDistance;   //距离最小的边
	int minVertex=-1; 
	map<int,vector<Edge*> *>::iterator it;
	Edge *edge;
	distance[s]=0;
	for(i=0;i<netNodeNum;i++)  // netNodeNum-1
	{
		curMinDistance=1e7;
		minVertex=-1;
		for(j=0;j<netNodeNum;j++)  //得到求最短路径，下一次的顶点
		{
			if(!isVisited[j]&&distance[j]<curMinDistance)
			{
				curMinDistance=distance[j];
				minVertex=j;
			}
		}
		if(minVertex==-1){
			break;
		}
		isVisited[minVertex]=true;
		vector<Edge*>* &tmp=outEdge[minVertex];  //邻接边
		for(j=0;j<tmp->size();j++)
		{
			edge=(*tmp)[j];
			if(!isVisited[edge->outVertex] && distance[edge->outVertex]>distance[minVertex]+edge->cost)
			{
				distance[edge->outVertex]=distance[minVertex]+edge->cost;
			}
		}
	}
	delete [] isVisited;
}
//SPFA 算法求解最短路径，维持一个队列，求到所有点距离，相对dijkstra较快
void Graph::SPFA(int s,vector<int>&distance)
{
	if(distance.size()<netNodeNum){
		distance.resize(netNodeNum);
	}
	bool *inQueue=new bool[netNodeNum];
	int i,j;
	for(i=0;i<netNodeNum;i++)
	{
		inQueue[i]=false;
		distance[i]=1e10;
	}
	int cur; 
	Edge *edge;
	distance[s]=0;
	queue<int> q;
	q.push(s);
	inQueue[s]=true;
	while(!q.empty())
	{
		cur=q.front();
		q.pop();
		inQueue[cur]=false;
		vector<Edge*>* &tmp=outEdge[cur];  //邻接边
		for(j=0;j<tmp->size();j++)
		{
			edge=(*tmp)[j];
			if(distance[edge->outVertex]>distance[cur]+edge->cost)
			{
				distance[edge->outVertex]=distance[cur]+edge->cost;
				if(!inQueue[edge->outVertex])
				{
					q.push(edge->outVertex);
					inQueue[edge->outVertex]=true;
				}
			}
		}
	}
	delete [] inQueue;
}
void Graph::print()
{
	int i,j;
	cout<<"网络节点数量:"<<this->netNodeNum<<endl;
	cout<<"网络链路数量:"<<this->edgeNum<<endl;
	cout<<"消费节点数量:"<<this->consumerEdgeNum<<endl;
	cout<<"视频服务器类型数量:"<<this->serverTypeNum<<endl;
	for(i=0;i<serverTypeNum;i++){
		cout<<"服务器类型编号:"<<i;
		cout<<",可供应流:"<<serverSuply[i];
		cout<<",价格:"<<serverCost[i]<<endl;
	}
	float cost1=0,cost2=0;
	for(i=0;i<netNodeNum;i++){
		if(isConnectWithConsumer[i]){
			cost1+=deployCost[i];
		}else{
			cost2+=deployCost[i];
		}
	}
	cout<<"与消费节点直连的网络节点平均部署成本:"<<cost1/consumerEdgeNum<<endl;
	cout<<"不与消费节点直连的网络节点平均部署成本:"<<cost2/(netNodeNum-consumerEdgeNum)<<endl;
	cout<<"不和消费节点直连的网络节点为:";
	for(i=0;i<this->netNodeNum;i++)
		if(!isConnectWithConsumer[i])
			cout<<i<<",";
	cout<<endl;
	cout<<"和消费节点直连的网络节点为:";
	for(i=0;i<this->netNodeNum;i++)
		if(isConnectWithConsumer[i])
			cout<<i<<",";
	cout<<endl;
	cout<<"节点--->边"<<endl;
	vector<Edge*> *myedge;
	for(j=0;j<outEdge.size();j++)
	{
		myedge=outEdge[j];
		cout<<j<<" -->"<<maxOutFlow[j]<<"-->"<<myedge->size()<<endl;
		//cout<<j<<" -->"<<maxOutFlow[j]<<"-->"<<net2consumer[j].size()<<"-->"<<myedge->size()<<endl;
		/*for(i=0;i<myedge->size();i++)
			cout<<(*myedge)[i]->outVertex<<",";
		cout<<endl;*/
	}
	
}

void Graph::reset()
{
	vector<Edge *> * pvec;
	Edge *pedge;
	int i,j;
	for(j=0;j<outEdge.size();j++)
	{
		pvec=outEdge[j];
		for(i=0;i<pvec->size();i++)
		{
			pedge=(*pvec)[i];
			pedge->capicity=pedge->capCopy;
		}
	}
}

void Graph::printFlow()
{
	vector<vector<Edge*>*> &edges=this->outEdge;
	int i,j;
	for(i=0;i<edges.size();i++){
		vector<Edge*>* pedges=edges[i];
		Edge*p1,*p2;
		for(j=0;j<pedges->size();j++){
			p1=pedges->at(j);
			if(i<p1->outVertex){
				p2=p1->reverseEdge;
				if(p1->capicity<p2->capicity){
					cout<<"x("<<i<<","<<j<<")="<<(p2->capicity-p1->capicity)/2<<endl;
				}else if(p2->capicity<p1->capicity){
					cout<<"x("<<i<<","<<j<<")="<<(p1->capicity-p2->capicity)/2<<endl;
				}
			}
		}
	}
}

void Graph::getNet2ConmerByOrder(int k)
{
	int i,j,n,m,idx;
	m=this->netNodeNum*this->consumerEdgeNum;
	vector<vector<int> > dis;
	dis.resize(netNodeNum);
	for(i=0;i<netNodeNum;i++)
		dis[i].resize(consumerEdgeNum);
	int *d=new int[m];
	vector<int> tmp;
	for(i=0;i<this->consumerEdgeNum;i++)
	{
		idx=consumerIdx[i];
		SPFA(idx,tmp);
		for(j=0;j<tmp.size();j++) //计算每个网络节点 j 到 消费节点 i 沿着最短路径传送视频流，需要的花费
			dis[j][i]=tmp[j];
	}
	for(i=0;i<netNodeNum;i++)
		for(j=0;j<consumerEdgeNum;j++)
			d[i*consumerEdgeNum+j]=dis[i][j];
	n=this->netNodeNum*k-1;
	int u=0;
	if(this->consumerEdgeNum<k)  //获取一个分位数，使得平均每个网络节点适合3个消费节点，后面调试增加
	{
		u=1e6;
	}else{
		nth_element(d,d+n-1,d+m);
		u=d[n-1];
	}
	delete [] d;
	net2consumer.resize(netNodeNum);
	for(i=0;i<this->netNodeNum;i++)
	{
		vector<int> vec;
		for(j=0;j<this->consumerEdgeNum;j++)   // j 消费节点编号
		{
			if(dis[i][j]<=u)
			{
				vec.push_back(j);
			}
		}
		net2consumer[i]=vec;
	}
}

void Graph::copyG(vector<vector<Edge*>* > &bestg)  //深拷贝图
{
	vector<vector<Edge*>* > &cg=this->outEdge;
	bestg.clear();
	bestg.insert(bestg.begin(),cg.size(),nullptr);
	int i,j,idx;
	vector<Edge*>*pgs1,*pgs2;
	Edge*e,*e1,*e2;
	for(i=0;i<cg.size();i++)
	{
		pgs1=cg[i];
		for(j=0;j<pgs1->size();j++)
		{
			e=pgs1->at(j);
			if(i<e->outVertex){
				e1=new Edge(e->outVertex,e->capicity,e->cost);
				e2=new Edge(i,e->capicity,e->cost);
				e1->reverseEdge=e2;
				e2->reverseEdge=e1;
				if(bestg[i]==nullptr)
					bestg[i]=new vector<Edge*>;
				bestg[i]->push_back(e1);

				idx=e->outVertex;
				if(bestg[idx]==nullptr){
					bestg[idx]=new vector<Edge*>;
				}
				bestg[idx]->push_back(e2);
			}
		}
	}
}

