#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <queue>
#include <condition_variable>
#include <future>
#include "windows.h"
#include <atomic>
#include <stdio.h>
#include <string.h>
//#include <execution>

#if 0
/* 定义简单的结构 */
struct
{
	unsigned int widthValidated;
	unsigned int heightValidated;
} status1;

/* 定义位域结构 */
struct
{
	int widthValidated : 8;
	int heightValidated : 25;
} status2;

std::vector<int> data;
std::atomic<bool> data_ready(false);
void reader_thread()
{
	long long i = 0;
	while (!data_ready.load()) // 1
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		++i;
	}
	std::cout << "The answer=" << data[0] << ", i = " << i << "\n"; // 2
}
void writer_thread()
{
	data.push_back(42); // 3
	data_ready = true; // 4
}

//清单 5.4 序列一致意味着全序
#include <atomic>
#include <thread>
#include <assert.h>
std::atomic<bool> x, y;
std::atomic<int> z;
void write_x()
{
	x.store(true, std::memory_order_seq_cst); // 1
}
void write_y()
{
	y.store(true, std::memory_order_seq_cst); // 2
}

void read_x_then_y(){
	while (!x.load(std::memory_order_seq_cst));
	if (y.load(std::memory_order_seq_cst)) // 3
		++z;
}

void read_y_then_x(){
	while (!y.load(std::memory_order_seq_cst));
	if (x.load(std::memory_order_seq_cst)) // 4
		++z;
}

int main(){
	x = false;
	y = false;
	z = 0;
	std::thread a(write_x);
	std::thread b(write_y);
	std::thread c(read_x_then_y);
	std::thread d(read_y_then_x);
	a.join();
	b.join();
	c.join();
	d.join();
	assert(z.load() != 0); // 5
}


//清单 5.5 宽松操作只有很少的顺序要求
#include <atomic>
#include <thread>
#include <assert.h>
std::atomic<bool> x, y;
std::atomic<int> z;
void write_x_then_y()
{
	x.store(true, std::memory_order_relaxed); // 1
	y.store(true, std::memory_order_relaxed); // 2
}
void read_y_then_x()
{
	while (!y.load(std::memory_order_relaxed)); // 3
	if (x.load(std::memory_order_relaxed)) // 4
		++z;
}
int main()
{
	x = false;
	y = false;
	z = 0;
	std::thread a(write_x_then_y);
	std::thread b(read_y_then_x);
	a.join();
	b.join();
	assert(z.load() != 0); // 5
}


//清单 5.6 多线程版上的宽松操作
#include <thread>
#include <atomic>
#include <iostream>
std::atomic<int> x(0), y(0), z(0); // 1
std::atomic<bool> go(false); // 2
unsigned const loop_count = 10;
struct read_values
{
	int x, y, z;
};
read_values values1[loop_count];
read_values values2[loop_count];
read_values values3[loop_count];
read_values values4[loop_count];
read_values values5[loop_count];
void increment(std::atomic<int>* var_to_inc, read_values* values)
{
	while (!go)
		std::this_thread::yield(); // 3 自旋，等待信号
	for (unsigned i = 0; i < loop_count; ++i)
	{
		values[i].x = x.load(std::memory_order_relaxed);
		values[i].y = y.load(std::memory_order_relaxed);
		values[i].z = z.load(std::memory_order_relaxed);
		var_to_inc->store(i + 1, std::memory_order_relaxed); // 4
		std::this_thread::yield();
	}
}
void read_vals(read_values* values)
{
	while (!go)
		std::this_thread::yield(); // 5 自旋，等待信号
	for (unsigned i = 0; i < loop_count; ++i)
	{
		values[i].x = x.load(std::memory_order_relaxed);
		values[i].y = y.load(std::memory_order_relaxed);
		values[i].z = z.load(std::memory_order_relaxed);
		std::this_thread::yield();
	}
}
void print(read_values* v)
{
	for (unsigned i = 0; i < loop_count; ++i)
	{
		if (i)
			std::cout << ",";
		std::cout << "(" << v[i].x << "," << v[i].y << "," << v[i].z << ")";
	}
	std::cout << std::endl;
}
int main()
{
	std::thread t1(increment, &x, values1);
	std::thread t2(increment, &y, values2);
	std::thread t3(increment, &z, values3);
	std::thread t4(read_vals, values4);
	std::thread t5(read_vals, values5);
	go = true; // 6 开始执行主循环的信号
	t5.join();
	t4.join();
	t3.join();
	t2.join();
	t1.join();
	print(values1); // 7 打印最终结果
	print(values2);
	print(values3);
	print(values4);
	print(values5);
}



