#pragma once
#include <string>
#include <unordered_map>
#include <ctime>
#include <chrono>
#include <vector>

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
//	Timer();
	Timer(std::string name = "");
	~Timer();

    std::string name() const { return name_; }
private:
	std::chrono::system_clock::time_point start_time_;
	std::string name_;
};

class TimerManager
{
public:
	using time_t = std::chrono::system_clock::time_point;

	static TimerManager* instance();
    void addTimer(Timer t);
    void stopTimer(std::string name);

    void addElapsedTime(std::string name, double time);
    void addTime(std::string name, time_t time);
	double getTime(std::string name) const;
	double getMeanTime(std::string name) const;
    void removeTimer(std::string name);

private:
	TimerManager()
	{ }
	std::unordered_map<std::string, std::pair<int, double> > elapsed_time_;
	std::unordered_map<std::string, std::vector<double> > elapsed_time_list_;
    std::unordered_map<std::string, std::vector<time_t> > time_list_;;

    std::unordered_map<std::string, Timer> timer_list_;
};

}
}
