/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <list>

#include "coco/util/threading.h"



#include "coco/util/timing.h"

namespace coco
{

/*! \brief Policy for an activity instantiation.
 *  Specifies wheter the actvity has to be executed on a new thread
 *  or on the main thread
 */
enum class ThreadSpace { OWN_THREAD, CLIENT_THREAD};

/*! \brief Policy for the scheduling of activities.
 *  Contains information regarding the execution activation policy,
 *  and the eventual core affinity.
 */
struct SchedulePolicy
{
    /*! \brief Activity execution policy.
     *  Specifies the activity scheduling policy
     */
    enum Policy
    {
        PERIODIC,   //!< The activity executes periodically with a given period
        TRIGGERED   //!< The activity execution is triggered by an event port receiving data
    };
    /*! \brief Specify the realtime type of the activity
     */
    enum RealTime
    {
        NONE,
        FIFO,
        RR,
        DEADLINE
    };

    /*! \brief Base constructor with default values.
     */
    explicit SchedulePolicy(Policy policy = PERIODIC, int period = 1)
        : scheduling_policy(policy), period_ms(period) {}

    Policy scheduling_policy;  //!< Scheduling policy
    RealTime realtime = NONE;
    int period_ms;  //!< In case of a periodic activity specifies the period in millisecon
    int affinity = -1;  //!< Specifies the core id where to pin the activity. If -1 no affinity
    int priority = 0;
    int runtime = 0;
    std::list<unsigned int> available_core_id;  //!< Contains the list of the available cores where the activity can run
};

class RunnableInterface;

/** \brief The container for components
 *  A CoCo application is composed of multiple activities, each one containing components.
 *  Each activity is associated with a thread and scheduled according to \ref SchedulePolicy
 */
class COCOEXPORT Activity
{
public:
    /*! \brief Specifies the execution policy when instantiating an activity
     */
    explicit Activity(SchedulePolicy policy);

    virtual ~Activity() {};

    /*! \brief Starts the activity.
     */
    virtual void start() = 0;
    /*! \brief Stops the activity.
     *  Sets the termination flag and wakes up all the internal components.
     */
    virtual void stop() = 0;
    /*! \brief Called to resume the execution of a trigger activity.
     *  Increases the trigger count and wake up the execution.
     */
    virtual void trigger() = 0;
    /*! \brief Decreases the trigger count.
     *  When data from an event port is read decreases the trigger counter.
     */
    virtual void removeTrigger() = 0;
    /*! \brief Main execution function.
     *  Contains the main execution loop. Manage the period timer in case of a periodic activity
     *  and the condition variable for trigger activityies.
     *  For every execution step, iterates over all the components contained by the activity
     *  and call ExecutionEngine::step() function.
     */
    virtual void entry() = 0;
    /*! \brief Join on the thread containing the activity
     */
    virtual void join() = 0;
    /*!
     * \return the thread id associated with the activity.
     */
    virtual std::thread::id threadId() const = 0;
    /*!
     * \return If the activity is periodic.
     */
    bool isPeriodic() const;
    /*!
     * \return The period in millisecond of the activity.
     */
    int period() const { return policy_.period_ms; }
    /*!
     * \return If the actvity is running.
     */
    bool isActive() const { return active_; }
    /*!
     * \return The schedule policy of the activity.
     */
    SchedulePolicy & policy() { return policy_; }
    /*!
     * \return The schedule policy of the activity.
     */
    const SchedulePolicy & policy() const { return policy_; }
    /*! \brief Add a \ref RunnableInterface object to the activity.
     * \param runnable Shared pointer of a RunnableInterface objec.
     */
    void addRunnable(const std::shared_ptr<RunnableInterface> &runnable) { runnable_list_.push_back(runnable); }
    /*!
     * \return The list of all the RunnableInterface objects associated to the activity
     */
    const std::list<std::shared_ptr<RunnableInterface> > &runnables() const { return runnable_list_; }
    /*!
     * \return a global unique identifier for the activity
     */
    uint32_t id() const { return guid_; }
protected:
    std::list<std::shared_ptr<RunnableInterface> > runnable_list_;
    SchedulePolicy policy_;
    bool active_;
    std::atomic<bool> stopping_;

