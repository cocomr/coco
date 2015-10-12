#include "timing.h"
#include "logging.h"

// void TimerManager::addTimer(std::string name)
// {
//     // Using the non explicit constructor and emplace is the only way of creating an element
//     timer_list_.emplace(name, name);
// }

namespace coco
{
namespace util
{

Timer::Timer(std::string timer_name)
	: name(timer_name)
{
	start_time = std::chrono::system_clock::now();
}

void Timer::start()
{
	auto now = std::chrono::system_clock::now();
	if (iteration != 0)
	{
		auto time = std::chrono::duration_cast<std::chrono::microseconds>(
				    now - start_time).count() / 1000000.0;
		service_time += time;
		service_time_square += time * time;
	}
	start_time = now;
	++iteration;
}

void Timer::stop()
{
	double time = std::chrono::duration_cast<std::chrono::microseconds>(
				  std::chrono::system_clock::now() - start_time).count() / 1000000.0;
	elapsed_time += time;
	elapsed_time_square += time * time;
}

TimerManager* TimerManager::instance()
{
	static TimerManager timer_manager;
	return &timer_manager;
}

void TimerManager::startTimer(std::string name)
{
    std::unique_lock<std::mutex> mlock(timer_mutex_);
	if (timer_list_.find(name) == timer_list_.end())
		timer_list_[name] = Timer(name);
    timer_list_[name].start();
}

void TimerManager::stopTimer(std::string name)
{
    std::unique_lock<std::mutex> mlock(timer_mutex_);
    
    auto t = timer_list_.find(name);
    if (t != timer_list_.end())
    	t->second.stop();
}

int TimerManager::getTimeCount(std::string name)
{
	std::unique_lock<std::mutex> mlock(timer_mutex_);
	auto t = timer_list_.find(name);
	if (t == timer_list_.end())
		return -1;

	return t->second.iteration;
}

double TimerManager::getTime(std::string name)
{
	std::unique_lock<std::mutex> mlock(timer_mutex_);
	auto t = timer_list_.find(name);
	if (t == timer_list_.end())
		return -1;

	return t->second.elapsed_time;
}

double TimerManager::getTimeMean(std::string name)
{
	std::unique_lock<std::mutex> mlock(timer_mutex_);
	auto t = timer_list_.find(name);
	if (t == timer_list_.end())
		return -1;

	return t->second.elapsed_time / t->second.iteration;
}

double TimerManager::getTimeVariance(std::string name)
{
	std::unique_lock<std::mutex> mlock(timer_mutex_);
	auto t = timer_list_.find(name);
	if (t == timer_list_.end())
		return -1;

	return (t->second.elapsed_time_square / t->second.iteration) -
			std::pow(t->second.elapsed_time / t->second.iteration, 2);
}

double TimerManager::getServiceTime(std::string name)
{
	std::unique_lock<std::mutex> mlock(timer_mutex_);
	auto t = timer_list_.find(name);
	if (t == timer_list_.end())
		return -1;

	return t->second.service_time / (t->second.iteration - 1);
}

double TimerManager::getServiceTimeVariance(std::string name)
{
	std::unique_lock<std::mutex> mlock(timer_mutex_);
	auto t = timer_list_.find(name);
	if (t == timer_list_.end())
		return -1;
	return (t->second.service_time_square / (t->second.iteration - 1)) -
			std::pow(t->second.service_time / (t->second.iteration - 1), 2);
}	

void TimerManager::removeTimer(std::string name)
{
	std::unique_lock<std::mutex> mlock(timer_mutex_);
	auto t = timer_list_.find(name);
	if (t != timer_list_.end())
		timer_list_.erase(t);
}

void TimerManager::printAllTime()
{
	COCO_LOG(1) << "Printing time information for " << timer_list_.size() << " tasks";
	for (auto &t : timer_list_)
	{
		auto &name = t.first;
		COCO_LOG(1) << "Task: " << name;
		COCO_LOG(1) << "Number of iterations: " << getTimeCount(name);
		COCO_LOG(1) << "\tTotal    : " << getTime(name);
		COCO_LOG(1) << "\tMean     : " << getTimeMean(name);
		COCO_LOG(1) << "\tVariance : " << getTimeVariance(name);
		COCO_LOG(1) << "\tService time mean    : " << getServiceTime(name);
		COCO_LOG(1) << "\tService time variance: " << getServiceTimeVariance(name);
	}
}

} // End of namespace util
} // End of namespace coco

