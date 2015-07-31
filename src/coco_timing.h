#pragma once
#include <string>
#include <unordered_map>
#include "coco_logging.h"

#define COCO_TIMER(x) coco::Timer(x);
#define COCO_END_TIMER() ~coco::Timer();
#define COCO_TIME(x) coco::TimerManager::instance()->getTime(x);
#define COCO_MEAN_TIME(x) coco::TimerManager::instance()->getMeanTime(x);
#define COCO_TIMER_FLUSH(x) coco::TimerManager::instance()->removeTimer(x);


namespace coco
{

class Timer
{
public:	
	Timer();
	Timer(std::string name);
	~Timer();

private:
	clock_t start_time_;
	std::string name_;
};

class TimerManager
{
public:
	static TimerManager* instance();
	void pushTime(std::string name, double time);
	double getTime(std::string name) const;
	double getMeanTime(std::string name) const;
	void removeTimer(std::string name);

private:
	TimerManager()
	{ }
	std::unordered_map<std::string, std::pair<int, double>> time_list_;
};

}