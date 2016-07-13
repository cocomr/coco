/**
Copyright 2015, Filippo Brizzi"
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@Author 
Filippo Brizzi, PhD Student
fi.brizzi@sssup.it
Emanuele Ruffaldi, PhD
e.ruffaldi@sssup.it

PERCRO, (Laboratory of Perceptual Robotics)
Scuola Superiore Sant'Anna
via Luigi Alamanni 13D, San Giuliano Terme 56010 (PI), Italy
*/

#pragma once
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <ctime>
#include <chrono>
#include <vector>
#include <mutex>
#include <cmath>
#include <limits>
#include <algorithm>
#include <atomic>

#include "coco/util/logging.h"

#define COCO_START_TIMER(x) coco::util::TimerManager::instance()->startTimer(x);
#define COCO_STOP_TIMER(x) coco::util::TimerManager::instance()->stopTimer(x);
#define COCO_CLEAR_TIMER(x) coco::util::TimerManager::instance()->removeTimer(x);
#define COCO_TIME(x) coco::util::TimerManager::instance()->time(x)
#define COCO_TIME_MEAN(x) coco::util::TimerManager::instance()->meanTime(x)
#define COCO_TIME_STATISTICS(x) coco::util::TimerManager::instance()->timeStatistics(x)
#define COCO_PRINT_ALL_TIME coco::util::TimerManager::instance()->printAllTime();
#define COCO_RESET_TIMERS coco::util::TimerManager::instance()->resetTimers();

namespace coco
{
namespace util
{

struct TimeStatistics
{
    unsigned long iterations;
    double last;
    double elapsed;
    double mean;
    double variance;
    double service_mean;
    double service_variance;
    double min;
    double max;

    std::string toString() const
    {
        std::stringstream ss;
        ss << "\tNumber of iterations: " << iterations << std::endl;
        ss << "\tTotal    : " << elapsed << std::endl;
        ss << "\tMean     : " << mean << std::endl;
        ss << "\tVariance : " << variance << std::endl;
        ss << "\tService time mean    : " << service_mean << std::endl;
        ss << "\tService time variance: " << service_variance << std::endl;
        ss << "\tMin: " << min << std::endl; 
        ss << "\tMax: " << max << std::endl;
        return ss.str();
    }
};

class Timer
{
public:
    using time_point = std::chrono::system_clock::time_point;

    explicit Timer(std::string timer_name = "") noexcept
        : name_(timer_name)
    {}

    Timer(const Timer &other)
        : name_(other.name_)
    {}

    void start()
    {
        while (lock_.exchange(true));

        auto now = std::chrono::system_clock::now();
        if (iterations_ != 0)
        {
            auto time = std::chrono::duration_cast<std::chrono::microseconds>(
                        now - start_time_).count() / 1000000.0;
            service_time_ += time;
            service_time_square_ += time * time;
        }

        start_time_ = now;
        ++iterations_;

        lock_ = false;
    }

    void stop()
    {
        while (lock_.exchange(true));
        

        time_ = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::system_clock::now() - start_time_).count() / 1000000.0;
        elapsed_time_ += time_;
        elapsed_time_square_ += time_ * time_;

        min_time_ = std::min(time_, min_time_);
        max_time_ = std::max(time_, max_time_);

        lock_ = false;
    }

    TimeStatistics timeStatistics()
    {
        TimeStatistics t;
        
        while (lock_.exchange(true));
        
        t.last = time_;
        t.iterations = iterations_;
        t.elapsed = elapsed_time_;
        t.mean = elapsed_time_ / iterations_;
        t.variance = (elapsed_time_square_ / iterations_) -
                        std::pow(elapsed_time_ / iterations_, 2);
        t.service_mean = service_time_ / (iterations_ - 1);
        t.service_variance = (service_time_square_ / (iterations_ - 1)) -
                                std::pow(service_time_ / (iterations_ - 1), 2);
        t.min = min_time_;
        t.max = max_time_;

        lock_ = false;

        return t;
    }

    double time()
    {
        while (lock_.exchange(true));
        auto t = time_;
        lock_ = false;
        return t;
    }

    double meanTime()
    {
        while (lock_.exchange(true));
        auto t = elapsed_time_ / iterations_;
        lock_ = false;
        return t;
    }

private:
    const std::string &name_;
    time_point start_time_;
    int iterations_ = 0;
    double time_ = 0;
    double elapsed_time_ = 0;
    double elapsed_time_square_ = 0;

    double service_time_ = 0;
    double service_time_square_ = 0;
    double max_time_ = 0;
    double min_time_ = {std::numeric_limits<double>::infinity()};

    std::atomic<bool> lock_ = {false};
};


class TimerManager
{
public:
    using time_point = std::chrono::system_clock::time_point;

    static TimerManager* instance()
    {
        static TimerManager timer_manager;
        return &timer_manager;
    }

    void startTimer(const std::string &name)
    {
        while (lock_.exchange(true));
        if (timer_list_.find(name) == timer_list_.end())
            timer_list_.insert({name, std::make_shared<Timer>(name)});
        timer_list_[name]->start();
        lock_ = false;
    }
    void stopTimer(const std::string &name)
    {
        while (lock_.exchange(true));
        auto t = timer_list_.find(name);
        if (t != timer_list_.end())
            t->second->stop();
        lock_ = false;
    }
    void removeTimer(const std::string &name)
    {
        while (lock_.exchange(true));
        auto t = timer_list_.find(name);
        if (t != timer_list_.end())
            timer_list_.erase(t);
        lock_ = false;
    }
    double time(const std::string &name)
    {
        while (lock_.exchange(true));
        auto t = timer_list_.find(name);
        if (t == timer_list_.end())
        {
            lock_ = false;
            return -1;
        }
        lock_ = false;
        return t->second->time();
    }
    double meanTime(const std::string &name)
    {
        while (lock_.exchange(true));
        auto t = timer_list_.find(name);
        if (t == timer_list_.end())
        {
            lock_ = false;
            return -1;
        }
        lock_ = false;
        return t->second->meanTime();
    }
    TimeStatistics timeStatistics(const std::string &name)
    {
        while (lock_.exchange(true));
        auto t = timer_list_.find(name);
        if (t == timer_list_.end())
        {
            lock_ = false;
            return TimeStatistics();
        }
        lock_ = false;
        return t->second->timeStatistics();
    }

    void printAllTime()
    {
        while (lock_.exchange(true));
        COCO_LOG(1) << "Printing time information for " << timer_list_.size() << " timers";
        for (auto &t : timer_list_)
        {
            auto &name = t.first;
            COCO_LOG(1) << "Name: " << name;
            COCO_LOG(1) << timeStatistics(name).toString();
        }
        lock_ = false;
    }
    void resetTimers()
    {
        while (lock_.exchange(true));
        timer_list_.clear();
        lock_ = false;
    }


private:
    TimerManager()
    { }

    std::unordered_map<std::string, std::shared_ptr<Timer> > timer_list_;
    std::atomic<bool> lock_ = {false};
};

}  // end of namespace util
}  // end of namespace coco