//清单 5.7 “获得 - 释放”并不意味着全序
#include <atomic>
#include <thread>
#include <assert.h>
std::atomic<bool> x, y;
std::atomic<int> z;
void write_x()
{
	x.store(true, std::memory_order_release);
}
void write_y()
{
	y.store(true, std::memory_order_release);
}
void read_x_then_y()
{
	while (!x.load(std::memory_order_acquire));
	if (y.load(std::memory_order_acquire)) // 1
		++z;
}
void read_y_then_x() {
	while (!y.load(std::memory_order_acquire));
	if (x.load(std::memory_order_acquire)) // 2
		++z;
}
int main()
{
	x = false;
	y = false;
	z = 0;
	std::thread a(write_x);
	std::thread b(write_y);
	std::thread c(read_x_then_y);
	std::thread d(read_y_then_x);
	a.join();
	b.join();
	c.join();
	d.join();
	assert(z.load() != 0); // 3
}

//清单 5.8 “获得 - 释放”操作可以在宽松操作上强加顺序
#include <atomic>
#include <thread>
#include <assert.h>
std::atomic<bool> x, y;
std::atomic<int> z;
void write_x_then_y()
{
	x.store(true, std::memory_order_relaxed); // 1
	y.store(true, std::memory_order_release); // 2
}
void read_y_then_x()
{
	while (!y.load(std::memory_order_acquire)); // 3 自旋，等待 y 被设置为
	true
		if (x.load(std::memory_order_relaxed)) // 4
			++z;
}
int main()
{
	x = false;
	y = false;
	z = 0;
	std::thread a(write_x_then_y);
	std::thread b(read_y_then_x);
	a.join();
	b.join();
	assert(z.load() != 0); // 5
}


//清单 5.9 使用获得和释放顺序传递同步
std::atomic<int> data[5];
std::atomic<bool> sync1(false), sync2(false);
void thread_1()
{
	data[0].store(42, std::memory_order_relaxed);
	data[1].store(97, std::memory_order_relaxed);
	data[2].store(17, std::memory_order_relaxed);
	data[3].store(-141, std::memory_order_relaxed);
	data[4].store(2003, std::memory_order_relaxed);
	sync1.store(true, std::memory_order_release); // 1.设置 sync1
}
void thread_2()
{
	while (!sync1.load(std::memory_order_acquire)); // 2.循环直到 sync1 被设置
		sync2.store(true, std::memory_order_release); // 3.设置 sync2
}
void thread_3()
{
	while (!sync2.load(std::memory_order_acquire)); // 4.循环直到 sync2 被设置
		assert(data[0].load(std::memory_order_relaxed) == 42);
	assert(data[1].load(std::memory_order_relaxed) == 97);
	assert(data[2].load(std::memory_order_relaxed) == 17);
	assert(data[3].load(std::memory_order_relaxed) == -141);
	assert(data[4].load(std::memory_order_relaxed) == 2003);
}


std::atomic<int> sync(0);
void thread_1()
{
	// ...
	sync.store(1, std::memory_order_release);
}
void thread_2()
{
	int expected = 1;
	while (!sync.compare_exchange_strong(expected, 2,
		std::memory_order_acq_rel))
		expected = 1;
}
void thread_3()
{
	while (sync.load(std::memory_order_acquire) < 2);
	// ...
}

//清单 5.12 宽松操作可以使用栅栏排好顺序
#include <atomic>
#include <thread>
#include <assert.h>
std::atomic<bool> x, y;
std::atomic<int> z;
void write_x_then_y()
{
	x.store(true, std::memory_order_relaxed); // 1
	std::atomic_thread_fence(std::memory_order_release); // 2
	y.store(true, std::memory_order_relaxed); // 3
}
void read_y_then_x()
{
	while (!y.load(std::memory_order_relaxed)); // 4
	std::atomic_thread_fence(std::memory_order_acquire); // 5
	if (x.load(std::memory_order_relaxed)) // 6
		++z;
}
int main()
{
	x = false;
	y = false;
	z = 0;
	std::thread a(write_x_then_y);
	std::thread b(read_y_then_x);
	a.join();
	b.join();
	assert(z.load() != 0); // 7
}
#endif

