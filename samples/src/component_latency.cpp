#include <chrono>
#include <thread>
#include <coco/coco.h>

class TaskLatStart : public coco::TaskContext
{
public:
    coco::OutputPort<double> out_time_ = {this, "time_OUT"};
    coco::Attribute<int long> asleep_time_ = {this, "sleep_time", sleep_time_};

    void init() {}
    void onConfig() {}

    void onUpdate()
    {
        auto time = coco::util::time();
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_));

        out_time_.write(time);
    }
private:
    int long sleep_time_ = 10;
};

COCO_REGISTER(TaskLatStart)

class TaskLatMiddle : public coco::TaskContext
{
public:
    coco::InputPort<double> in_time_ = {this, "time_IN", true};
    coco::OutputPort<double> out_time_ = {this, "time_OUT"};

    coco::Attribute<int long> asleep_time_ = {this, "sleep_time", sleep_time_};

    void init() {}
    void onConfig() {}

    void onUpdate()
    {
        //if (this->instantiationName() == "middle_2")
            std::cout  << "Executing start" << std::endl;
        double time;
        in_time_.read(time);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_));

        out_time_.write(time);

        //if (this->instantiationName() == "middle_2")
            std::cout  << "Executing end" << std::endl;
    }
private:
    int long sleep_time_ = 10;
};

COCO_REGISTER(TaskLatMiddle)


class TaskLatSink : public coco::TaskContext
{
public:
    coco::InputPort<double> in_time_ = {this, "time_IN", true};
    coco::OutputPort<double> out_time_ = {this, "time_OUT"};
    coco::Attribute<int long> asleep_time_ = {this, "sleep_time", sleep_time_};

    void init() {}
    void onConfig() {}

    void onUpdate()
    {
        double time;
        in_time_.read(time);

        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_));

        times_.push_back(coco::util::time() - time);
        out_time_.write(time);

        static int count = 1;
        if (count++ % 10 == 0)
        {
            double sum = std::accumulate(times_.begin(), times_.end(), 0);
            COCO_LOG(1) << "Latency: " << sum / times_.size();
        }
    }
private:
    std::vector<double> times_;
    int long sleep_time_ = 10;
};

COCO_REGISTER(TaskLatSink)