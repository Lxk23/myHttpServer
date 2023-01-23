#ifndef _SINGLETON_H_
#define _SINGLETON_H_

template<class T, class X = void, int N = 0>
class Singleton
{
public:
	static T* getInstance() {
		static T v;
		return &v;
	}
};

template<class T, class X = void, int N = 0>
class SingletonPtr 
{
public:
	static std::shared_ptr<T> getInstance() {
		static std::shared_ptr<T> v(new T);
		return v;
	}
};


#endif
