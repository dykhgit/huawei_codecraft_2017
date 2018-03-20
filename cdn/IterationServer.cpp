#include "IterationServer.h"
#include <sys/timeb.h>
#include <iostream>
#include <queue>
#include <algorithm>
using namespace std;
IterationServer::IterationServer(){}

IterationServer::IterationServer(Graph *g)
{
	struct timeb rawtime;
	ftime(&rawtime);
	beginTime=rawtime.millitm+rawtime.time*1000;
	this->g=g;
	bestg=new Graph;
	bestg->isConnectWithConsumer=nullptr;
	bestg->consumerIdx=nullptr;
	vector<vector<Edge*>*>&tmp=bestg->outEdge;
	tmp.resize(g->netNodeNum);
	fill(tmp.begin(),tmp.end(),nullptr);
	iterCount=0;
	if(g->netNodeNum>500){  //每5次run,进行计时
		iterMax=5;                
		iterTime=89000;
	}else if(g->netNodeNum>200){
		iterMax=30;               
		iterTime=89000;
	}else{
		iterMax=40;          
		iterTime=89000;
	}
}
IterationServer::~IterationServer()
{
	delete bestg;
}

unsigned long IterationServer::getRunTime()
{
	return getmillitm()-beginTime;
}

unsigned long IterationServer::getmillitm()
{
	struct timeb rawtime;
	ftime(&rawtime);
	return rawtime.millitm+rawtime.time*1000;
}

int IterationServer::runFlow(vector<int> &cur,vector<int>& serverType,vector<int> &flow,vector<int>& inFlow,map<int,int>&demand)
{
	/*clock_t t1,t2,t3,t4;
	g->reset();
	t1=clock();
	int cost= minCostFlowSPFA(cur,serverType,flow,inFlow,g->outEdge,demand);
	t2=clock();
	g->reset();
	t3=clock();
	int cost2=minCostFlowZKW(cur,serverType,flow,inFlow,g->outEdge,demand);
	t4=clock();
	if(cost!=cost2){
	cout<<"测试数据:";
	for(int i=0;i<cur.size();i++)
	cout<<cur[i]<<",";
	cout<<endl;
	cout<<"时间spfa="<<t2-t1<<"值="<<cost<<endl;
	cout<<"-----时间zkw="<<t4-t3<<"值="<<cost2<<endl;
	cout<<"-----------二者不等,差值为"<<cost2-cost<<"-------------"<<endl;
	}*/
	g->reset();
	int cost=minCostFlowZKW(cur,serverType,flow,inFlow,g->outEdge,demand);
	if(cost!=-1)
	{
		if(cost<bestCost)  //最优解得到更新,保存相关信息
		{
			bestCost=cost;
			bestSolution=cur;
			this->serverType=serverType;
			bestinflow.clear();
			bestinflow.insert(bestinflow.begin(),inFlow.begin(),inFlow.end());
			bestflow.clear();
			bestflow.insert(bestflow.begin(),flow.begin(),flow.end());
			keepg();
			/*cout<<"number="<<cur.size()<<"cost="<<cost<<"---------";
			for(int i=0;i<cur.size();i++)
			cout<<cur[i]<<",";
			cout<<endl;*/
		}
	}
	iterCount+=1;
	return cost;
}

// true:有解 false:无解
bool IterationServer::getInitSolutionAndW2(vector<int>& w2)
{
	vector<int> sol,curServerType,flow,inFlow;
	map<int,int> demand;
	vector<vector<Edge*>* > &outEdge=g->outEdge;
	int i,c,cost;
	bool *in=new bool[g->netNodeNum];
	fill(in,in+g->netNodeNum,true);
	bestCost=0x7fffffff;
	float aveNeed=0;
	for(i=0;i<g->consumerEdgeNum;i++){
		c=(g->consumerIdx)[i];
		aveNeed=(g->consumerEdge)[c]->capicity;
	}
	aveNeed/=g->consumerEdgeNum;
	if(g->serverTypeNum>=2){
		if(g->netNodeNum<200){
			aveNeed=max<float>(aveNeed,(g->serverSuply)[0]);  //该点最多流出的流量必须大于最低级服务器的流量
		}else if(g->netNodeNum<500){
			aveNeed=max<float>(aveNeed,(g->serverSuply)[1]);
		}else{
			aveNeed=max<float>(aveNeed,(g->serverSuply)[1]);
		}
	}
	for(i=0;i<g->netNodeNum;i++)
		if((g->maxOutFlow)[i]<aveNeed)
			in[i]=false;
	for(i=0;i<g->netNodeNum;i++){
		if(in[i])
			sol.push_back(i);
	}
	cost=runFlow(sol,curServerType,flow,inFlow,demand);
	if(cost==-1)        //该解不可行
	{
		for(i=0;i<g->consumerEdgeNum;i++){
			c=(g->consumerIdx)[i];
			if(demand[c]>0&&!in[c])
				sol.push_back(c);
		}
		demand.clear();
		cost=runFlow(sol,curServerType,flow,inFlow,demand);
		if(cost==-1){
			sol.clear();
			for(i=0;i<g->netNodeNum;i++)
				sol.push_back(i);
		}
		cost=runFlow(sol,curServerType,flow,inFlow,demand);   //如果还是不可行，就是无解了
	}
	w2.clear();
	w2.insert(w2.begin(),sol.begin(),sol.end());
	delete [] in;
	if(cost==-1)
		return false;
	else 
		return true;
}

//从直连点开始删除，最后留下的直连点基本是最优解需要的，当然也有少量特例
bool IterationServer::getInitSolution(vector<int> &w2,int *skip)
{
	vector<int> sol,curSol,flow,inFlow,curServerType;
	map<int,int> demand;
	vector<vector<Edge*>* > &outEdge=g->outEdge;
	int i,j,m,c=0;
	bool outTime,stop;
	int curCost,cost;
	bool *in=new bool[g->netNodeNum];
	if(getInitSolutionAndW2(w2))
		sol=bestSolution;
	else
		return false;
	vector<sortHelper<int> >sorts;
	//clock_t t1,t2;
	while(true)
	{
		m=sol.size();
		outTime=false;
		stop=true;
		curCost=bestCost;
		for(i=0;i<m;i++)                   //删除
		{
			if(skip[sol[i]]>=3)  //该节点被多次删除，仍得不到改进，认为其应该出现在最终解中，不再删除
				continue;
			curSol.clear();
			if(i==0)
				curSol.insert(curSol.begin(),sol.begin()+1,sol.end());
			else if(i==m-1){
				curSol.insert(curSol.begin(),sol.begin(),sol.begin()+i);
			}else{
				curSol.insert(curSol.begin(),sol.begin(),sol.begin()+i);
				curSol.insert(curSol.end(),sol.begin()+i+1,sol.end());
			}
			//t1=clock();
			curServerType.clear();
			cost=runFlow(curSol,curServerType,flow,inFlow,demand);
			//t2=clock();
			//cout<<"time:"<<t2-t1<<endl;
			if(iterCount>iterMax)   //检验是否超时
			{
				iterCount=0;
				if(this->getRunTime()>iterTime){
					outTime=true;
					break;
				}
			}
			if(cost==-1){
				skip[sol[i]]+=2;
			}else{
				if(cost>=curCost)      //删除 sol[i] 不能使得解变好
				{
					skip[sol[i]]+=1;
				}else{               //找到一组更好的解,对新解进行排序，直连点在前，非直连点在后
					stop=false;
					sorts.clear();
					for(j=0;j<sol.size();j++)
						if(j!=i&&(g->isConnectWithConsumer)[sol[j]])
							sorts.push_back(sortHelper<int>(sol[j],flow[sol[j]]));
					c=sorts.size();
					for(j=0;j<sol.size();j++)
						if(j!=i&&!((g->isConnectWithConsumer)[sol[j]]))
							sorts.push_back(sortHelper<int>(sol[j],flow[sol[j]]*1e4-(g->deployCost)[sol[j]]));
					sort(sorts.begin(),sorts.begin()+c,compare);
					sort(sorts.begin()+c,sorts.end(),compare);
					sol.clear();
					for(j=0;j<sorts.size();j++)
						sol.push_back(sorts[j].id);
					break;
				}
			}
		}
		if(stop||outTime)    //不能通过删除获得更好的解了,退出
		{
			break;
		}
	}
	c=0;
	m=min<int>(g->serverTypeNum/2,2);
	for(i=0;i<serverType.size();i++)
		if(serverType[i]<=m)
			c=c+1;
	if(g->netNodeNum>1000&&c>=0.3*sol.size())  //布置的服务器太多，生成了一个不大恰当的解，加入一些直连点,然后排序，消费节点按流量排序，新加入直连点，原来直连点
	{
		cout<<"尝试继续删除"<<endl;
		cout<<"当前节点数:"<<bestSolution.size()<<endl;
		sol=bestSolution;
		flow=bestflow;
		fill(in,in+g->netNodeNum,false);
		c=0;
		sorts.clear();
		vector<int> s1,s2;
		for(i=0;i<sol.size();i++){
			in[sol[i]]=true;
			if((g->isConnectWithConsumer)[sol[i]]){
				c+=1;
				s2.push_back(sol[i]);
			}else{
				if(flow.size()==g->netNodeNum){
					sorts.push_back(sortHelper<int>(sol[i],flow[sol[i]]));
				}else{
					sorts.push_back(sortHelper<int>(sol[i],(g->maxOutFlow)[sol[i]]));
				}
			}
		}
		sort(sorts.begin(),sorts.end(),compare);
		sol.clear();
		for(i=0;i<sorts.size();i++)
			sol.push_back(sorts[i].id);
		sorts.clear();
		for(i=0;i<g->netNodeNum;i++)
			if((g->isConnectWithConsumer)[i]&&!in[i])
				sorts.push_back(sortHelper<int>(i,-(g->maxOutFlow)[i]));
		sort(sorts.begin(),sorts.end(),compare);
		s1.reserve(c);
		for(i=0;i<c;i++)
			s1.push_back(sorts[i].id);
		cout<<"增加:";
		for(i=0;i<s1.size();i++)
			cout<<s1[i]<<",";
		cout<<endl;
		sol.insert(sol.end(),s1.begin(),s1.end());
		sol.insert(sol.end(),s2.begin(),s2.end());
		cout<<"增加后节点数:"<<sol.size()<<endl;
		fill(skip,skip+g->netNodeNum,0);
		del(sol,flow,skip);
	}
	if(outTime)
		return true;
	else
		return false;
}

