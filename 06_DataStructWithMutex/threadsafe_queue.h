#include <mutex>
#include <memory>

//清单 6.7 使用锁和等待的线程安全队列：内部构件与接口
template<typename T>
class threadsafe_queue {
private:
	struct node {
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
		head(new node), tail(head.get()) {}
	threadsafe_queue(const threadsafe_queue& other) = delete;
	threadsafe_queue& operator=(const threadsafe_queue& other) = delete;
	std::shared_ptr<T> try_pop();
	bool try_pop(T& value);
	std::shared_ptr<T> wait_and_pop();
	void wait_and_pop(T& value);
	void push(T new_value);
	bool empty();

private:
	node* get_tail() {
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

};

//清单 6.8 使用锁和等待的线程安全队列：推入新节点
template<typename T>
void threadsafe_queue<T>::push(T new_value) {
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

template<typename T>
std::shared_ptr<T> threadsafe_queue<T>::wait_and_pop()
{
	std::unique_ptr<node> const old_head = wait_pop_head();
	return old_head->data;
}

template<typename T>
void threadsafe_queue<T>::wait_and_pop(T& value)
{
	std::unique_ptr<node> const old_head = wait_pop_head(value);
}

template<typename T>
std::shared_ptr<T> threadsafe_queue<T>::try_pop()
{
	std::unique_ptr<node> old_head = try_pop_head();
	return old_head ? old_head->data : std::shared_ptr<T>();
}

template<typename T>
bool threadsafe_queue<T>::try_pop(T& value)
{
	std::unique_ptr<node> const old_head = try_pop_head(value);
	return old_head;
}

template<typename T>
bool threadsafe_queue<T>::empty()
{
	std::lock_guard<std::mutex> head_lock(head_mutex);
	return (head.get() == get_tail());
}