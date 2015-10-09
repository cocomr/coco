#include "timing.h"
#include "logging.h"

namespace coco
{
namespace util
{

Timer::Timer(std::string name)
	: name_(name)
{
	start_time_ = std::chrono::system_clock::now();
}

Timer::~Timer()
{
	auto now = std::chrono::system_clock::now();
	double elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(
			now - start_time_).count() / 1000000.0;
	
	if (name_.empty())
	{
		COCO_LOG(0) << "Elapsed time: " << elapsed_time;
	}
	else
	{
        TimerManager::instance()->addElapsedTime(name_, elapsed_time);
        TimerManager::instance()->addTime(name_, now);
	}
}

TimerManager* TimerManager::instance()
{
	static TimerManager timer_manager;
	return &timer_manager;
}

void TimerManager::addTimer(std::string name)
{
    std::unique_lock<std::mutex> mlock(timer_mutex_);
    // Using the non explicit constructor and emplace is the only way of creating an element
    // directly in the container
    timer_list_.emplace(name, name);
}

void TimerManager::stopTimer(std::string name)
{
    std::unique_lock<std::mutex> mlock(timer_mutex_);
    timer_list_.erase(name);
}

void TimerManager::addElapsedTime(std::string name, double elapsed_time)
{
	std::unique_lock<std::mutex> mlock(time_mutex_);

	auto &t = elapsed_time_[name];
	t.iteration += 1;
	t.time += elapsed_time;
	t.time_square += elapsed_time * elapsed_time;
}

void TimerManager::addTime(std::string name, time_t time_now)
{
	std::unique_lock<std::mutex> mlock(time_mutex_);

	auto now = std::chrono::system_clock::now();
	auto &t = service_time_[name];
	double time = std::chrono::duration_cast<std::chrono::microseconds>(
					now - t.now).count() / 1000000.0;
	t.iteration += 1;
	t.time += time;
	t.time_square += time * time;
	t.now = now;
}

int TimerManager::getTimeCount(std::string name)
{
	std::unique_lock<std::mutex> mlock(time_mutex_);
	auto t = elapsed_time_.find(name);
	if (t == elapsed_time_.end())
		return -1;

	return t->second.iteration;
}

double TimerManager::getTime(std::string name)
{
	std::unique_lock<std::mutex> mlock(time_mutex_);
	auto t = elapsed_time_.find(name);
	if (t == elapsed_time_.end())
		return -1;

	return t->second.time;
}

double TimerManager::getTimeMean(std::string name)
{
	std::unique_lock<std::mutex> mlock(time_mutex_);
	auto t = elapsed_time_.find(name);
	if (t == elapsed_time_.end())
		return -1;

	return t->second.time / t->second.iteration;
}

double TimerManager::getTimeVariance(std::string name)
{
	std::unique_lock<std::mutex> mlock(time_mutex_);
	auto t = elapsed_time_.find(name);
	if (t == elapsed_time_.end())
		return -1;

	return (t->second.time_square / t->second.iteration) -
			std::pow(t->second.time / t->second.iteration, 2);
}

double TimerManager::getServiceTime(std::string name)
{
	std::unique_lock<std::mutex> mlock(time_mutex_);
	auto t = service_time_.find(name);
	if (t == service_time_.end())
		return -1;

	return t->second.time / t->second.iteration;
}

double TimerManager::getServiceTimeVariance(std::string name)
{
	std::unique_lock<std::mutex> mlock(time_mutex_);
	auto t = service_time_.find(name);
	if (t == service_time_.end())
		return -1;
	return (t->second.time_square / t->second.iteration) -
			std::pow(t->second.time / t->second.iteration, 2);
}	

void TimerManager::removeTimer(std::string name)
{
	std::unique_lock<std::mutex> mlock(time_mutex_);
	auto t = elapsed_time_.find(name);
	if (t != elapsed_time_.end())
		elapsed_time_.erase(t);
}

void TimerManager::printAllTime()
{
	COCO_LOG(1) << "Printing time information for " << elapsed_time_.size() << " tasks";
	for (auto &t : elapsed_time_)
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
