#include <iostream>
#include <thread> //1

void hello() //2
{
	std::cout << "Hello Thread" << std::endl;
}

int main()
{
	std::thread t(hello); //3
	t.join(); //4
	return 0;
}