//通过交换试图得到一个更好的解
// 1:得到更好解 0:没有找到更好解 -1:时间到了
int IterationServer::exchangeAndAdd(const vector<int> &sol,const vector<int> &flow)
{
	vector<int> others,curServerType,nSerTy;  //s1 非直连点  s2 直连点
	bool *in=new bool[g->netNodeNum];
	fill(in,in+g->netNodeNum,false);
	int i,j,idx,tmp,cost,cost2,c=0;
	for(i=0;i<sol.size();i++)
		in[sol[i]]=true;
	vector<sortHelper<int> >sorts;
	for(i=0;i<g->netNodeNum;i++)
		if((g->isConnectWithConsumer)[i]&&!in[i])
			sorts.push_back(sortHelper<int>(i,-(g->maxOutFlow)[i]));
	sort(sorts.begin(),sorts.end(),compare);  //按照可供应的流量从大到小排序
	for(i=0;i<sorts.size();i++)
		others.push_back(sorts[i].id);
	int n=min<int>(12,sol.size());
	int m=min<int>(20,others.size());
	vector<int> curSol;
	curSol.insert(curSol.begin(),sol.begin(),sol.end());
	vector<int> curflow,inFlow;
	map<int,int>demand;
	int curCost=bestCost;
	for(i=0;i<curSol.size();i++)
	{
		if((g->isConnectWithConsumer)[curSol[i]]){
			continue;
		}
		c+=1;
		if(c>=n)
			break;
		for(j=0;j<m;j++)
		{
			tmp=bestCost;
			idx=curSol[i];
			curSol[i]=others[j];
			curServerType.clear();
			cost=runFlow(curSol,curServerType,curflow,inFlow,demand);
			if(cost!=-1){
				if(adjustServerType(curSol,inFlow,curServerType,nSerTy)){
					g->reset();
					cost2=runFlow(curSol,nSerTy,curflow,inFlow,demand);
					if(cost2==-1)
						cost2=runAndSearchServerType(curSol,inFlow,curServerType,nSerTy,cost);
					if(cost2<cost)
						cost=cost2;
				}
			}
			if(iterCount>iterMax){
				iterCount=0;
				if(this->getRunTime()>iterTime){
					return -1;
				}
			}
			if(cost!=-1&&cost<tmp) //找个一个更优解
			{
				cout<<"交换"<<i<<"/"<<curSol.size()<<"和"<<j<<"/"<<others.size()<<",("<<idx<<","<<others[j]<<")"<<endl;
			}
			curSol[i]=idx;
		}
	}
	if(bestCost<curCost){
		return 1;
	}
	m=min<int>(others.size(),20);
	for(i=0;i<m;i++)      //增加
	{
		if(i==0)
		{
			curSol.push_back(others[0]);
		}else{
			curSol.pop_back();
			curSol.push_back(others[i]);
		}
		tmp=bestCost;
		curServerType.clear();
		cost=runFlow(curSol,curServerType,curflow,inFlow,demand);
		if(iterCount>iterMax){
			iterCount=0;
			if(this->getRunTime()>iterTime){
				return -1;
			}
		}
		if(cost!=-1&&cost<tmp) //找个一个更优解
		{
			cout<<"节点"<<others[i]<<",第"<<i<<"/"<<others.size()<<"被增加"<<endl;
			return 1;
		}
	}
	return 0;
}


// 1:通过删除改进解,非直连点被优先删除， 0:没有找到更好的 -1:时间到了
int IterationServer::del(const vector<int> &initsol,vector<int> &flow,int*skip)
{
	vector<int>curSol,curFlow,curServerType,inFlow,solution,sol;
	map<int,int> demand;
	vector<vector<Edge*>* > &outEdge=g->outEdge;
	int i,j,m,c=bestCost;
	bool outTime,stop;
	int curCost,cost;
	vector<sortHelper<int> >sorts;
	//clock_t t1,t2;
	vector<int> s1,s2;
	for(i=0;i<initsol.size();i++)
	{
		j=initsol[i];
		if((g->isConnectWithConsumer)[j]){
			s2.push_back(j);   //直连
		}else{
			s1.push_back(j);   //非直连
		}
	}
	if(flow.size()==g->netNodeNum){
		sorts.clear();
		for(i=0;i<s1.size();i++){
			j=s1[i];
			sorts.push_back(sortHelper<int>(j,flow[j]));
		}
		s1.clear();
		for(i=0;i<sorts.size();i++)
			s1.push_back(sorts[i].id);
	}
	sol.insert(sol.begin(),s1.begin(),s1.end());
	sol.insert(sol.end(),s2.begin(),s2.end());
	curServerType.clear();
	int initcost=runFlow(sol,curServerType,flow,inFlow,demand);
	cout<<initcost<<"---------------"<<endl;
	while(true)
	{
		m=sol.size();
		outTime=false;
		stop=true;
		curCost=initcost;
		for(i=0;i<m;i++)                   //删除,不再删除直连点
		{
			if(skip[sol[i]]>=3)  //该节点被多次删除，仍得不到改进，认为其应该出现在最终解中，不再删除
				continue;
			curSol.clear();
			if(i==0)
				curSol.insert(curSol.begin(),sol.begin()+1,sol.end());
			else if(i==m-1){
				curSol.insert(curSol.begin(),sol.begin(),sol.begin()+i);
			}else{
				curSol.insert(curSol.begin(),sol.begin(),sol.begin()+i);
				curSol.insert(curSol.end(),sol.begin()+i+1,sol.end());
			}
			//t1=clock();
			curServerType.clear();
			cost=runFlow(curSol,curServerType,flow,inFlow,demand);
			//t2=clock();
			//cout<<"time:"<<t2-t1<<endl;
			if(iterCount>iterMax)   //检验是否超时
			{
				iterCount=0;
				if(this->getRunTime()>iterTime){
					outTime=true;
					curFlow.clear();
					curFlow.insert(curFlow.begin(),flow.begin(),flow.end());
					break;
				}
			}
			if(cost==-1){
				skip[sol[i]]+=2;
			}else{
				if(cost>=curCost)      //删除 sol[i] 不能使得解变好
				{
					skip[sol[i]]+=1;
				}else{               //找到一组更好的解,按照流排序，且非直连点在前，尝试继续删除
					sorts.clear();
					s2.clear();
					for(j=0;j<sol.size();j++){
						if(j!=i){
							if((g->isConnectWithConsumer)[sol[j]]){
								s2.push_back(sol[j]);
							}else{
								sorts.push_back(sortHelper<int>(sol[j],flow[sol[j]]));
							}
						}
					}
					sort(sorts.begin(),sorts.end(),compare);
					sol.clear();
					for(j=0;j<sorts.size();j++)
						sol.push_back(sorts[j].id);
					sol.insert(sol.end(),s2.begin(),s2.end());
					curFlow.clear();
					curFlow.insert(curFlow.begin(),flow.begin(),flow.end());
					stop=false;
					initcost=cost;
					break;
				}
			}
		}
		if(stop||outTime)    //不能通过删除获得更好的解了,退出
		{
			solution.clear();
			solution.insert(solution.begin(),sol.begin(),sol.end());
			flow.clear();
			flow.insert(flow.begin(),curFlow.begin(),curFlow.end());
			break;
		}
	}
	
	if(outTime)
		return -1;
	else if(bestCost>=c)
		return 0;
	else
		return 1;
}

