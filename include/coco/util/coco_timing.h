#pragma once
#include <string>
#include <unordered_map>
#include <ctime>

#define COCO_START_TIMER(x) coco::util::TimerManager::instance()->addTimer(coco::util::Timer(x));
#define COCO_STOP_TIMER(x) coco::util::TimerManager::instance()->stopTimer(x);
#define COCO_FLUSH_TIMER(x) coco::util::TimerManager::instance()->removeTimer(x);
#define COCO_TIME(x) coco::util::TimerManager::instance()->getTime(x)
#define COCO_MEAN_TIME(x) coco::util::TimerManager::instance()->getMeanTime(x)

namespace coco
{
namespace util
{
class Timer
{
public:	
	Timer();
	Timer(std::string name);
	~Timer();

    std::string name() const { return name_; }
private:
	clock_t start_time_;
	std::string name_;
};

class TimerManager
{
public:
	static TimerManager* instance();
    void addTimer(Timer t);
    void stopTimer(std::string name);

    void addTime(std::string name, double time);
	double getTime(std::string name) const;
	double getMeanTime(std::string name) const;
    void removeTimer(std::string name);

private:
	TimerManager()
	{ }
	std::unordered_map<std::string, std::pair<int, double>> time_list_;
    std::unordered_map<std::string, Timer> timer_list_;
};

}
}
