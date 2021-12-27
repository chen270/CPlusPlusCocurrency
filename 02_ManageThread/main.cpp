#include <thread>
#include <vector>
#include <iostream>
#include <stdexcept>
#include <numeric>      // std::accumulate
/************************************************************************/
/* 2.1 - 2.2 基础管理与传参                                             */
/************************************************************************/

#if 0
void do_something(int)
{
}

//清单 2.1 函数已经返回，线程仍然访问局部变量
struct func
{
    int &i;
    func(int &i_) : i(i_) {}
    void operator()()
    {
        for (unsigned j = 0; j < 1000000; ++j)
        {
            do_something(i); // 潜在访问悬垂引用
        }
    }
};
void oops()
{
    int some_local_state = 0;
    func my_func(some_local_state);
    std::thread my_thread(my_func);
    my_thread.detach(); // 不等待线程结束
} // 新线程可能还在运行

class widget_data
{
public:
    widget_data() = default;
    widget_data(widget_data&) = delete;
    widget_data(widget_data&&) noexcept {
    }
};

void update_data_for_widget(int w, widget_data& data)
{

}

void oops_again(int w)
{
	widget_data data;
	std::thread t(update_data_for_widget, w, std::ref(data));
	t.join();
	//process_widget_data(data);
}
#endif


/************************************************************************/
/*  2.3 转移线程所有权                                                  */
/************************************************************************/

//void some_function() {}
//void some_other_function() {}
//void some_other_function2(int) {}
//
//void swapThreadOwnership()
//{
//	std::thread t1(some_function);
//	std::thread t2 = std::move(t1);
//	t1 = std::thread(some_other_function);
//	std::thread t3;
//	t3 = std::move(t2);
//    
//    //t1.swap(t3);
//	t1 = std::move(t3);// 赋值操作将使程序崩溃
//    //call std::terminate()
//}

//清单 2.5 函数返回 std::thread 对象
//std::thread f()
//{
//	void some_function();
//	return std::thread(some_function);
//}
//std::thread g()
//{
//	void some_other_function2(int);
//	std::thread t(some_other_function2, 42);
//	return t;
//}

//void some_function() {
//};
//
//void f(std::thread t) {
//    t.join();
//};
//void g()
//{
//	f(std::thread(some_function));
//	std::thread t(some_function);
//	f(std::move(t));
//}


//清单 2.6 scoped_thread 的用法
//class scoped_thread
//{
//	std::thread t;
//public:
//	explicit scoped_thread(std::thread t_) :
//		t(std::move(t_))
//	{
//		if (!t.joinable())
//			throw std::logic_error("No thread");
//	}
//	~scoped_thread()
//	{
//		t.join();
//	}
//	scoped_thread(scoped_thread const&) = delete;
//	scoped_thread& operator=(scoped_thread const&) = delete;
//};
//struct func; // 定义在清单 2.1 中
//void f()
//{
//	int some_local_state;
//	scoped_thread t(std::thread(func(some_local_state)));
//	//do_something_in_current_thread();
//}


//清单 2.8 创建一些线程并等待它们结束
//void do_work(unsigned id) {};
//void f()
//{
//	std::vector<std::thread> threads;
//	for (unsigned i = 0; i < 20; ++i)
//	{
//		threads.emplace_back(do_work, i); // 生成线程
//	}
//	for (auto& entry : threads) // 对每个线程调用 join()
//		entry.join();
//}

//清单 2.9 初级并行版的 std::accumulate
template<typename Iterator, typename T>
struct accumulate_block
{
	void operator()(Iterator first, Iterator last, T& result)
	{
		result = std::accumulate(first, last, result);
	}
};
template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
	unsigned long const length = std::distance(first, last);
	if (!length)
		return init;
	unsigned long const min_per_thread = 25;
	unsigned long const max_threads =
		(length + min_per_thread - 1) / min_per_thread;
	unsigned long const hardware_threads =
		std::thread::hardware_concurrency();
	unsigned long const num_threads =
		std::min(hardware_threads != 0 ? hardware_threads : 2, max_thread
			s);
	unsigned long const block_size = length / num_threads;
	std::vector<T> results(num_threads);
	std::vector<std::thread> threads(num_threads - 1);
	Iterator block_start = first;
	for (unsigned long i = 0; i < (num_threads - 1); ++i)
	{
		Iterator block_end = block_start;
		std::advance(block_end, block_size);
		threads[i] = std::thread(
			accumulate_block<Iterator, T>(),
			block_start, block_end, std::ref(results[i]));
		block_start = block_end;
	}
	accumulate_block<Iterator, T>()(
		block_start, last, results[num_threads - 1]);
	for (auto& entry : threads)
		entry.join();
	return std::accumulate(results.begin(), results.end(), init);
}

//std::thread::id master_thread;
//void some_core_part_of_algorithm()
//{
//	if (std::this_thread::get_id() == master_thread)
//	{
//		do_master_thread_work();
//	}
//	do_common_work();
//}

int main()
{
	std::cout << std::this_thread::get_id();
    return 0;
}