// true:时间到了 false:还有时间
bool IterationServer::searchNeighborhood(vector<int> &cur,vector<int> &others,vector<int>& flow)
{
	vector<int> curSolution,curServerType,inFlow,nSerTy;
	map<int,int> demand;
	int i,j,m1,n1,curCost=bestCost,cost;
	int m=cur.size(),n=others.size();
	int stopTime;
	stopTime=this->iterTime;
	int tmp;
	curSolution.clear();
	curSolution.insert(curSolution.begin(),cur.begin(),cur.end());
	if(g->netNodeNum<1000){
		m1=min<int>(m,40);
		n1=min<int>(n,30);
	}else{
		m1=min<int>(m,5);
		n1=min<int>(n,10);
	}
	for(i=0;i<m1;i++)          //交换,控制个数
	{
		for(j=0;j<n1;j++)
		{
			tmp=curSolution[i];
			curSolution[i]=others[j];
			curServerType.clear();
			cost=runFlow(curSolution,curServerType,flow,inFlow,demand);
			if(cost!=-1){
				if(adjustServerType(curSolution,inFlow,curServerType,nSerTy)){
					g->reset();
					runFlow(curSolution,nSerTy,flow,inFlow,demand);
				}
			}
			if(iterCount>iterMax){
				iterCount=0;
				if(this->getRunTime()>stopTime){
					return true;
				}
			}
			curSolution[i]=tmp;
		}
	}
	if(cost!=-1&&cost<curCost)  //交换成功不再进行后续操作
		return false;

	curSolution.clear();
	curSolution.insert(curSolution.begin(),cur.begin(),cur.end());
	n1=min<int>(n,30);
	for(i=0;i<n1;i++)           //增加
	{
		if(i==0)
		{
			curSolution.push_back(others[0]);
		}else{
			curSolution.pop_back();
			curSolution.push_back(others[i]);
		}
		curServerType.clear();
		cost=runFlow(curSolution,curServerType,flow,inFlow,demand);
		if(cost!=-1){
			if(adjustServerType(curSolution,inFlow,curServerType,nSerTy)){
				g->reset();
				runFlow(curSolution,nSerTy,flow,inFlow,demand);
			}
		}
		if(iterCount>iterMax){
			iterCount=0;
			if(this->getRunTime()>stopTime){
				return true;
			}
		}
	}

	curSolution.clear();
	curSolution.insert(curSolution.begin(),cur.begin(),cur.end());
	m1=min<int>(m,30);
	for(i=0;i<m1;i++)   //删除
	{
		curSolution.clear();
		if(i==0)
			curSolution.insert(curSolution.begin(),cur.begin()+1,cur.end());
		else if(i==m-1){
			curSolution.insert(curSolution.begin(),cur.begin(),cur.begin()+i);
		}else{
			curSolution.insert(curSolution.begin(),cur.begin(),cur.begin()+i);
			curSolution.insert(curSolution.end(),cur.begin()+i+1,cur.end());
		}
		curServerType.clear();
		cost=runFlow(curSolution,curServerType,flow,inFlow,demand);
		if(cost!=-1){
			if(adjustServerType(curSolution,inFlow,curServerType,nSerTy)){
				g->reset();
				runFlow(curSolution,nSerTy,flow,inFlow,demand);
			}
		}
		if(iterCount>iterMax){
			iterCount=0;
			if(this->getRunTime()>stopTime){
				return true;
			}
		}
	}
	return false;
}

//true:时间到了 false:还有时间
bool IterationServer::searchForBetter(vector<int> &cur,vector<int> &others,vector<int>& flow)
{
	vector<int> curSolution,curServerType,inFlow,nSerTy;
	map<int,int> demand;
	int i,j,cost,cost2,m1,n1,curCost=bestCost;
	int m=cur.size(),n=others.size();
	int stopTime;
	stopTime=this->iterTime;
	curSolution.clear();
	curSolution.insert(curSolution.begin(),cur.begin(),cur.end());
	n1=min<int>(25,n);
	for(i=0;i<n1;i++)           //增加
	{
		if(i==0)
		{
			curSolution.push_back(others[0]);
		}else{
			curSolution.pop_back();
			curSolution.push_back(others[i]);
		}
		curServerType.clear();
		cost=runFlow(curSolution,curServerType,flow,inFlow,demand);
		if(cost!=-1){
			if(adjustServerType(curSolution,inFlow,curServerType,nSerTy)){
				g->reset();
				cost2=runFlow(curSolution,nSerTy,flow,inFlow,demand);
				if(cost2!=-1&&cost2<cost)
					cost=cost2;
			}
		}
		if(cost!=-1&&cost<curCost){
			cout<<"增加:"<<i<<"/"<<others.size()<<endl;
			return false;
		}
		if(iterCount>iterMax){
			iterCount=0;
			if(this->getRunTime()>stopTime){
				return true;
			}
		}
	}
	curSolution.clear();
	curSolution.insert(curSolution.begin(),cur.begin(),cur.end());
	m1=min<int>(m,25);
	for(i=0;i<m1;i++)   //删除
	{
		curSolution.clear();
		if(i==0)
			curSolution.insert(curSolution.begin(),cur.begin()+1,cur.end());
		else if(i==m-1){
			curSolution.insert(curSolution.begin(),cur.begin(),cur.begin()+i);
		}else{
			curSolution.insert(curSolution.begin(),cur.begin(),cur.begin()+i);
			curSolution.insert(curSolution.end(),cur.begin()+i+1,cur.end());
		}
		curServerType.clear();
		cost=runFlow(curSolution,curServerType,flow,inFlow,demand);
		if(cost!=-1){
			if(adjustServerType(curSolution,inFlow,curServerType,nSerTy)){
				g->reset();
				cost2=runFlow(curSolution,nSerTy,flow,inFlow,demand);
				if(cost2!=-1&&cost2<cost)
					cost=cost2;
			}
		}
		if(cost!=-1&&cost<curCost){
			cout<<"删除:"<<i<<"/"<<m<<endl;
			return false;
		}
			
		if(iterCount>iterMax){
			iterCount=0;
			if(this->getRunTime()>stopTime){
				return true;
			}
		}
	}
	int tmp;
	curSolution.clear();
	curSolution.insert(curSolution.begin(),cur.begin(),cur.end());
	m1=min<int>(m,30);      // 15*25
	n1=min<int>(n,30);
	for(i=0;i<m1;i++)          //交换
	{
		for(j=0;j<n1;j++)
		{
			tmp=curSolution[i];
			curSolution[i]=others[j];
			curServerType.clear();
			cost=runFlow(curSolution,curServerType,flow,inFlow,demand);
			if(cost!=-1){
				if(adjustServerType(curSolution,inFlow,curServerType,nSerTy)){
					g->reset();
					cost2=runFlow(curSolution,nSerTy,flow,inFlow,demand);
					if(cost2!=-1&&cost2<cost)
						cost=cost2;
				}
			}
			if(cost!=-1&&cost<curCost){
				cout<<"交换:("<<i<<"/"<<m<<","<<j<<"/"<<n<<")"<<endl;
				return false;
			}

			if(iterCount>iterMax){
				iterCount=0;
				if(this->getRunTime()>stopTime){
					return true;
				}
			}
			curSolution[i]=tmp;
		}
	}

	return false;
}

