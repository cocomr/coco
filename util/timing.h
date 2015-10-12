#pragma once
#include <string>
#include <unordered_map>
#include <ctime>
#include <chrono>
#include <vector>
#include <mutex>

#define COCO_START_TIMER(x) coco::util::TimerManager::instance()->startTimer(x);
#define COCO_STOP_TIMER(x) coco::util::TimerManager::instance()->stopTimer(x);
#define COCO_CLEAR_TIMER(x) coco::util::TimerManager::instance()->removeTimer(x);
#define COCO_TIME_COUNT(x) coco::util::TimerManager::instance()->getTimeCount(x);
#define COCO_TIME(x) coco::util::TimerManager::instance()->getTime(x);
#define COCO_TIME_MEAN(x) coco::util::TimerManager::instance()->getTimeMean(x);
#define COCO_TIME_VARIANCE(x) coco::util::TimerManager::instance()->getTimeVariance(x);
#define COCO_SERVICE_TIME(x) coco::util::TimerManager::instance()->getServiceTime(x);
#define COCO_SERVICE_TIME_VARIANCE(x) coco::util::TimerManager::instance()->getServiceTimeVariance(x);
#define COCO_PRINT_ALL_TIME coco::util::TimerManager::instance()->printAllTime();

namespace coco
{
namespace util
{

struct Timer
{
public:	
	using time_t = std::chrono::system_clock::time_point;

	Timer(std::string timer_name = "");
	
	void start();
	void stop();
    
	std::string name;
	time_t start_time;
	int iteration = 0;
	double elapsed_time = 0;
	double elapsed_time_square = 0;
	//time_t start_time;
	double service_time = 0;
	double service_time_square = 0;
};

class TimerManager
{
public:
	using time_t = std::chrono::system_clock::time_point;

	static TimerManager* instance();
    void startTimer(std::string name);
    void stopTimer(std::string name);
    void removeTimer(std::string name);
    //void addElapsedTime(std::string name, double time);
    //void addTime(std::string name, time_t time);
	double getTime(std::string name);
	int getTimeCount(std::string name);
	double getTimeMean(std::string name);
	double getTimeVariance(std::string name);
	double getServiceTime(std::string name);
    double getServiceTimeVariance(std::string name);
    void printAllTime();

private:
	TimerManager()
	{ }

    std::unordered_map<std::string, Timer> timer_list_;
    std::mutex timer_mutex_;
};

}
}
