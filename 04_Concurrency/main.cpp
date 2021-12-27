#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <queue>
#include <condition_variable>
#include <future>
#include "windows.h"
#include <experimental/>


#if 0
//清单 4.1 使用 std::condition_variable 等待需要处理的数据
class data_chunk;
std::mutex mut;
std::queue<data_chunk> data_queue; // 1
std::condition_variable data_cond;
void data_preparation_thread()
{
	while (more_data_to_prepare())
	{
		data_chunk const data = prepare_data();
		{
			std::lock_guard<std::mutex> lk(mut);
			data_queue.push(data); // 2
		}
		data_cond.notify_one(); // 3
	}
}
void data_processing_thread()
{
	while (true)
	{
		std::unique_lock<std::mutex> lk(mut); // 4
		data_cond.wait(
			lk, [] {return !data_queue.empty(); }); // 5
		data_chunk data = data_queue.front();
		data_queue.pop();
		lk.unlock(); // 6
		process(data);
		if (is_last_chunk(data))
			break;
	}
}
#endif

#if 0
//清单 4.2 std::queue 接口
template <class T, class Container = std::deque<T> >
class queue {
public:
	explicit queue(const Container&);
	explicit queue(Container && = Container());
	template <class Alloc> explicit queue(const Alloc&);
	template <class Alloc> queue(const Container&, const Alloc&);
	template <class Alloc> queue(Container&&, const Alloc&);
	template <class Alloc> queue(queue&&, const Alloc&);
	void swap(queue& q);
	bool empty() const;
	size_type size() const;
	T& front();
	const T& front() const;
	T& back();
	const T& back() const;
	void push(const T& x);
	void push(T&& x);
	void pop();
	template <class... Args> void emplace(Args&&... args);
};

//清单 4.3 threadsafe_queue 的接口
#include <memory> // 为了使用 std::shared_ptr
template<typename T>
class threadsafe_queue
{
public:
	threadsafe_queue();
	threadsafe_queue(const threadsafe_queue&);
	threadsafe_queue& operator=(
		const threadsafe_queue&) = delete; // 不允许简单的赋值
	void push(T new_value);
	bool try_pop(T& value); // 1
	std::shared_ptr<T> try_pop(); // 2
	void wait_and_pop(T& value);
	std::shared_ptr<T> wait_and_pop();
	bool empty() const;
};
#endif

#if 0
//清单 4.5 使用条件变量的线程安全队列的完整类定义
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
template<typename T>
class threadsafe_queue
{
private:
	mutable std::mutex mut; // 1 互斥锁必须是可变的
	std::queue<T> data_queue;
	std::condition_variable data_cond;
public:
	threadsafe_queue()
	{}
	threadsafe_queue(threadsafe_queue const& other)
	{
		std::lock_guard<std::mutex> lk(other.mut);
		data_queue = other.data_queue;
	}
	void push(T new_value)
	{
		std::lock_guard<std::mutex> lk(mut);
		data_queue.push(new_value);
		data_cond.notify_one();
	}
	void wait_and_pop(T& value)
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		value = data_queue.front();
		data_queue.pop();
	}
	std::shared_ptr<T> wait_and_pop()
	{
		std::unique_lock<std::mutex> lk(mut);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
		data_queue.pop();
		return res;
	}
	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return false;
		value = data_queue.front();
		data_queue.pop();
		return true;
	}
	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(mut);
		if (data_queue.empty())
			return std::shared_ptr<T>();
		std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
		data_queue.pop();
		return res;
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lk(mut);
		return data_queue.empty();
	}
};
#endif

#if 0
//清单 4.6 使用 std::future 获取异步任务的返回值
#include <future>
#include <iostream>
int find_the_answer_to_ltuae();
void do_other_stuff();
int main()
{
	std::future<int> the_answer = std::async(find_the_answer_to_ltuae);
	do_other_stuff();
	std::cout << "The answer is " << the_answer.get() << std::endl;
}