//重新排序现在部署位置 sol,另外生成others 集，主要用来交换，w2=sol + others, sol 非直连+直连+...  others 直连+非直连
void IterationServer::getSortedSolAndO(vector<int>& sol,vector<int>&others,const vector<int> &w2,const vector<int> &flow,bool *in)
{
	vector<int> s1,s2;
	vector<sortHelper<int> >sorts1,sorts2;
	int i,j,idx,m,n;
	for(i=0;i<w2.size();i++)
		in[w2[i]]=true;
	for(i=0;i<sol.size();i++){
		idx=sol[i];
		in[idx]=false;
		if((g->isConnectWithConsumer)[idx]){
			s1.push_back(idx);    //直连
		}else{
			s2.push_back(idx);   //非直连
		}
	}
	for(i=0;i<s2.size();i++){
		idx=s2[i];
		sorts1.push_back(sortHelper<int>(idx,flow[idx]));
	}
	sort(sorts1.begin(),sorts1.end(),compare);
	m=min<int>(20,sorts1.size());
	for(i=0;i<s1.size();i++){
		idx=s1[i];
		sorts2.push_back(sortHelper<int>(idx,flow[idx]));
	}
	sort(sorts2.begin(),sorts2.end(),compare);
	n=min<int>(10,s1.size());
	sol.clear();
	for(i=0;i<m;i++)
		sol.push_back(sorts1[i].id);
	for(i=0;i<n;i++)
		sol.push_back(sorts2[i].id);
	for(i=m;i<sorts1.size();i++)
		sol.push_back(sorts1[i].id);
	for(i=n;i<sorts2.size();i++)
		sol.push_back(sorts2[i].id);

	//获取others集，并进行排序
	others.clear();
	for(i=0;i<w2.size();i++){
		if(in[w2[i]])
			others.push_back(w2[i]);
	}
	s1.clear();
	s2.clear();
	for(i=0;i<others.size();i++){
		idx=others[i];
		if((g->isConnectWithConsumer)[idx]){
			s1.push_back(idx);    //直连
		}else{
			s2.push_back(idx);   //非直连
		}
	}
	sorts1.clear();
	for(i=0;i<s1.size();i++){
		idx=s1[i];
		sorts1.push_back(sortHelper<int>(idx,-(g->maxOutFlow)[idx]));
	}
	sort(sorts1.begin(),sorts1.end(),compare);
	m=min<int>(sorts1.size(),15);
	sorts2.clear();
	for(i=0;i<s2.size();i++){
		idx=s2[i];
		sorts2.push_back(sortHelper<int>(idx,-flow[idx]));
	}
	sort(sorts2.begin(),sorts2.end(),compare);
	n=min<int>(sorts2.size(),10);
	others.clear();
	for(i=0;i<m;i++)
		others.push_back(sorts1[i].id);
	for(i=0;i<n;i++)
		others.push_back(sorts2[i].id);
	for(i=m;i<sorts1.size();i++)
		others.push_back(sorts1[i].id);
	for(i=n;i<sorts2.size();i++)
		others.push_back(sorts2[i].id);
}

const char* IterationServer::iteration()
{
	bool stop;
	vector<int> sol,curSol,flow,inFlow,curFlow,others,w2;
	int cost,i,count;
	int *skip=new int[g->netNodeNum];
	fill(skip,skip+g->netNodeNum,0);
	stop=getInitSolution(w2,skip);
	int state1,state2;
	sol=bestSolution;
	flow=bestflow;
	cout<<"得到初始解,cost"<<bestCost<<"-----";
	for(i=0;i<sol.size();i++)
		cout<<sol[i]<<",";
	cout<<endl;
	cout<<"----------进行调整----------"<<endl;
	if(!stop){
		while(true)
		{
			state1=exchangeAndAdd(sol,flow);
			if(state1==-1)
				break;
			sol=bestSolution;
			fill(skip,skip+g->netNodeNum,0);
			state2=del(sol,flow,skip);
			if(state2==-1)
				break;
			if(state1==0&&state2==0)
				break;
		}
	}
	sol=bestSolution;
	cout<<"得到解 cost="<<bestCost<<endl;
	cout<<"run time:"<<getRunTime()<<endl;
	
	for(i=0;i<sol.size();i++)
		cout<<sol[i]<<",";
	cout<<endl;
	for(i=0;i<serverType.size();i++)
		cout<<serverType[i]<<",";
	cout<<endl;
	vector<int> con,nocon;
	for(int i=0;i<sol.size();i++)
	{
		cout<<"("<<sol[i]<<","<<(g->serverCost)[serverType[i]]<<","<<bestinflow[sol[i]]
		<<"/"<<(g->serverSuply)[serverType[i]]<<"/"<<(g->maxOutFlow)[sol[i]]<<","<<((g->outEdge)[sol[i]])->size()<<","<<skip[sol[i]]<<"), ";
		if((g->isConnectWithConsumer)[sol[i]])
			con.push_back(sol[i]);
		else
			nocon.push_back(sol[i]);
	}
	cout<<endl;
	cout<<"其中直连的节点有"<<con.size()<<"个,分别是 ";
	sort(con.begin(),con.end());
	for(i=0;i<con.size();i++)
		cout<<con[i]<<",";
	cout<<endl;
	cout<<"不是直连的有"<<nocon.size()<<"个,分别是";
	sort(nocon.begin(),nocon.end());
	for(i=0;i<nocon.size();i++)
		cout<<nocon[i]<<",";
	cout<<endl;
	cout<<"排序:";
	sort(sol.begin(),sol.end());
	for(i=0;i<sol.size();i++)
		cout<<sol[i]<<" ";
	cout<<endl;
	const char *topo_file=showTrajectory();
	return topo_file;
}