    static uint32_t guid_gen;
    const uint32_t  guid_;
};

/*! \brief Create an activity running on the main thread of the process.
 *  Maximum one sequential activity per application
 */
class COCOEXPORT SequentialActivity: public Activity
{
public:
    /*! \brief Specifies the execution policy when instantiating an activity
     */
    explicit SequentialActivity(SchedulePolicy policy);
    /*! \brief Starts the activity.
     *  Simply call entry().
     */
    void start() final;

    void stop() final;
    /*! \brief NOT IMPLEMENTED FOR SEQUENTIAL ACTIVITY
     */
    void trigger() final;
    /*! \brief NOT IMPLEMENTED FOR SEQUENTIAL ACTIVITY
     */
    void removeTrigger() final;
    /*! \brief Does nothing, nothing to join
     */
    void join() final;
    std::thread::id threadId() const final;
protected:

    void entry() final;
};

/*!
 * \brief Activity that run in its own thread
 */
class COCOEXPORT ParallelActivity: public Activity
{
public:
    explicit ParallelActivity(SchedulePolicy policy);
    /*! \brief Starts the activity.
     *  Moves the execution (entry() function) on a new thread
     *  and sets the affinity according to the \ref SchedulePolicy.
     */
    void start() final;
    void stop() final;
    void trigger() final;
    void removeTrigger() final;
    void join() final;
    std::thread::id threadId() const final;
protected:
    void setSchedule();
    void entry() final;

    std::atomic<int> pending_trigger_ = {0};
    std::unique_ptr<std::thread> thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

/*! \brief Interface that manages the execution of a component.
 *  It is in charge of the component initialization, loop function
 *  and pending operations.
 */
class RunnableInterface
{
public:
    virtual ~RunnableInterface() {}
    /*! \brief Calls the component initialization function
     */
    virtual void init() = 0;
    /*! \brief Execute one step of the loop.
     */
    virtual void step() = 0;
    /*! \brief It is called When the execution is stopped.
    */
    virtual void finalize() = 0;
protected:
};

class TaskContext;
/*! \brief Container to manage the execution of a component.
 *  It is in charge of the component initialization, loop function
 *  and pending operations.
 */
class COCOEXPORT ExecutionEngine: public RunnableInterface
{
public:
    /*! \brief Base constructor.
     *  \param task The component assosiaceted to the engine.
     *         One engine controlls only one component and
     *         one component must be associated to only on engine.
     *  \param profiling Flag to specify to the engine if to inject profiling
     *         call into the step function.
     */
    explicit ExecutionEngine(std::shared_ptr<TaskContext> task);


    /*! \brief Calls the TaskContext::onConfig() function of the associated task.
     */
    void init() final;
    /*! \brief Execution step.
     *  Iterate over the task pending operations executing them and then
     *  and then executes the TaskContext::onUpdate() function.
     */
    void step() final;
    /*! Call the component stop function, TaskContext::stop().
     */
    void finalize() final;
    /*!
     * \return The pointer to the associated component object.
     */
    std::shared_ptr<TaskContext> task() const { return task_; }
    /*!
     *  \return Aggregate time statistics about current execution
     */
    util::TimeStatistics timeStatistics()
    {
        return timer_.timeStatistics();
    }
    /*! \brief Reset the statistics for the current task
     */
    void resetTimeStatistics()
    {
        timer_.reset();
    }

private:
    std::shared_ptr<TaskContext> task_;
    //bool stopped_;

    util::Timer timer_;

public:
    struct LatencyTimer
    {
        std::atomic<int long> start_time = {-1};
        std::atomic<int long> tmp_time = {-1};
        int long tot_time = 0;
        unsigned long iterations = 0;
        bool source = false;
        bool target = false;
        bool start = true;
    };
    LatencyTimer latency_timer;
};

}  // end of namespace coco