//清单 4.7 使用 std::async 向函数传递参数
#include <string>
#include <future>
struct X
{
	void foo(int, std::string const&);
	std::string bar(std::string const&);
};
X x;
auto f1 = std::async(&X::foo, &x, 42, "hello"); // 调用 p->foo(42, "hello")，p 是指向 x 的指针
auto f2 = std::async(&X::bar, x, "goodbye"); // 调用 tmpx.bar("goodbye")， tmpx 是 x 的拷贝副本
struct Y
{
	double operator()(double);
};
Y y;
auto f3 = std::async(Y(), 3.141); // 调用 tmpy(3.141)，tmpy 从 Y 通过移动构造得到
auto f4 = std::async(std::ref(y), 2.718); // 调用 y(2.718)
X baz(X&);
std::async(baz, std::ref(x)); // 调用 baz(x)
class move_only
{
public:
	move_only();
	move_only(move_only&&)
		move_only(move_only const&) = delete;
	move_only& operator=(move_only&&);
	move_only& operator=(move_only const&) = delete;
	void operator()();
};
auto f5 = std::async(move_only()); // 调用 tmp()，tmp 是通过 std::move(move_only())构造得到

//清单 4.8 特化版 std::packaged_task<>的部分类定义
template<>
class packaged_task<std::string(std::vector<char>*, int)>
{
public:
	template<typename Callable>
	explicit packaged_task(Callable&& f);
	std::future<std::string> get_future();
	void operator()(std::vector<char>*, int);
};
#endif

#if 0
//清单 4.9 使用 std::packaged_task 在 GUI 线程运行代码
#include <deque>
#include <mutex>
#include <future>
#include <thread>
#include <utility>
std::mutex m;
std::deque<std::packaged_task<void()> > tasks;
bool gui_shutdown_message_received();
void get_and_process_gui_message();
void gui_thread() // 1
{
	while (!gui_shutdown_message_received()) // 2
	{
		get_and_process_gui_message(); // 3
		std::packaged_task<void()> task;
		{
			std::lock_guard<std::mutex> lk(m);
			if (tasks.empty()) // 4
				continue;
			task = std::move(tasks.front()); // 5
			tasks.pop_front();
		}
		task(); // 6
	}
}
std::thread gui_bg_thread(gui_thread);
template<typename Func>
std::future<void> post_task_for_gui_thread(Func f)
{
	std::packaged_task<void()> task(f); // 7
	std::future<void> res = task.get_future(); // 8
	std::lock_guard<std::mutex> lk(m);
	tasks.push_back(std::move(task)); // 9
	return res; // 10
}
#endif

#if 0
//清单 4.10 使用承诺处理单线程上的多个连接
void process_connections(connection_set & connections)
{
	while (!done(connections)) // 1
	{
		for (connection_iterator // 2
			connection = connections.begin(), end = connections.end();
			connection != end;
			++connection)
		{
			if (connection->has_incoming_data()) // 3
			{
				data_packet data = connection->incoming();
				std::promise<payload_type>& p =
					connection->get_promise(data.id); // 4
				p.set_value(data.payload);
			}
			if (connection->has_outgoing_data()) // 5
			{
				outgoing_packet data =
					connection->top_of_outgoing_queue();
				connection->send(data.payload);
				data.promise.set_value(true); // 6
			}
		}
	}
}


double square_root(double x)
{
	if (x < 0)
	{
		throw std::out_of_range(“x < 0”);
	}
	return sqrt(x);
}
#endif


#if 0
using namespace std;
using namespace std::chrono;

void get_now_time()
{

	auto start = std::chrono::high_resolution_clock::now();
	auto stop = std::chrono::high_resolution_clock::now();
	std::cout <<" 1"
		<< std::chrono::duration<double, std::chrono::seconds>(stop - start).count()
		<<"sec" << std::endl;

	auto start = std::chrono::system_clock::now();
	auto tm_t = std::chrono::system_clock::to_time_t(start);
	struct tm* ptm = localtime(&tm_t);

	char date[60] = { 0 };
	sprintf(date, "%d-%02d-%02d-%02d:%02d:%02d",
		(int)ptm->tm_year + 1900, (int)ptm->tm_mon + 1, (int)ptm->tm_mday,
		(int)ptm->tm_hour, (int)ptm->tm_min, (int)ptm->tm_sec);
	
	std::cout << date << std::endl;
	return;
}

#endif