const char* IterationServer::iteration2()
{
	bool stop;
	vector<int> sol,curSolution,flow,inFlow,curFlow,others,w2;
	int cost,i,curCost,count=0;
	int *skip=new int[g->netNodeNum];
	bool *in=new bool[g->netNodeNum];
	fill(skip,skip+g->netNodeNum,0);
	stop=getInitSolution(w2,skip);
	if(!stop&&bestCost==0x7fffffff){
		return "NA\n";
	}
	curSolution=bestSolution;
	cout<<"得到初始解,cost"<<bestCost<<"-----";
	for(i=0;i<curSolution.size();i++)
		cout<<curSolution[i]<<",";
	cout<<endl;
	cout<<"----------进行调整----------"<<endl;
	bool searchA=true;
	/*if(g->netNodeNum>1000)
		searchA=false;*/
	unsigned long t1,t2;
	vector<sortHelper<int> >sorts;
	while(!stop)
	{
		if(this->getRunTime()>this->iterTime){
			break;
		}
		curCost=bestCost;
		curSolution=bestSolution;
		flow=bestflow;
		getSortedSolAndO(curSolution,others,w2,flow,in);
		t1=getmillitm();
		if(searchA)
			stop=searchNeighborhood(curSolution,others,flow);
		else
			stop=searchForBetter(curSolution,others,flow);
		if(stop)
			break;
		count+=1;
		if(searchA&&g->netNodeNum>1000)
			searchA=false;
		if(searchA&&count>=2)  //最后一段时间每次在邻域找一个好的就重新找,最好算法自动停止
			searchA=false;
		t2=getmillitm();
		cout<<"迭代用时:"<<t2-t1<<endl;
		if(bestCost<curCost)   //经过调整得到更好的
		{
			cout<<"一次迭代结束,当前cost="<<bestCost<<endl;
		}else{
			cout<<"达到局部最优"<<endl;
			break;
		}
	}
	cout<<"得到解 cost="<<bestCost<<endl;
	cout<<"run time:"<<getRunTime()<<endl;
	/*sol=bestSolution;
	for(i=0;i<sol.size();i++)
		cout<<sol[i]<<",";
	cout<<endl;
	for(i=0;i<serverType.size();i++)
		cout<<serverType[i]<<",";
	cout<<endl;
	vector<int> con,nocon;
	for(int i=0;i<sol.size();i++)
	{
		cout<<"("<<sol[i]<<","<<(g->serverCost)[serverType[i]]<<","<<bestinflow[sol[i]]
		<<"/"<<(g->serverSuply)[serverType[i]]<<"/"<<(g->maxOutFlow)[sol[i]]<<","<<((g->outEdge)[sol[i]])->size()<<","<<skip[sol[i]]<<"), ";
		if((g->isConnectWithConsumer)[sol[i]])
			con.push_back(sol[i]);
		else
			nocon.push_back(sol[i]);
	}
	cout<<endl;
	cout<<"其中直连的节点有"<<con.size()<<"个,分别是 ";
	sort(con.begin(),con.end());
	for(i=0;i<con.size();i++)
		cout<<con[i]<<",";
	cout<<endl;
	cout<<"不是直连的有"<<nocon.size()<<"个,分别是";
	sort(nocon.begin(),nocon.end());
	for(i=0;i<nocon.size();i++)
		cout<<nocon[i]<<",";
	cout<<endl;
	cout<<"排序:";
	sort(sol.begin(),sol.end());
	for(i=0;i<sol.size();i++)
		cout<<sol[i]<<" ";
	cout<<endl;*/
	const char *topo_file=showTrajectory();
	return topo_file;
}

void IterationServer::addTrajectory(vector<vector<int> >& Trajectory,int pos,int beginID,vector<Edge *>& Tra,vector<int>&inflow)
{
	int min_flow=inflow[beginID],j;
	for(j=0;j<Tra.size();j++)
	{
		if((*Tra[j]).capicity<min_flow)
		{
			min_flow=(*Tra[j]).capicity;
		}
	}
	vector<int> tmp(Tra.size()+4);    //保存该条路径
	tmp[0]=beginID;
	for(j=0;j<Tra.size();j++)
	{
		(*Tra[j]).capicity-=min_flow;
		tmp[j+1]=(*Tra[j]).outVertex;
	}
	Edge *e=(g->consumerEdge)[tmp[Tra.size()]]; //加入消费节点
	tmp[Tra.size()+1]=e->outVertex;
	tmp[Tra.size()+2]=min_flow;
	tmp[Tra.size()+3]=serverType[pos];   //serverType[id]表示下标为id的服务器类型,不部署,该值无意义.
	inflow[beginID]-=min_flow;
	Trajectory.push_back(tmp);
	Tra.clear();
}

const char * IterationServer::showTrajectory()
{
	int i=0,j,n;
	int cur=-1,next=-1,s;
	vector<int> pos=bestSolution;
	n=pos.size();
	Edge *pedge;
	vector<vector<int> > Trajectory;
	vector<vector<Edge*> *> & m=bestg->outEdge;
	vector<int> &inFlow=bestinflow;
	bool flag=false;
	for(i=0;i<n;i++)
	{
		cur=pos[i];
		//判断 视频服务器位置是否和消费节点相邻，相邻直接添加路径
		if((g->isConnectWithConsumer)[cur])
		{
			Edge *e=(g->consumerEdge)[cur];
			vector<int> tmp(4);
			tmp[0]=cur;
			tmp[1]=e->outVertex;
			tmp[2]=min(e->capicity,inFlow[cur]);
			tmp[3]=serverType[i];
			Trajectory.push_back(tmp);
			inFlow[cur]-=tmp[2];
		}
		if(m[cur]==nullptr || m[cur]->size()==0||inFlow[cur]==0)  //没有其它路径
			continue;
		vector<Edge *> Tra;
		while(true)  //寻找所有以 pos[i] 为初始节点的路径，必须经过网络节点，即路径至少包含 视频服务器和消费节点外另一个网络节点
		{
			if(m[cur]==nullptr||m[cur]->size()==0)  //一条路径搜索完成
			{
				addTrajectory(Trajectory,i,pos[i],Tra,inFlow);
				cur=pos[i];
				if(inFlow[cur]==0)
					break;
			}else{
				vector<Edge*> * tmp=m[cur];  //cur 节点的后继几点
				flag=false;
				s=-1;
				for(j=0;j<(*tmp).size();j++)
				{
					pedge=(*tmp)[j];
					if(pedge->capicity>0)       //找到一个容量大于0的路径
					{
						flag=true;
						next=pedge->outVertex;
						s=j;
						break;
					}
				}
				if(flag)  //找到后继节点,保存路径
				{
					cur=next;
					Tra.push_back((*tmp)[s]);
				}else{      //没有容量大于0边
					if(cur!=pos[i]&&((g->isConnectWithConsumer)[cur]))  //找到一个消费节点
					{
						addTrajectory(Trajectory,i,pos[i],Tra,inFlow);
						cur=pos[i];
						if(inFlow[cur]==0)
							break;
					}else{
						break;
					}
				}
			}
		}
	}
	string *str=new string;
	char cstr[10];
	//fill(cstr,cstr+10,0);
	sprintf(cstr,"%d\n\n",Trajectory.size());
	str->append(cstr);
	int k=0,t;
	for(i=0;i<Trajectory.size();i++)
	{
		vector<int> &path=Trajectory[i];
		for(j=0;j<path.size();j++)
		{
			fill(cstr,cstr+10,0);
			cur=path[j];
			k=0;
			while(cur!=0)
			{
				cstr[k]=cur%10+'0';
				cur/=10;
				k+=1;
			}
			if(path[j]!=0)
			{
				for(t=k-1;t>=0;--t)
				{
					str->push_back(cstr[t]);
				}
			}else{
				str->push_back('0');
			}
			if(j==path.size()-1)
			{
				str->push_back('\n');
			}else{
				str->push_back(' ');
			}
		}
	}
	//cout<<str->c_str();
	return str->c_str();
}

