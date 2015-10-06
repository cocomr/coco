#include "timing.h"
#include "logging.h"

namespace coco
{
namespace util
{
Timer::Timer()
{
	start_time_ = clock();
}

Timer::Timer(std::string name)
	: name_(name)
{
	start_time_ = clock();
}

Timer::~Timer()
{
	double elapsed_time = ((double)(clock() - start_time_)) / CLOCKS_PER_SEC;
	if (name_.empty())
	{
		COCO_LOG(0) << "Elapsed time: " << elapsed_time;
	}
	else
	{
        TimerManager::instance()->addTime(name_, elapsed_time);
	}
}

TimerManager* TimerManager::instance()
{
	static TimerManager timer_manager;
	return &timer_manager;
}

void TimerManager::addTimer(Timer t)
{
    timer_list_[t.name()] = t;
}

void TimerManager::stopTimer(std::string name)
{
    timer_list_.erase(name);
}

void TimerManager::addTime(std::string name, double time)
{
	if (time_list_.find(name) == time_list_.end())
		time_list_[name] = std::pair<int, double>(0, 0);

	++time_list_[name].first;
	time_list_[name].second += time;
}

double TimerManager::getTime(std::string name) const
{
	auto t = time_list_.find(name);
	if (t != time_list_.end())
		return t->second.second;

	return -1;
}

double TimerManager::getMeanTime(std::string name) const
{
	auto t = time_list_.find(name);
	if (t != time_list_.end())
		return t->second.second / t->second.first;

	return -1;
}

void TimerManager::removeTimer(std::string name)
{
	auto t = time_list_.find(name);
	if (t != time_list_.end())
		time_list_.erase(t);
}

} // End of namespace util
} // End of namespace coco