#if 0
//两种初始化方式
std::atomic<const char*>m_name = ATOMIC_VAR_INIT(nullptr);
std::atomic<int>m_property(0);
bool exitFlag = true;
void threadRun()
{
	while (exitFlag)
	{
		if (m_name.exchange(nullptr, std::memory_order_acq_rel) != nullptr)//2
		{
			m_property.store(23333,std::memory_order_release);//3
			//break;
			//std::cout << "get" << std::endl;
		}
	}
}

int main()
{
	std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < 10000; ++i)
	{
		std::thread g1(threadRun);

		m_name.store("aid", std::memory_order_release);//1
		int res = 0;
		while ((res = m_property.load(std::memory_order_acquire)) == 0);//4

		if (g1.joinable()) {
			exitFlag = false;
			g1.join();
		}
	}

	auto ends = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::ratio<1, 1>> duration_s(ends - start);
	std::cout << "Running Time : " << duration_s.count() / 1.0000 << " seconds" << std::endl;
}
#endif

#if 0
//std::atomic<const char*>m_name = ATOMIC_VAR_INIT(nullptr);
std::string m_name;
std::atomic<int>m_property(0);
bool exitFlag = true;
void threadRun()
{
	while (exitFlag)
	{
		if (!m_name.empty())//2
		{
			m_property.store(23333, std::memory_order_release);//3
			m_name.clear();
			//break;
			//std::cout << "get" << std::endl;
		}
	}
}

#include <chrono>

int main()
{
	std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();

	for (int i = 0; i < 10000; ++i)
	{
		m_name.clear();
		std::thread g1(threadRun);
		m_name = "aid";// +i + 3;
		//m_name.store("aid", std::memory_order_release);//1
		int res = 0;
		while ((res = m_property.load(std::memory_order_acquire)) == 0);//4
		//std::cout << res << std::endl;

		if (g1.joinable()) {
			exitFlag = false;
			g1.join();
		}
	}

	auto ends = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::ratio<1, 1>> duration_s(ends - start);
	std::cout << "Running Time : " << duration_s.count() / 1.0 << " seconds" << std::endl;
}
#endif


#if 0
std::string m_name;
int m_property = 0;
bool exitFlag = true;
void threadRun()
{
	while (exitFlag)
	{
		if (!m_name.empty())//2
		{
			//m_property.store(23333, std::memory_order_release);//3
			m_property = 23333;
			m_name.clear();
			//break;
			//std::cout << "get" << std::endl;
		}
	}
}


using namespace std;
int THreadRun() {
	cout << "thread id = " << std::this_thread::get_id() << endl;
	return 70;
}

void ThreadTest() {
	cout << "main id = " << std::this_thread::get_id() << endl;
	std::packaged_task<int()>task(THreadRun);
	auto fu = task.get_future();
	cout << "fu is defined" << endl;

#if 1
	std::thread t1(std::ref(task));
	t1.join();
#elif 0
	std::async(std::ref(task));
#elif 0
	task();
#endif

	cout << fu.get() << endl; // 70
	return;
}


int main(){

	std::promise<int>f1;

	vector<std::promise<int>>vec;

	vec.emplace_back(std::move(f1));
	//ThreadTest();

	vec.clear();
	return 0;
}

#endif

#if 0
//清单 6.2 使用条件变量实现的线程安全队列
//#include <exception>
#include <stack>

template<typename T>
class threadsafe_queue
{
private:
	mutable std::mutex mut;
	std::queue<T> data_queue;
	std::condition_variable data_cond;
public:
	threadsafe_queue()
	{}
	void push(T data)
	{
		std::lock_guard<std::mutex> lk(mut);
		data_queue.push(std::move(data));
		data_cond.notify_one(); // 1
	}
	void wait_and_pop(T& value) // 2
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		value = std::move(data_queue.front());
		data_queue.pop();
	}
	std::shared_ptr<T> wait_and_pop() // 3
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		std::shared_ptr<T> res(
			std::make_shared<T>(std::move(data_queue.front()))); // 4
		data_queue.pop();
		return res;
	}
	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return false;
		value = std::move(data_queue.front());
		data_queue.pop();
		return true;
	}
	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return std::shared_ptr<T>(); // 5
		std::shared_ptr<T> res(
			std::make_shared<T>(std::move(data_queue.front())));
		data_queue.pop();
		return res;
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lk(mut);
		return data_queue.empty();
	}
};