//此处serverType和服务器个数一致,返回double 防止溢出
int IterationServer::minCostFlowSPFA(vector<int> &serverLoaction,vector<int>& serverType,vector<int> &flow, vector<int>& inFlow,vector<vector<Edge*>* >& edgeGraph,map<int,int>&demand)
{
	int i,j,cur,next,tmp;
	int totalCost=0;       
	vector<int> supply;      //serverLoaction[i] 剩余的流提供能力,不是服务器位置该值为0
	if(flow.size()<g->netNodeNum)
		flow.resize(g->netNodeNum);
	if(inFlow.size()<g->netNodeNum)
		inFlow.resize(g->netNodeNum);
	supply.resize(g->netNodeNum);
	bool *isServerLocation=new bool[g->netNodeNum];  //该位置 是否部署服务器
	for(i=0;i<g->netNodeNum;i++){
		inFlow[i]=flow[i]=supply[i]=0;
		isServerLocation[i]=false;
	}
	if(serverType.size()==0){  //服务器型号未定,默认都是流量最大的型号,也就是最后一个型号
		for(i=0;i<serverLoaction.size();i++)
			supply[serverLoaction[i]]=(g->serverSuply)[g->serverTypeNum-1];
	}else{
		for(i=0;i<serverLoaction.size();i++){
			supply[serverLoaction[i]]=(g->serverSuply)[serverType[i]];
			totalCost+=(g->serverCost)[serverType[i]];
		}
	}
	for(i=0;i<serverLoaction.size();i++){      //服务器部署成本
		isServerLocation[serverLoaction[i]]=true;
		totalCost+=(g->deployCost)[serverLoaction[i]];
	}
	for(i=0;i<g->consumerEdgeNum;i++)
	{
		cur=(g->consumerIdx)[i];
		tmp=(g->consumerEdge)[cur]->capicity;
		demand[cur]=tmp;
		if(isServerLocation[cur]){           //消费节点旁有服务器,优先满足旁边的消费节点
			tmp=min(supply[cur],tmp);
			flow[cur]+=tmp;
			demand[cur]-=tmp;
			supply[cur]-=tmp;
		}
	}
	/*int *pre=new int[g->netNodeNum];
	int *distance=new int[g->netNodeNum];*/
	bool * inQueue=new bool[g->netNodeNum];
	vector<int> pre;
	vector<int> distance;
	pre.resize(g->netNodeNum);
	distance.resize(g->netNodeNum);
	bool flag=false;
	
	int minCostVertexIdx;
	int minCostVertexval;
	vector<Edge*>* edges;
	Edge *edge;
	int currentDemand;
	int edgeCapicity;
	int edgeCost;
	vector<int> path;
	vector<Edge*> edgePath;
	Edge *pe1,*pe2;
	int preIdx=-1;
	queue<int> q;            //队列
	while(true)
	{
		for(i=0;i<g->netNodeNum;i++)
		{
			distance[i]=0x7fffffff;
			inQueue[i]=false;
		}
		for(i=0;i<serverLoaction.size();i++)
		{
			if(supply[serverLoaction[i]]>0)   //该点还提供流量
			{
				pre[serverLoaction[i]]=-1;
				distance[serverLoaction[i]]=0;
				q.push(serverLoaction[i]);
				inQueue[serverLoaction[i]]=true;
			}
		}
		while(!q.empty())  //求最小花费路径,从视频服务器节点到所有点的cost距离
		{
			cur=q.front();
			q.pop();
			inQueue[cur]=false;
			edges=edgeGraph[cur];
			for(i=0;i<edges->size();i++)
			{
				edge=(*edges)[i];
				if(edge->capicity>0)
				{
					if(edge->capicity>edge->capCopy) //其反方向已经有流,优先处理反向流的问题
					{
						edgeCost=-edge->cost;
					}else{
						edgeCost=edge->cost;
					}
					if(distance[edge->outVertex]>distance[cur]+edgeCost)
					{
						distance[edge->outVertex]=distance[cur]+edgeCost;
						pre[edge->outVertex]=cur;
						if(!inQueue[edge->outVertex])
						{
							inQueue[edge->outVertex]=true;
							q.push(edge->outVertex);
						}
					}
				}
			}
		}
		minCostVertexIdx=-1;
		minCostVertexval=0x7fffffff;
		for(i=0;i<g->consumerEdgeNum;i++)
		{
			cur=(g->consumerIdx)[i];
			if(distance[cur]<minCostVertexval&&demand[cur]>0)
			{
				minCostVertexIdx=cur;
				minCostVertexval=distance[cur];
			}
		}
		if(minCostVertexIdx!=-1)                 //找到一条增广路
		{
			currentDemand=demand[minCostVertexIdx];      //该增广路的最大可行流
			path.clear();
			path.push_back(minCostVertexIdx);
			cur=minCostVertexIdx;
			while(pre[cur]!=-1)
			{
				cur=pre[cur];
				path.push_back(cur);
			}
			currentDemand=min(currentDemand,supply[cur]);
			edgePath.clear();     //确定增广路所经过的边，并用来更新流量
			i=path.size()-1;
			cur=path[i];
			i-=1;
			next=path[i];
			i-=1;
			while(true)    //根据顶点编号来确定 边的路径指针数组
			{
				vector<Edge*>* &edges=edgeGraph[cur]; 
				for(j=0;j<edges->size();j++)
				{
					edge=(*edges)[j];
					if(edge->outVertex==next)
					{
						edgePath.push_back(edge);
						edgeCapicity=edge->capicity;
						if(edge->capicity>edge->capCopy)    //处理反向边的问题
							edgeCapicity=edgeCapicity-edge->capCopy;
						currentDemand=min(currentDemand,edgeCapicity);
						break;
					}
				}
				if(i>=0){
					cur=next;
					next=path[i];
					i-=1;
				}else{
					break;
				}
			}
			for(i=0;i<edgePath.size();i++)  //更新流
			{
				pe1=edgePath[i];
				pe2=pe1->reverseEdge;
				pe1->capicity-=currentDemand;
				pe2->capicity+=currentDemand;
			}
			demand[minCostVertexIdx]-=currentDemand;
			supply[path[path.size()-1]]-=currentDemand;
		}else  //判断当前 是否所有消费节点得到满足
		{
			flag=true;
			for(map<int,int>::iterator it=demand.begin();it!=demand.end();++it)
			{
				if(it->second>0)
				{
					flag=false;
					break;
				}
			}
			if(!flag)
				totalCost=-1;
			break;
		}
	}
	tmp=totalCost;
	for(j=0;j<edgeGraph.size();j++)  //每个网络节点的流
	{
		vector<Edge*>* &edges=edgeGraph[j];
		for(i=0;i<edges->size();i++)
		{
			pe1=(*edges)[i];
			pe2=pe1->reverseEdge;
			if(pe1->capicity<pe2->capicity)
			{
				cur=(pe2->capicity-pe1->capicity)/2;
				flow[j]+=cur;
				if(flag)
					totalCost+=cur*pe1->cost;
			}
		}
	}
	cout<<"流成本:"<<totalCost-tmp<<endl;
	if(serverType.size()!=0)  //服务器类型已经指定
	{
		for(i=0;i<serverLoaction.size();i++){
			cur=serverLoaction[i];
			inFlow[cur]=(g->serverSuply)[serverType[i]]-supply[cur];
			//cout<<"节点:"<<cur<<",flow:"<<(g->serverSuply)[serverType[i]]-supply[cur]<<"/"<<(g->serverSuply)[serverType[i]]<<endl;
		}
	}
	if(totalCost!=-1&&serverType.size()==0){  //前面服务器类型没有确定，按照最终提供的流量确定服务器类型
		serverType.resize(serverLoaction.size());
		for(i=0;i<serverLoaction.size();i++){
			cur=serverLoaction[i];
			inFlow[cur]=(g->serverSuply)[g->serverTypeNum-1]-supply[cur];  //总共花费的流量
			if(inFlow[cur]==0){                                            //不提供流量,也部署了服务器,默认最低级的
				serverType[i]=0;
				totalCost+=(g->serverCost)[0];
				continue;
			}
			for(j=0;j<g->serverTypeNum;j++){
				if((g->serverSuply)[j]>=inFlow[cur]){
					serverType[i]=j;
					totalCost+=(g->serverCost)[j];
					break;
				}
			}
		}
	}
	
	
	/*delete [] pre;
	delete [] distance;*/
	delete [] inQueue;
	return totalCost;
}

void IterationServer::keepg()
{
	vector<vector<Edge*>* >& edgeGraph=g->outEdge;
	vector<vector<Edge*>* >&tmp=(bestg->outEdge);
	int i,j;
	for(i=0;i<tmp.size();i++)
	{
		if(tmp[i]!=nullptr)
		{
			for(j=0;j<tmp[i]->size();j++)
			{
				delete tmp[i]->at(j);
			}
			tmp[i]->clear();
		}
	}
	for(j=0;j<edgeGraph.size();j++)  //获取每个链路的流，只保存不为0的
	{
		vector<Edge*>* &edges=edgeGraph[j];
		if(edges==nullptr)
			continue;
		for(i=0;i<edges->size();i++)
		{
			Edge *pe1=(*edges)[i];
			Edge *pe2=pe1->reverseEdge;
			if(pe1->capicity<pe2->capicity)
			{
				bestg->addEdge(j,pe1->outVertex,(pe2->capicity-pe1->capicity)/2);
			}
		}
	}
}

