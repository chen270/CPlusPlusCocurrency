#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <stdexcept>

//清单 3.1 使用互斥锁保护链表
#include <list>
#include <mutex>
#include <algorithm>
std::list<int> some_list; // 1
std::mutex some_mutex; // 2
void add_to_list(int new_value)
{
	std::lock_guard<std::mutex> guard(some_mutex); // 3
	some_list.push_back(new_value);
}
bool list_contains(int value_to_find)
{
	std::lock_guard<std::mutex> guard(some_mutex); // 4
	return std::find(some_list.begin(), some_list.end(), value_to_find) !=
		some_list.end();
}


//清单 3.2 无意中传递了被保护数据的引用
#if 0
class some_data
{
	int a;
	std::string b;
public:
	void do_something();
};
class data_wrapper
{
private:
	some_data data;
	std::mutex m;
public:
	template<typename Function>
	void process_data(Function func)
	{
		std::lock_guard<std::mutex> l(m);
		func(data); // 1 传递“被保护的”数据给用户函数
	}
};
some_data* unprotected;
void malicious_function(some_data& protected_data)
{
	unprotected = &protected_data;
}
data_wrapper x;
void foo()
{
	x.process_data(malicious_function); // 2 传递一个恶意函数
	unprotected->do_something(); // 3 在无保护的情况下访问保护数据
}
#endif

//清单 3.3 std::stack 容器适配器接口
template<typename T, typename Container = std::deque<T> >
class stack
{
public:
	explicit stack(const Container&);
	explicit stack(Container && = Container());
	template <class Alloc> explicit stack(const Alloc&);
	template <class Alloc> stack(const Container&, const Alloc&);
	template <class Alloc> stack(Container&&, const Alloc&);
	template <class Alloc> stack(stack&&, const Alloc&);
	bool empty() const;
	size_t size() const;
	T& top();
	T const& top() const;
	void push(T const&);
	void push(T&&);
	void pop();
	void swap(stack&&);
	template <class... Args> void emplace(Args&&... args); // C++14 的新特性
};


//清单 3.4 线程安全栈的轮廓
#if 0
#include <exception>
#include <memory> // For std::shared_ptr<>
struct empty_stack : std::exception
{
	const char* what() const throw();
};
template<typename T>
class threadsafe_stack
{
public:
	threadsafe_stack();
	threadsafe_stack(const threadsafe_stack&);
	threadsafe_stack& operator=(const threadsafe_stack&) = delete; // 1 赋值操作被删除
		void push(T new_value);
	std::shared_ptr<T> pop();
	void pop(T& value);
	bool empty() const;
};
#endif


//清单 3.5 完整线程安全栈定义
#include <exception>
#include <memory>
#include <mutex>
#include <stack>
struct empty_stack : std::exception
{
	const char* what() const throw() {
		return "empty stack!";
	};
};
template<typename T>
class threadsafe_stack
{
private:
	std::stack<T> data;
	mutable std::mutex m;
public:
	threadsafe_stack()
		: data(std::stack<T>()) {}
	threadsafe_stack(const threadsafe_stack& other)
	{
		std::lock_guard<std::mutex> lock(other.m);
		data = other.data; // 1 在构造函数体中执行拷贝
	}
	threadsafe_stack& operator=(const threadsafe_stack&) = delete;
	void push(T new_value)
	{
		std::lock_guard<std::mutex> lock(m);
		data.push(new_value);
	}
	std::shared_ptr<T> pop()
	{
		std::lock_guard<std::mutex> lock(m);
		if (data.empty()) throw empty_stack(); // 在调用 pop 前，检查栈是否为空
		std::shared_ptr<T> const res(std::make_shared<T>(data.top())); // 在修改栈前，分配出返回值
			data.pop();
		return res;
	}
	void pop(T& value)
	{
		std::lock_guard<std::mutex> lock(m);
		if (data.empty()) throw empty_stack();
		value = data.top();
		data.pop();
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lock(m);
		return data.empty();
	}
};