#if 0
//清单 4.12 快速排序的顺序实现
template<typename T>
std::list<T> sequential_quick_sort(std::list<T> input)
{
	if (input.empty())
	{
		return input;
	}
	std::list<T> result;
	result.splice(result.begin(), input, input.begin()); // 1
	T const& pivot = *result.begin(); // 2
	auto divide_point = std::partition(input.begin(), input.end(),
		[&](T const& t) {return t < pivot; }); // 3
	std::list<T> lower_part;
	lower_part.splice(lower_part.end(), input, input.begin(),
		divide_point); // 4
	auto new_lower(
		sequential_quick_sort(std::move(lower_part))); // 5
	auto new_higher(
		sequential_quick_sort(std::move(input))); // 6
	result.splice(result.end(), new_higher); // 7
	result.splice(result.begin(), new_lower); // 8
	return result;
}
#endif

#if 0

//清单 4.13 使用期望的并行快速排序
template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input)
{
	if (input.empty())
	{
		return input;
	}
	std::list<T> result;
	result.splice(result.begin(), input, input.begin());
	T const& pivot = *result.begin();
	auto divide_point = std::partition(input.begin(), input.end(),
		[&](T const& t) {return t < pivot; });
	std::list<T> lower_part;
	lower_part.splice(lower_part.end(), input, input.begin(),
		divide_point);
	std::future<std::list<T> > new_lower( // 1
		std::async(&parallel_quick_sort<T>, std::move(lower_part
		)));
	auto new_higher(
		parallel_quick_sort(std::move(input))); // 2
	result.splice(result.end(), new_higher); // 3
	result.splice(result.begin(), new_lower.get()); // 4
	return result;
}


//清单 4.14 spawn_task 的示例实现
template<typename F, typename A>
std::future<std::result_of<F(A&&)>::type>
spawn_task(F && f, A && a)
{
	typedef std::result_of<F(A&&)>::type result_type;
	std::packaged_task<result_type(A&&)>
		task(std::move(f)));
		std::future<result_type> res(task.get_future());
		std::thread t(std::move(task), std::move(a));
		t.detach();
		return res;
}
#endif

#if 0
//清单 4.15 ATM 逻辑类的简单实现
struct card_inserted
{
	std::string account;
};
class atm
{
	messaging::receiver incoming;
	messaging::sender bank;
	messaging::sender interface_hardware;
	void (atm::* state)();
	std::string account;
	std::string pin;
	void waiting_for_card() // 1
	{
		interface_hardware.send(display_enter_card()); // 2
		incoming.wait(). // 3
			handle<card_inserted>(
				[&](card_inserted const& msg) // 4
				{
					account = msg.account;
					pin = "";
					interface_hardware.send(display_enter_pin());
					state = &atm::getting_pin;
				}
		);
	}
	void getting_pin();
public:
	void run() // 5
	{
		state = &atm::waiting_for_card; // 6
		try
		{
			for (;;)
			{
				(this->*state)(); // 7
			}
		}
		catch (messaging::close_queue const&)
		{
		}
	}
};


//清单 4.17 为并发技术规范中的期望而实现的一个简单的 std::async 等价物
template<typename Func>
std::experimental::future<decltype(std::declval<Func>()())>
spawn_async(Func && func) {
	std::experimental::promise<
		decltype(std::declval<Func>()())> p;
	auto res = p.get_future();
	std::thread t(
		[p = std::move(p), f = std::decay_t<Func>(func)]()
		mutable{
		try {
			p.set_value_at_thread_exit(f());
		}
		catch (...) {
			p.set_exception_at_thread_exit(std::current_exception());
		}
	});
	t.detach();
	return res;
}
#endif

using namespace std;
int THreadRun(std::promise<int>&flag)
{
	cout << "thread id = " << std::this_thread::get_id() << endl;
	flag.set_value(222);
	return 0;
}


void ThreadTest()
{
	cout << "main id = " << std::this_thread::get_id() << endl;

	std::experimental::future<>

	std::promise<int>flag;
	std::thread t1(THreadRun, std::ref(flag));

	auto fu = flag.get_future();

	cout << fu.get() << endl;

	t1.join();
	return;

}



int main()
{
	ThreadTest();
    return 0;
}