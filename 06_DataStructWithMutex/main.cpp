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
#include "threadsafe_queue.h"
#include "threadsafe_unordermap.h"
#include "logger.h"

// extern "C" {
// #include "log.h"
// }


int main(int argc, char* argv[])
{


	//LOGI("format example: %d%c%s", 1, '2', "3");
	//LOGD("file logging\n");

	std::list<int> tmp;
	tmp.push_back(11);

	tmp.erase(tmp.begin());
	return 0;
}