//清单 3.6 交换操作中使用 std::lock()和 std::lock_guard
class some_big_object {};
void swap(some_big_object& lhs, some_big_object& rhs);
class X
{
private:
	some_big_object some_detail;
	std::mutex m;
public:
	X(some_big_object const& sd) :some_detail(sd) {}
	friend void swap(X& lhs, X& rhs)
	{
		if (&lhs == &rhs)
			return;
		std::lock(lhs.m, rhs.m); // 1
		std::lock_guard<std::mutex> lock_a(lhs.m, std::adopt_lock); // 2
		std::lock_guard<std::mutex> lock_b(rhs.m, std::adopt_lock); // 3
		swap(lhs.some_detail, rhs.some_detail);
	}
};


//清单 3.7 使用层次锁来避免死锁
hierarchical_mutex high_level_mutex(10000); // 1
hierarchical_mutex low_level_mutex(5000); // 2
hierarchical_mutex other_mutex(6000); // 3
int do_low_level_stuff();
int low_level_func()
{
	std::lock_guard<hierarchical_mutex> lk(low_level_mutex); // 4
	return do_low_level_stuff();
}
void high_level_stuff(int some_param);
void high_level_func()
{
	std::lock_guard<hierarchical_mutex> lk(high_level_mutex); // 6
	high_level_stuff(low_level_func()); // 5
}
void thread_a() // 7
{
	high_level_func();
}
void do_other_stuff();
void other_stuff()
{
	high_level_func(); // 10
	do_other_stuff();
}
void thread_b() // 8
{
	std::lock_guard<hierarchical_mutex> lk(other_mutex); // 9
	other_stuff();
}


//清单 3.8 简单的层级互斥锁实现
class hierarchical_mutex
{
	std::mutex internal_mutex;
	unsigned long const hierarchy_value;
	unsigned long previous_hierarchy_value;
	static thread_local unsigned long this_thread_hierarchy_value; // 1
	void check_for_hierarchy_violation()
	{
		if (this_thread_hierarchy_value <= hierarchy_value) // 2
		{
			throw std::logic_error("mutex hierarchy violated");
		}
	}
	void update_hierarchy_value()
	{
		previous_hierarchy_value = this_thread_hierarchy_value; // 3
		this_thread_hierarchy_value = hierarchy_value;
	}
public:
	explicit hierarchical_mutex(unsigned long value) :
		hierarchy_value(value),
		previous_hierarchy_value(0)
	{}
	void lock()
	{
		check_for_hierarchy_violation();
		internal_mutex.lock(); // 4
		update_hierarchy_value(); // 5
	}
	void unlock()
	{
		if (this_thread_hierarchy_value != hierarchy_value)
			throw std::logic_error("mutex hierarchy violated"); // 9
		this_thread_hierarchy_value = previous_hierarchy_value; // 6
		internal_mutex.unlock();
	}
	bool try_lock()
	{
		check_for_hierarchy_violation();
		if (!internal_mutex.try_lock()) // 7
			return false;
		update_hierarchy_value();
		return true;
	}
};
thread_local unsigned long
hierarchical_mutex::this_thread_hierarchy_value(ULONG_MAX); // 8

//清单 3.9 交换操作中 std::lock()和 std::unique_lock 的使用
class some_big_object;
void swap(some_big_object& lhs, some_big_object& rhs);
class X
{
private:
	some_big_object some_detail;
	std::mutex m;
public:
	X(some_big_object const& sd) :some_detail(sd) {}
	friend void swap(X& lhs, X& rhs)
	{
		if (&lhs == &rhs)
			return;
		std::unique_lock<std::mutex> lock_a(lhs.m, std::defer_lock); // 1
		std::unique_lock<std::mutex> lock_b(rhs.m, std::defer_lock); // std::defer_lock 让互斥锁保持未上锁状态
		std::lock(lock_a, lock_b); // 2 互斥锁在这里上锁
		swap(lhs.some_detail, rhs.some_detail);
	}
};

//清单 3.10 比较操作符中一次锁住一个互斥锁
class Y
{
private:
	int some_detail;
	mutable std::mutex m;
	int get_detail() const
	{
		std::lock_guard<std::mutex> lock_a(m); // 1
		return some_detail;
	}
public:
	Y(int sd) :some_detail(sd) {}
	friend bool operator==(Y const& lhs, Y const& rhs)
	{
		if (&lhs == &rhs)
			return true;
		int const lhs_value = lhs.get_detail(); // 2
		int const rhs_value = rhs.get_detail(); // 3
		return lhs_value == rhs_value; // 4
	}
};




int main()
{
    return 0;
}