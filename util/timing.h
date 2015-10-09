#pragma once
#include <string>
#include <unordered_map>
#include <ctime>
#include <chrono>
#include <vector>
#include <mutex>

#define COCO_START_TIMER(x) coco::util::TimerManager::instance()->addTimer(x);
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

class Timer
{
public:	
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
    void addTimer(std::string name);
    void stopTimer(std::string name);
    void removeTimer(std::string name);
    void addElapsedTime(std::string name, double time);
    void addTime(std::string name, time_t time);
	double getTime(std::string name);
	int getTimeCount(std::string name);
	double getTimeMean(std::string name);
	double getTimeVariance(std::string name);
	double getServiceTime(std::string name);
    double getServiceTimeVariance(std::string name);
    void printAllTime();

private:
	struct ElapsedTime
	{
		int iteration = 0;
		double time = 0;
		double time_square = 0;
	};
	struct ServiceTime
	{
		ServiceTime()
			: iteration(-1), time(0), time_square(0), 
			  now(std::chrono::system_clock::now())
		{}
		int iteration;
		time_t now;
		double time;
		double time_square;
	};
	TimerManager()
	{ }
	//std::unordered_map<std::string, std::tuple<int, double, double> > elapsed_time_;
    //std::unordered_map<std::string, std::vector<time_t> > time_list_;
    //std::unordered_map<std::string, std::tuple<int, time_t, double, double> >
	std::unordered_map<std::string, ElapsedTime> elapsed_time_;
	std::unordered_map<std::string, ServiceTime> service_time_;

    std::unordered_map<std::string, Timer> timer_list_;

    std::mutex timer_mutex_;
    std::mutex time_mutex_;
};

}
}