template<typename T>
class threadsafe_queue
{
private:
	mutable std::mutex mut;
	std::queue<std::shared_ptr<T> > data_queue;
	std::condition_variable data_cond;
public:
	threadsafe_queue()
	{}
	void wait_and_pop(T& value)
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		value = std::move(*data_queue.front()); // 1
		data_queue.pop();
	}
	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return false;
		value = std::move(*data_queue.front()); // 2
		data_queue.pop();
		return true;
	}
	std::shared_ptr<T> wait_and_pop()
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		std::shared_ptr<T> res = data_queue.front(); // 3
		data_queue.pop();
		return res;
	}
	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return std::shared_ptr<T>();
		std::shared_ptr<T> res = data_queue.front(); // 4
		data_queue.pop();
		return res;
	}
	void push(T new_value)
	{
		std::shared_ptr<T> data(
			std::make_shared<T>(std::move(new_value))); // 5
		std::lock_guard<std::mutex> lk(mut);
		data_queue.push(data);
		data_cond.notify_one();
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lk(mut);
		return data_queue.empty();
	}
};

//清单 6.4 简单的单线程队列实现
template<typename T>
class queue
{
private:
	struct node
	{
		T data;
		std::unique_ptr<node> next;
		node(T data_) :
			data(std::move(data_))
		{}
	};
	std::unique_ptr<node> head; // 1
	node* tail; // 2
public:
	queue()
	{}
	queue(const queue& other) = delete;
	queue& operator=(const queue& other) = delete;
	std::shared_ptr<T> try_pop()
	{
		if (!head)
		{
			return std::shared_ptr<T>();
		}
		std::shared_ptr<T> const res(
			std::make_shared<T>(std::move(head->data)));
		std::unique_ptr<node> const old_head = std::move(head);
		head = std::move(old_head->next); // 3
		return res;
	}
	void push(T new_value)
	{
		std::unique_ptr<node> p(new node(std::move(new_value)));
		node* const new_tail = p.get();
		if (tail)
		{
			tail->next = std::move(p); // 4
		}
		else
		{
			head = std::move(p); // 5
		}
		tail = new_tail; // 6
	}
};


//清单 6.5 带有虚拟节点的简单队列
template<typename T>
class queue
{
private:
	struct node
	{
		std::shared_ptr<T> data; // 1
		std::unique_ptr<node> next;
	};
	std::unique_ptr<node> head;
	node* tail;
public:
	queue() :
		head(new node), tail(head.get()) // 2
	{}
	queue(const queue& other) = delete;
	queue& operator=(const queue& other) = delete;
	std::shared_ptr<T> try_pop()
	{
		if (head.get() == tail) // 3
		{
			return std::shared_ptr<T>();
		}
		std::shared_ptr<T> const res(head->data); // 4
		std::unique_ptr<node> old_head = std::move(head);
		head = std::move(old_head->next); // 5
		return res; // 6
	}
	void push(T new_value)
	{
		std::shared_ptr<T> new_data(
			std::make_shared<T>(std::move(new_value))); // 7
		std::unique_ptr<node> p(new node); //8
		tail->data = new_data; // 9
		node* const new_tail = p.get();
		tail->next = std::move(p);
		tail = new_tail;
	}
};


//清单 6.6 细粒度锁的线程安全队列
template<typename T>
class threadsafe_queue
{
private:
	struct node
	{
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;
	};
	std::mutex head_mutex;
	std::unique_ptr<node> head;
	std::mutex tail_mutex;
	node* tail;
	node* get_tail()
	{
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		return tail;
	}
	std::unique_ptr<node> pop_head()
	{
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail())
		{
			return nullptr;
		}
		std::unique_ptr<node> old_head = std::move(head);
		head = std::move(old_head->next);
		return old_head;
	}