void IterationServer::printSolution(const vector<int>&sol,const vector<int>&serverType,vector<int> &inFlow,int*skip)
{
	int i;
	for(i=0;i<sol.size();i++)
		cout<<sol[i]<<" ";
	cout<<endl;
	for(i=0;i<serverType.size();i++)
		cout<<serverType[i]<<" ";
	cout<<endl;
	vector<int> con,nocon;
	for(i=0;i<sol.size();i++)
	{
		cout<<"("<<sol[i]<<","<<(g->serverCost)[serverType[i]]<<","<<inFlow[sol[i]]
		<<"/"<<(g->serverSuply)[serverType[i]]<<"/"<<(g->maxOutFlow)[sol[i]]<<","<<((g->outEdge)[sol[i]])->size()<<","<<skip[sol[i]]<<"), ";
		if((g->isConnectWithConsumer)[sol[i]])
			con.push_back(sol[i]);
		else
			nocon.push_back(sol[i]);
	}
	cout<<endl;
	cout<<"其中直连的节点有"<<con.size()<<"个,分别是 ";
	for(i=0;i<con.size();i++)
		cout<<con[i]<<",";
	cout<<endl;
	cout<<"不是直连的有"<<nocon.size()<<"个,分别是";
	for(i=0;i<nocon.size();i++)
		cout<<nocon[i]<<",";
	cout<<endl;
}

//按照流从小到大排序
void IterationServer::sortByFlow(vector<int> &sol,vector<int>&flow,vector<sortHelper<int> >&sorts)
{
	int i,idx;
	sorts.clear();
	for(i=0;i<sol.size();i++){
		idx=sol[i];
		sorts.push_back(sortHelper<int>(idx,flow[idx]));
	}
	sort(sorts.begin(),sorts.end(),compare);
	sol.clear();
	for(i=0;i<sorts.size();i++)
		sol.push_back(sorts[i].id);
}

int IterationServer::runAndSearchServerType(vector<int>&sol,vector<int> &inFlow,vector<int>& oldServerType,vector<int>& newServerType,int curCost)
{
	int i,t,idx,cost;
	float p;
	bool flag=false;
	vector<int> flow;
	map<int,int> demand;
	if(oldServerType.size()==0)         //该解还没测试布置服务器的初始类型
		curCost=runFlow(sol,oldServerType,flow,inFlow,demand);
	newServerType.clear();
	newServerType.insert(newServerType.begin(),oldServerType.begin(),oldServerType.end());
	float minp;
	while(true){
		minp=100;
		for(i=0;i<newServerType.size();i++)
		{
			t=newServerType[i];
			if(t==0){
				p=0.8;   //已经是最低，不做调整
			}else{
				p=(inFlow[sol[i]]-(g->serverSuply)[t-1])*1.0/((g->serverSuply)[t]-(g->serverSuply)[t-1]);
				if(p<minp){
					minp=p;
					idx=i;
				}
			}
		}
		if(minp<0.6){   //尝试对一个服务器进行降级
			newServerType[idx]-=1;
			cost=runFlow(sol,newServerType,flow,inFlow,demand);
			if(cost!=-1&&cost<curCost){    //降级后解变好了
				curCost=cost;
			}else{
				newServerType[idx]+=1;  //不变好,恢复原来的类型
				return curCost;
			}
		}else{
			return curCost;
		}
	}
}

//通过提供的流确定服务器类型,使得服务器可提供流量大于等于提供的流量
void IterationServer::getServerTypeByFlow(const vector<int>&inFlow,vector<int> &serverType)
{
	int i,j;
	if(serverType.size()<inFlow.size())
		serverType.resize(inFlow.size());
	for(i=0;i<inFlow.size();i++)
	{
		for(j=0;j<g->serverTypeNum;j++){
			if((g->serverSuply)[j]>=inFlow[i]){
				serverType[i]=j;
				break;
			}
		}
	}
}

bool IterationServer::adjustServerType(const vector<int>&sol,vector<int> inFlow,const vector<int>& oldServerType,vector<int>& newServerType)
{
	int i,t;
	float p;
	bool flag=false;
	if(newServerType.size()<oldServerType.size())
		newServerType.resize(oldServerType.size());
	for(i=0;i<oldServerType.size();i++)
	{
		t=oldServerType[i];
		if(t==0){
			p=0.5;   //已经是最低，不做调整
		}else{
			p=(inFlow[sol[i]]-(g->serverSuply)[t-1])*1.0/((g->serverSuply)[t]-(g->serverSuply)[t-1]);
		}
		if(p<0.5){
			newServerType[i]=t-1;
			flag=true;
		}else{
			newServerType[i]=t;
		}
	}
	return flag;
}

//尝试寻找一条增光路径
//beginId 开始增光的初始节点，该点必须还可以提供流量，可能是某一个服务器节点，supply视频节点还可以提供的流量值
//
bool IterationServer::Aug(const vector<int>& sol,const vector<int>& initSupply,vector<int>& supply,const vector<int>&dis,vector<vector<Edge*>* > &outEdge, bool*isVisit,map<int,int> &demand)
{
	int i,j,cur,flow;
	bool flag=false;
	vector<int> used(sol.size(),0);
	for(i=0;i<sol.size();i++){   //计算每个视频服务器已经提供的流量
		j=sol[i];
		used[i]=initSupply[j]-supply[j];
	}
	vector<sortHelper<int> >sorts;
	for(i=0;i<used.size();i++)
	{
		cur=used[i];
		if(cur==0){
			sorts.push_back(sortHelper<int>(sol[i],-1e6));
			continue;
		}
		for(j=0;j<g->serverTypeNum;j++)
		{
			if(cur<=(g->serverCost)[j])
			{
				sorts.push_back(sortHelper<int>(sol[i],cur-(g->serverCost)[j]));
				break;
			}
		}
	}
	sort(sorts.begin(),sorts.end(),compare);  //按照以现有流量计算服务器,剩余流量多的优先
	for(i=0;i<sorts.size();i++)
	{
		cur=sorts[i].id;
		if(supply[cur]==0)   //该视频服务器已经不提供流，无法增广
			continue;
		flow=supply[cur];
		flow=Aug(cur,flow,dis,outEdge,isVisit,demand);
		if(flow>0){
			supply[cur]-=flow;
			flag=true;
		}
	}
	return flag;
}
//从节点beginIdx开始增广
int IterationServer::Aug(int beginIdx,int flow,const vector<int>&dis,vector<vector<Edge*>* > &outEdge, bool*isVisit,map<int,int> &demand)
{
	int i,j,tmp,tmp2,cur=beginIdx;
	if((g->isConnectWithConsumer)[cur]&&demand[cur]>0){
		flow=min(flow,demand[cur]);
		demand[cur]-=flow;
		return flow;
	}
	vector<Edge*>*pedges=outEdge[cur];
	Edge*pedge;
	int leftFlow = flow;
	isVisit[cur]=true;
	for(i=0;i<pedges->size();i++)
	{
		pedge=pedges->at(i);
		tmp=pedge->capicity;        //识别反向边
		tmp2=pedge->cost;
		if(pedge->capicity>pedge->capCopy){
			tmp=tmp-pedge->capCopy;
			tmp2=-pedge->cost;
		}
		if(dis[cur]==dis[pedge->outVertex]+tmp2&&pedge->capicity>0&&!isVisit[pedge->outVertex])//满足距离条件,流没有满，没有访问
		//if(!pedge->cost&&pedge->capicity&&!isVisit[pedge->outVertex])
		{
			int delta=Aug(pedge->outVertex,min(leftFlow,tmp),dis,outEdge,isVisit,demand);
			pedge->capicity-=delta;
			pedge->reverseEdge->capicity+=delta;
			leftFlow-=delta;
			if(leftFlow==0){
				return flow;
			}
		}
	}
	return flow-leftFlow;
}

