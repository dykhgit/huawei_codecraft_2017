#pragma once
template<class T>
class sortHelper
{
public:
	int id;
	T count;
	sortHelper();
	sortHelper(int id,T count);
	~sortHelper();
	bool operator < (const sortHelper<T> &y) const ;
};


template<class T> sortHelper<T>::sortHelper()
{
}

template<class T> sortHelper<T>::sortHelper(int id,T count)
{
	this->id=id;
	this->count=count;
}

template<class T>sortHelper<T>::~sortHelper(){}

template<class T> bool sortHelper<T>::operator<(const sortHelper<T> &y) const
{
	return this->count< y.count;
}


