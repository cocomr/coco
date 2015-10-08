#include "profiling.h"

namespace coco
{
namespace util
{

Profiler::Profiler(std::string name) 
	: name_(name)
{
	// TODO use chrono
	//start_time_ = clock();
	start_time_ = std::chrono::system_clock::now();
	ProfilerManager::getInstance()->addServiceTime(name_, std::chrono::system_clock::now());
}

Profiler::~Profiler()
{
	//double elapsed_time = ((double)(clock() - start_time_)) / CLOCKS_PER_SEC;

	double elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(
			std::chrono::system_clock::now() - start_time_).count() / 1000000.0;
	ProfilerManager::getInstance()->addTime(name_, elapsed_time);
}

ProfilerManager::ProfilerManager(const std::string &log_file)
{
	if (log_file.compare("") != 0)
		log_file_.open(log_file);	
}

ProfilerManager::~ProfilerManager()
{
	log_file_.close();
}

ProfilerManager* ProfilerManager::getInstance()
{
	const std::string log_file = "";	
	static ProfilerManager log(log_file);
	return &log;
}

void ProfilerManager::addTime(std::string id, double elapsed_time)
{
	std::unique_lock<std::mutex> mlock(mutex_);
	time_list_[id].push_back(elapsed_time);

	if (time_accumulator_.find(id) == time_accumulator_.end())
	{
		time_accumulator_[id].first = 0;
		time_accumulator_[id].second = 0.0;	
	}

	time_accumulator_[id].first += 1;
	time_accumulator_[id].second += elapsed_time;
	if (time_list_[id].size() % 500 == 0 && time_list_[id].size() > 0 && false)
	{
		printStatistics();
	}

}

void ProfilerManager::addServiceTime(std::string id, std::chrono::system_clock::time_point service_time)
{
	std::unique_lock<std::mutex> mlock(mutex_);
	service_time_list_[id].push_back(service_time);

}

void ProfilerManager::printStatistic(std::string id) 
{
	std::cout << id << std::endl;
	if (time_list_[id].size() > 0)
	{ 
		double tot_time = 0;
		for(auto &itr : time_list_[id])
			tot_time += itr;

		double avg_time = tot_time / time_list_[id].size();
		std::cout << "\tExecution time: " << avg_time <<
					 " s on " << time_list_[id].size() << " iterations\n";
		std::cout << "\tExecution time: " << time_accumulator_[id].second / time_accumulator_[id].first <<
					 " s on " << time_accumulator_[id].first << " iterations\n";
		if (log_file_.is_open())
			log_file_ << id << " " << avg_time << 
						 " s on " << time_list_[id].size() << " iterations\n";
	}
// Service Time
	if (service_time_list_[id].size() > 0)
	{
		double tot_time = 0;
		//clock_t tmp = service_time_list_[id][0];
		for (int i = 1; i < service_time_list_[id].size(); ++i)
		{
			//tot_time += (double)(service_time_list_[id][i] - service_time_list_[id][i - 1]) / CLOCKS_PER_SEC;
			tot_time += (std::chrono::duration_cast<std::chrono::microseconds>(service_time_list_[id][i] - service_time_list_[id][i - 1]).count() / 1000000.0);
			
		
			//tmp = service_time_list_[id][i];
		}
		
		double avg_time = tot_time / (service_time_list_[id].size() - 1);
		std::cout << "\tService Time: " << " " << avg_time << " s on " << service_time_list_[id].size() << " iterations\n";

		if (log_file_.is_open())
			log_file_ << "Service Time: " << " " << avg_time << " s on " << service_time_list_[id].size() << " iterations\n";
	}
}

void ProfilerManager::printStatistics()
{
	std::unique_lock<std::mutex> mlock(mutex_);
	std::cout << "Statistics of " << time_list_.size() << " components\n";
	for (auto &t : time_list_)
		printStatistic(t.first);
}

void ProfilerManager::resetStatistics()
{
	std::unique_lock<std::mutex> mlock(mutex_);
	time_list_.clear();
	service_time_list_.clear();
	std::cout << "Statistics cleared\n";
}

} // End of namespace util
} // End of namespace coco