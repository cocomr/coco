#include <cassert>

#include "coco/util/timing.h"

#include "coco/task.h"
#include "coco/register.h"

#include "coco/execution.h"

namespace coco
{

Activity::Activity(SchedulePolicy policy)
    : policy_(policy), active_(false), stopping_(false)
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
    // PERIODIC
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
    // TRIGGERED
    else
    {
        while (true)
        {
            // wait on condition variable or timer
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

void ParallelActivity::entry()
{
    for (auto &runnable : runnable_list_)
        runnable->init();

    // PERIODIC
    if (isPeriodic())
    {
        std::chrono::system_clock::time_point next_start_time;
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
    // TRIGGERED
    else
    {
        while (true)
        {
            // wait on condition variable or timer
            if (pending_trigger_ == 0)
            {
                std::unique_lock<std::mutex> mlock(mutex_);
                cond_.wait(mlock);
            }

            if (stopping_)
            {
                COCO_DEBUG("Activity") << "Stopping activity with task: "
                                       << std::dynamic_pointer_cast<
                                            ExecutionEngine>(
                                                runnable_list_.front())->
                                                    task()->instantiationName();
                break;
            }

            for (auto &runnable : runnable_list_)
                runnable->step();
        }
    }
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
    task_->setState(INIT);
    task_->onConfig();
    COCO_DEBUG("Execution") << "[" << task_->instantiationName() << "] onConfig completed.";
    coco::ComponentRegistry::increaseConfigCompleted();
    task_->setState(IDLE);
}

void ExecutionEngine::step()
{
    assert(task_ && "Trying executing an ExecutionEngine without a task");

    while (task_->hasPending())
    {
        task_->setState(PRE_OPERATIONAL);
        task_->stepPending();
    }
    task_->setState(RUNNING);

    if (ComponentRegistry::profilingEnabled())
    {
        COCO_START_TIMER(task_->instantiationName())
        task_->onUpdate();
        COCO_STOP_TIMER(task_->instantiationName())
    }
    else
    {
        task_->onUpdate();
    }
    task_->setState(IDLE);
}

void ExecutionEngine::finalize()
{
    if (task_->state() != STOPPED)
        task_->stop();
}

}  // end of namespace coco