//修改距离标注
bool IterationServer::modlabel(vector<int> &dis,vector<vector<Edge*>* > &outEdge,bool*isVisit)
{
	int i,j;
	vector<Edge*>* pedges;
	Edge*pedge;
	int d=0x7fffffff;
	for(i=0;i<outEdge.size();i++)
	{
		if(isVisit[i])
		{
			pedges=outEdge[i];
			for(j=0;j<pedges->size();j++)
			{
				pedge=pedges->at(j);
				if(!isVisit[pedge->outVertex]&&pedge->capicity>0){
					if(pedge->capicity<=pedge->capCopy)
						d=min(d,dis[pedge->outVertex]+pedge->cost-dis[i]);
					else
						d=min(d,dis[pedge->outVertex]-pedge->cost-dis[i]);  //反向边
				}
			}
		}
	}
	if(d<0x7fffffff){              //出现d=0
		if(d==0){
			cout<<"被访问";
			for(i=0;i<outEdge.size();i++)
				if(isVisit[i])
					cout<<i<<",";
			cout<<endl;
			cout<<"未被访问";
			for(i=0;i<outEdge.size();i++)
				if(!isVisit[i])
					cout<<i<<",";
			cout<<endl;
			cout<<"8888888888888888888888888888888888888888888888888888888888888888888"<<endl;
			for(i=0;i<outEdge.size();i++)
			{
					pedges=outEdge[i];
					for(j=0;j<pedges->size();j++)
					{
						pedge=pedges->at(j);
						if(pedge->capicity>0&&dis[i]==dis[pedge->outVertex]+pedge->cost){
							cout<<i<<";"<<pedge->outVertex<<endl;
							//cout<<"("<<i<<","<<pedge->outVertex<<")="<<dis[i]<<","<<dis[pedge->outVertex]<<","<<pedge->cost<<endl;
						}
					}
			}
			cout<<"8888888888888888888888888888888888888888888888888888888888888888888"<<endl;
		}
		for(i=0;i<outEdge.size();i++)
			if(isVisit[i]){
				dis[i]+=d;
				/*pedges=outEdge[i];
				for(j=0;j<pedges->size();j++)
				{
				pedge=pedges->at(j);
				pedge->cost-=d;
				pedge->reverseEdge->cost+=d;
				}*/
			}
		return true;
	}
	return false;
}

int IterationServer::minCostFlowZKW(vector<int> &serverLoaction,vector<int>& serverType,vector<int> &flow,vector<int>& inFlow, vector<vector<Edge*>* >& edgeGraph,map<int,int>&demand)
{
	int i,j,cur,tmp;
	int totalCost=0;
	Edge *pe1,*pe2;
	vector<int> supply,initSupply;      //serverLoaction[i] 剩余的流提供能力,不是服务器位置该值为0
	if(flow.size()<g->netNodeNum)
		flow.resize(g->netNodeNum);
	if(inFlow.size()<g->netNodeNum)
		inFlow.resize(g->netNodeNum);
	supply.resize(g->netNodeNum);
	initSupply.resize(g->netNodeNum);
	bool *isServerLocation=new bool[g->netNodeNum];  //该位置 是否部署服务器
	for(i=0;i<g->netNodeNum;i++){
		inFlow[i]=flow[i]=supply[i]=0;
		isServerLocation[i]=false;
	}
	if(serverType.size()==0){  //服务器型号未定,默认都是流量最大的型号,也就是最后一个型号
		for(i=0;i<serverLoaction.size();i++){
			initSupply[serverLoaction[i]]=supply[serverLoaction[i]]=(g->serverSuply)[g->serverTypeNum-1];
		}
	}else{
		for(i=0;i<serverLoaction.size();i++){
			initSupply[serverLoaction[i]]=supply[serverLoaction[i]]=(g->serverSuply)[serverType[i]];
			totalCost+=(g->serverCost)[serverType[i]];
		}
	}
	for(i=0;i<serverLoaction.size();i++){      //服务器部署成本
		isServerLocation[serverLoaction[i]]=true;
		totalCost+=(g->deployCost)[serverLoaction[i]];
	}
	demand.clear();
	for(i=0;i<g->consumerEdgeNum;i++)
	{
		cur=(g->consumerIdx)[i];
		tmp=(g->consumerEdge)[cur]->capicity;
		demand[cur]=tmp;
		if(isServerLocation[cur]){           //消费节点旁有服务器,优先满足旁边的消费节点
			tmp=min(supply[cur],tmp);
			flow[cur]+=tmp;
			demand[cur]-=tmp;
			supply[cur]-=tmp;
		}
	}
	//-----------------------前面部分直接复制 spfa
	bool *isVisit=new bool[g->netNodeNum];
	vector<int> dis(g->netNodeNum,0);
	

	////----------------------------------------begin spfa ------开始是求一个距离，保证zkw费用流初始就能找到最短路
	//bool* inQueue=isServerLocation;
	//queue<int> q;
	//vector<Edge*>*edges;
	//Edge *edge;
	//for(i=0;i<g->netNodeNum;i++)
	//{
	//	dis[i]=0x7fffffff;
	//	inQueue[i]=false;
	//}
	//for(i=0;i<g->consumerEdgeNum;i++)   //将所有需求没有得到满足的 与消费节点直连的点进入队列
	//{
	//	cur=(g->consumerIdx)[i];
	//	if(demand[cur]>0)
	//	{
	//		dis[cur]=0;
	//		q.push(cur);
	//		inQueue[cur]=true;
	//	}
	//}
	//while(!q.empty())  //求最小花费路径,从消费节点到所有点的cost距离
	//{
	//	cur=q.front();
	//	q.pop();
	//	inQueue[cur]=false;
	//	edges=edgeGraph[cur];
	//	for(i=0;i<edges->size();i++)
	//	{
	//		edge=(*edges)[i];
	//		if(dis[edge->outVertex]>dis[cur]+edge->cost)
	//		{
	//			dis[edge->outVertex]=dis[cur]+edge->cost;
	//			if(!inQueue[edge->outVertex])
	//			{
	//				inQueue[edge->outVertex]=true;
	//				q.push(edge->outVertex);
	//			}
	//		}
	//	}
	//}
	//------------------------end spfa------------------------------------------ //加上后，速度反而变慢


	do {
		do {
			fill(isVisit,isVisit+g->netNodeNum,false);
		} while (Aug(serverLoaction,initSupply,supply,dis,edgeGraph,isVisit,demand));
	} while (modlabel(dis,edgeGraph,isVisit));
	//---------------------后面也直接复制    spfa
	delete [] isVisit;
	bool flag=true;
	for(map<int,int>::iterator it=demand.begin();it!=demand.end();++it)
	{
		if(it->second>0)
		{
			flag=false;
			break;
		}
	}
	if(!flag)
		return -1;
	for(j=0;j<edgeGraph.size();j++)  //每个网络节点的流
	{
		vector<Edge*>* &edges=edgeGraph[j];
		for(i=0;i<edges->size();i++)
		{
			pe1=(*edges)[i];
			pe2=pe1->reverseEdge;
			if(pe1->capicity<pe2->capicity)
			{
				cur=(pe2->capicity-pe1->capicity)/2;
				flow[j]+=cur;
				if(flag){
					totalCost+=cur*pe1->cost;
				}
			}
		}
	}
	if(serverType.size()!=0)  //服务器类型已经指定
	{
		for(i=0;i<serverLoaction.size();i++){
			cur=serverLoaction[i];
			inFlow[cur]=(g->serverSuply)[serverType[i]]-supply[cur];
			//cout<<"节点:"<<cur<<",flow:"<<(g->serverSuply)[serverType[i]]-supply[cur]<<"/"<<(g->serverSuply)[serverType[i]]<<endl;
		}
	}
	if(totalCost!=-1&&serverType.size()==0){  //前面服务器类型没有确定，按照最终提供的流量确定服务器类型
		serverType.resize(serverLoaction.size());
		for(i=0;i<serverLoaction.size();i++){
			cur=serverLoaction[i];
			inFlow[cur]=(g->serverSuply)[g->serverTypeNum-1]-supply[cur];  //总共花费的流量
			if(inFlow[cur]==0){                                            //不提供流量,也部署了服务器,默认最低级的
				serverType[i]=0;
				totalCost+=(g->serverCost)[0];
				continue;
			}
			for(j=0;j<g->serverTypeNum;j++){
				if((g->serverSuply)[j]>=inFlow[cur]){
					serverType[i]=j;
					totalCost+=(g->serverCost)[j];
					break;
				}
			}
		}
	}
	return totalCost;
}
