/**
 * Compact Framework Core
 * 2014-2015 Emanuele Ruffaldi and Filippo Brizzi @ Scuola Superiore Sant'Anna, Pisa, Italy
 */
#include <iostream>
#include <thread>
#include <chrono>

#include "coco/coco_rt.hpp"

namespace coco{
// -------------------------------------------------------------------
// Connection
// -------------------------------------------------------------------

void ConnectionBase::trigger()
{
	input_->triggerComponent();
}

bool ConnectionBase::hasComponent(const std::string &name) const
{
    if (input_->taskName() == name)
        return true;
    if (output_->taskName() == name)
        return true;
    return false;
}


// -------------------------------------------------------------------
// Execution
// -------------------------------------------------------------------


void ExecutionEngine::init()
{
	task_->onConfig();
}

void ExecutionEngine::step()
{

	//processMessages();
    //processFunctions();
    if (task_)
    {
    	if (task_->state_ == RUNNING)
    	{
    		//processFunctions();
    		while (task_->hasPending())
			{
    			std::cout << "Execution function\n";
    			task_->stepPending();
			}
#ifdef PROFILING    		
    		util::Profiler log(task_->name());
#endif
    		task_->onUpdate();
    	}
    }
}

void ExecutionEngine::finalize()
{
}

Activity * createSequentialActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r = 0)
{
	return new SequentialActivity(sp, r);
}

Activity * createParallelActivity(SchedulePolicy sp, std::shared_ptr<RunnableInterface> r = 0) 
{
	return new ParallelActivity(sp, r);
}

void SequentialActivity::start() 
{
	this->entry();
}

void SequentialActivity::entry()
{
	runnable_->init();
	if(isPeriodic())
	{
		std::chrono::system_clock::time_point currentStartTime{ std::chrono::system_clock::now() };
		std::chrono::system_clock::time_point nextStartTime{ currentStartTime };
		while(!stopping_)
		{
			currentStartTime = std::chrono::system_clock::now();
			runnable_->step();
			nextStartTime =	currentStartTime + std::chrono::milliseconds(policy_.period_ms_);
			std::this_thread::sleep_until(nextStartTime); // NOT interruptible, limit of std::thread
		}
	}
	else
	{
		while(true)
		{
			// wait on condition variable or timer
			if(stopping_)
				break;
			else
				runnable_->step();
		}
	}
	runnable_->finalize();

}

void SequentialActivity::join()
{
	while(true);
}

void SequentialActivity::stop()
{
	if (active_)
	{
		stopping_ = true;
		if(!isPeriodic())
			trigger();
		else
		{
			// LIMIT: std::thread sleep cannot be interrupted
		}
	}
}

void ParallelActivity::start() 
{
	if(thread_)
		return;
	stopping_ = false;
	active_ = true;
	thread_ = std::move(std::unique_ptr<std::thread>(new std::thread(&ParallelActivity::entry,this)));
}

void ParallelActivity::join()
{
	if(thread_)
	thread_->join();
}

void ParallelActivity::stop() 
{
	std::cout << "STOPPING ACTIVITY\n";
	if(thread_)
	{
		{
			std::unique_lock<std::mutex> mlock(mutex_);
			stopping_ = true;
		}
		if(!isPeriodic())
			trigger();
		else
		{
			// LIMIT: std::thread sleep cannot be interrupted
		}
		thread_->join();
		COCO_LOG(10) << "Thread join";
	}
}

void ParallelActivity::trigger() 
{
	std::unique_lock<std::mutex> mlock(mutex_);
	pending_trigger_++;
	cond_.notify_one();
}

void ParallelActivity::entry()
{
	runnable_->init();
	if(isPeriodic())
	{
		std::chrono::system_clock::time_point currentStartTime{ std::chrono::system_clock::now() };
		std::chrono::system_clock::time_point nextStartTime{ currentStartTime };
		while(!stopping_)
		{
			currentStartTime = std::chrono::system_clock::now();
			runnable_->step();
			nextStartTime =		 currentStartTime + std::chrono::milliseconds(policy_.period_ms_);
			std::this_thread::sleep_until(nextStartTime); // NOT interruptible, limit of std::thread
		}
	}
	else
	{
		while(true)
		{
			// wait on condition variable or timer
			{
				std::unique_lock<std::mutex> mlock(mutex_);
				if(pending_trigger_ > 0) // TODO: if pendingtrigger is ATOMIC then skip the lock
				{
					pending_trigger_ = 0;
				}
				else
					cond_.wait(mlock);
			}
			if(stopping_)
				break;
			else
				runnable_->step();
		}
	}
	runnable_->finalize();
}
}
