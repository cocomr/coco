/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#include <cassert>
#include <iomanip>

#include "coco/util/timing.h"
#include "coco/util/linux_sched.h"

#include "coco/task.h"
#include "coco/register.h"
#include "coco/execution.h"

namespace coco
{
uint32_t Activity::guid_gen = 0;

Activity::Activity(SchedulePolicy policy)
    : policy_(policy), active_(false), stopping_(false),
      guid_(guid_gen++)
{}

bool Activity::isPeriodic() const
{
    return policy_.scheduling_policy != SchedulePolicy::TRIGGERED;
}

SequentialActivity::SequentialActivity(SchedulePolicy policy)
    : Activity(policy)
{}

void SequentialActivity::start()
{
    this->entry();
}

void SequentialActivity::stop()
{
    if (active_)
    {
        stopping_ = true;
        if (!isPeriodic())
            trigger();
        else
        {
            // LIMIT: std::thread sleep cannot be interrupted
        }
    }
}

void SequentialActivity::trigger()
{}

void SequentialActivity::removeTrigger()
{}

void SequentialActivity::join()
{
    return;
}

std::thread::id SequentialActivity::threadId() const
{
    return std::thread::id();
}


void SequentialActivity::entry()
{
    for (auto &runnable : runnable_list_)
        runnable->init();
    /* PERIODIC */
    if (isPeriodic())
    {
        std::chrono::system_clock::time_point next_start_time;
        while (!stopping_)
        {
            next_start_time = std::chrono::system_clock::now() +
                              std::chrono::milliseconds(policy_.period_ms);
            for (auto &runnable : runnable_list_)
                runnable->step();
            std::this_thread::sleep_until(next_start_time);
        }
    }
    /* TRIGGERED */
    else
    {
        while (true)
        {
            /* wait on condition variable or timer */
            if (stopping_)
            {
                break;
            }
            else
            {
                for (auto &runnable : runnable_list_)
                    runnable->step();
            }
        }
    }
    active_ = false;

    for (auto &runnable : runnable_list_)
        runnable->finalize();
}

ParallelActivity::ParallelActivity(SchedulePolicy policy)
    : Activity(policy)
{}

void ParallelActivity::start()
{
    if (thread_)
        return;
    stopping_ = false;
    active_ = true;
    thread_ = std::move(std::unique_ptr<std::thread>(
            new std::thread(&ParallelActivity::entry, this)));
#if 0
#ifdef __linux__
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    if (policy_.affinity >= 0)
    {
        CPU_SET(policy_.affinity, &cpu_set);
    }
    else
    {
        for (auto i : policy_.available_core_id)
            CPU_SET(i, &cpu_set);
    }
    int res = pthread_setaffinity_np(thread_->native_handle(),
                                     sizeof(cpu_set_t), &cpu_set);
    if (res != 0)
    {
        COCO_FATAL() << "Failed to set affinity on core: " << policy_.affinity
                     << " with error: " << res << std::endl;
    }
#elif __APPLE__

#elif WIN

#endif
#endif
}

void ParallelActivity::stop()
{
    COCO_DEBUG("Activity") << "STOPPING ACTIVITY";
    if (thread_)
    {
        stopping_ = true;
        cond_.notify_all();
    }
}

void ParallelActivity::trigger()
{
    if (isPeriodic())
        return;
    
    ++pending_trigger_;
    cond_.notify_all();
}

void ParallelActivity::removeTrigger()
{
    if (pending_trigger_ > 0)
    {
        --pending_trigger_;
    }
}

void ParallelActivity::join()
{
    if (thread_)
    {
        if (thread_->joinable())
        {
            thread_->join();
        }
    }
}

std::thread::id ParallelActivity::threadId() const
{
    return thread_->get_id();
}

void ParallelActivity::setSchedule()
{
#ifdef __linux__

    /* Setting core affinity */
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    if (policy_.affinity >= 0 &&
        std::find(policy_.available_core_id.begin(),
                  policy_.available_core_id.end(),
                  policy_.affinity) != policy_.available_core_id.end())
        CPU_SET(policy_.affinity, &cpu_set);
    else
        for (auto i : policy_.available_core_id)
            CPU_SET(i, &cpu_set);
   
    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set) < 0)
        COCO_FATAL() << "Failed to set affinity on core: " << policy_.affinity;

    /* Setting linux real time scheduler */
    sched_attr sched;
    sched.size = sizeof(sched_attr);

    switch (policy_.realtime)
    {
        case SchedulePolicy::FIFO:
        {
            sched.sched_policy = SCHED_FIFO;
            sched.sched_priority = policy_.priority;
            break;
        }
        case SchedulePolicy::RR:
        {
            sched.sched_policy = SCHED_RR;
            sched.sched_priority = policy_.priority;
            break;
        }
        case SchedulePolicy::DEADLINE:
        {
            // setup_hr_tick()

            sched.sched_policy = SCHED_DEADLINE;
            sched.sched_runtime = policy_.runtime;
            sched.sched_deadline = policy_.period_ms * 10e6;  // Convert ms to ns
            sched.sched_period = policy_.period_ms * 10e6;  // Convert ms to ns
            break;
        }
        case SchedulePolicy::NONE:
        {
            return;
        }
    }
    int ret = sched_setattr(0, &sched, 0);
    if (ret < 0)
    {
        COCO_FATAL() << "Failed to setattr for thread: " << getpid() << " with guid: " << guid_;

        return;
    }

