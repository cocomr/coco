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

void TimerManager::addTimer(Timer t)
{
    timer_list_[t.name()] = t;
}

void TimerManager::stopTimer(std::string name)
{
    timer_list_.erase(name);
}

void TimerManager::addTime

void TimerManager::addElapsedTime(std::string name, double time)
{
	auto timer = elapsed_time_.find(name);
	if (timer == elapsed_time_.end())
		timer = std::pair<int, double>(0, 0);

	++timer.first;
	timer.second += time;

	elapsed_time_list_[name].push_back(time);
}

void TimerManager::addTime(std::string name, time_t time)
{
	auto timer = time_list_.find(name);

	time_list_[name].push_back(time);
}

double TimerManager::getTime(std::string name) const
{
	auto t = elapsed_time_.find(name);
	if (t != elapsed_time_.end())
		return t->second.second;

	return -1;
}

double TimerManager::getMeanTime(std::string name) const
{
	auto t = elapsed_time_.find(name);
	if (t != elapsed_time_.end())
		return t->second.second / t->second.first;

	return -1;
}

void TimerManager::removeTimer(std::string name)
{
	auto t = elapsed_time_.find(name);
	if (t != elapsed_time_.end())
		elapsed_time_.erase(t);
}

} // End of namespace util
} // End of namespace coco