public:
	threadsafe_queue() :
		head(new node), tail(head.get())
	{}
	threadsafe_queue(const threadsafe_queue& other) = delete;
	threadsafe_queue& operator=(const threadsafe_queue& other) = delete;
	std::shared_ptr<T> try_pop()
	{
		std::unique_ptr<node> old_head = pop_head();
		return old_head ? old_head->data : std::shared_ptr<T>();
	}
	void push(T new_value)
	{
		std::shared_ptr<T> new_data(
			std::make_shared<T>(std::move(new_value)));
		std::unique_ptr<node> p(new node);
		node* const new_tail = p.get();
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		tail->data = new_data;
		tail->next = std::move(p);
		tail = new_tail;
	}
};
#endif

#if 0
//清单 6.7 使用锁和等待的线程安全队列：内部构件与接口
template<typename T>
class threadsafe_queue
{
private:
	struct node
	{
		std::shared_ptr<T> data;
		std::unique_ptr<node> next;
	};
	std::mutex head_mutex;
	std::unique_ptr<node> head;
	std::mutex tail_mutex;
	node* tail;
	std::condition_variable data_cond;
public:
	threadsafe_queue() :
		head(new node), tail(head.get())
	{}
	threadsafe_queue(const threadsafe_queue& other) = delete;
	threadsafe_queue& operator=(const threadsafe_queue& other) = delete;
	std::shared_ptr<T> try_pop();
	bool try_pop(T& value);
	std::shared_ptr<T> wait_and_pop();
	void wait_and_pop(T& value);
	void push(T new_value);
	bool empty();
};

//清单 6.8 使用锁和等待的线程安全队列：推入新节点
template<typename T>
void threadsafe_queue<T>::push(T new_value)
{
	std::shared_ptr<T> new_data(
		std::make_shared<T>(std::move(new_value)));
	std::unique_ptr<node> p(new node);
	{
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		tail->data = new_data;
		node* const new_tail = p.get();
		tail->next = std::move(p);
		tail = new_tail;
	}
	data_cond.notify_one();
}


//清单 6.9 使用锁和等待的线程安全队列：wait_and_pop()
template<typename T>
class threadsafe_queue
{
private:
	node* get_tail()
	{
		std::lock_guard<std::mutex> tail_lock(tail_mutex);
		return tail;
	}
	std::unique_ptr<node> pop_head() // 1
	{
		std::unique_ptr<node> old_head = std::move(head);
		head = std::move(old_head->next);
		return old_head;
	}
	std::unique_lock<std::mutex> wait_for_data() // 2
	{
		std::unique_lock<std::mutex> head_lock(head_mutex);
		data_cond.wait(head_lock, [&] {return head.get() != get_tail(); });
		return std::move(head_lock); // 3
	}
	std::unique_ptr<node> wait_pop_head()
	{
		std::unique_lock<std::mutex> head_lock(wait_for_data()); // 4
		return pop_head();
	}
	std::unique_ptr<node> wait_pop_head(T& value)
	{
		std::unique_lock<std::mutex> head_lock(wait_for_data()); // 5
		value = std::move(*head->data);
		return pop_head();
	}
public:
	std::shared_ptr<T> wait_and_pop()
	{
		std::unique_ptr<node> const old_head = wait_pop_head();
		return old_head->data;
	}
	void wait_and_pop(T& value)
	{
		std::unique_ptr<node> const old_head = wait_pop_head(value);
	}
};

//清单 6.10 使用锁和等待的线程安全队列：try_pop()和 empty()
template<typename T>
class threadsafe_queue
{
private:
	std::unique_ptr<node> try_pop_head()
	{
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail())
		{
			return std::unique_ptr<node>();
		}
		return pop_head();
	}
	std::unique_ptr<node> try_pop_head(T& value)
	{
		std::lock_guard<std::mutex> head_lock(head_mutex);
		if (head.get() == get_tail())
		{
			return std::unique_ptr<node>();
		}
		value = std::move(*head->data);
		return pop_head();
	}
public:
	std::shared_ptr<T> try_pop()
	{
		std::unique_ptr<node> old_head = try_pop_head();
		return old_head ? old_head->data : std::shared_ptr<T>();
	}
	bool try_pop(T& value)
	{
		std::unique_ptr<node> const old_head = try_pop_head(value);
		return old_head;
	}
	bool empty()
	{
		std::lock_guard<std::mutex> head_lock(head_mutex);
		return (head.get() == get_tail());
	}
};
#endif

int main()
{

	return 0;
}