#elif __APPLE__

#elif WIN

#endif

}

void ParallelActivity::entry()
{
    setSchedule();

    for (auto &runnable : runnable_list_)
        runnable->init();

    /* PERIODIC */
    if (isPeriodic())
    {
        while (!stopping_)
        {
            auto now = std::chrono::system_clock::now();
            for (auto &runnable : runnable_list_)
                runnable->step();

            auto new_now = std::chrono::system_clock::now();
            auto sleep_time =
                    std::chrono::microseconds(policy_.period_ms * 1000) -
                    (new_now - now);
            if (sleep_time > std::chrono::microseconds(0))
            {
                std::unique_lock<std::mutex> mlock(mutex_);
                cond_.wait_for(mlock, sleep_time);
            }
        }
    }
    /* TRIGGERED */
    else
    {
        while (!stopping_)
        {
            /* wait on condition variable or timer */
            if (pending_trigger_ == 0)
            {
                std::unique_lock<std::mutex> mlock(mutex_);
                cond_.wait(mlock);
            }

            for (auto &runnable : runnable_list_)
                runnable->step();
        }
    }
    COCO_DEBUG("Activity") << "Stopping activity with task: "
                                       << std::dynamic_pointer_cast<
                                            ExecutionEngine>(
                                                runnable_list_.front())->
                                                    task()->instantiationName();
    active_ = false;
    for (auto &runnable : runnable_list_)
        runnable->finalize();
}

// -------------------------------------------------------------------
// Execution
// -------------------------------------------------------------------
ExecutionEngine::ExecutionEngine(std::shared_ptr<TaskContext> task)
    : task_(task)
{}

void ExecutionEngine::init()
{
    task_->setState(TaskState::INIT);
    task_->onConfig();
    COCO_DEBUG("Execution") << "[" << task_->instantiationName() << "] onConfig completed.";
    //COCO_DEBUG("Execution") << "Task " << task_->instantiationName() << " is on thread: " << pthread_self() << ", " <<  getpid();
    coco::ComponentRegistry::increaseConfigCompleted();
    task_->setState(TaskState::IDLE);
}

void ExecutionEngine::step()
{
    assert(task_ && "Trying executing an ExecutionEngine without a task");

    while (task_->hasPending())
    {
        task_->setState(TaskState::PRE_OPERATIONAL);
        task_->stepPending();
    }
    task_->setState(TaskState::RUNNING);

    if (ComponentRegistry::profilingEnabled())
    {
        if (latency_timer.source)
        {
            latency_timer.start_time = util::time();
        }

        timer_.start();
        task_->onUpdate();
        timer_.stop();

        if (latency_timer.tmp_time > 0)
        {
            latency_timer.start_time.exchange(latency_timer.tmp_time);
            latency_timer.tmp_time = -1;
        }
        if (latency_timer.target && latency_timer.start_time > 0)
        {
            latency_timer.tot_time += util::time() - latency_timer.start_time;

            ++ latency_timer.iterations;
            latency_timer.start_time = -1;
            {
                COCO_LOG_SAMPLE("Execution", 10) << "Latency is : " << double(latency_timer.tot_time / latency_timer.iterations) / 1000000.0;
            }
        }
    }
    else
    {
        task_->onUpdate();
    }
    task_->setState(TaskState::IDLE);
}

void ExecutionEngine::finalize()
{
    if (task_->state() != TaskState::STOPPED)
        task_->stop();
}

}  // end of namespace coco
