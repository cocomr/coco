#include "ezoro.hpp"


class EzTask1: public coco::TaskContextT<EzTask1>
{
public:

	coco::AttributeRef<int> aa_ = {this, "a", a_};
	coco::AttributeRef<float> ab_ = {this, "b", b_};

	 EzTask1()
	 {
	 	doc("Basic EzTask1 Component");
	 	aa_.doc("attribute aa");
	 	ab_.doc("attribute ab");
		out_.doc("someport");
	 	coco::SchedulePolicy policy(coco::SchedulePolicy::PERIODIC, 1000);
    	this->setActivity(createParallelActivity(policy, engine_));
    	addOperation("adder",&EzTask1::adder,this);
	 }

	 int adder(int a, int b)
	 {
	 	return a+b;
	 }

	virtual std::string info() { return ""; }
	virtual void init() {
		std::cout << "attribute a: " << a_ << std::endl;
		std::cout << "attribute b: " << b_ << std::endl;
	}

	virtual void onConfig() 
	{

	}

	virtual void onUpdate() 
	{
		out_.write(a_);
		++ a_;

	}
	coco::OutputPort<int> out_ = {this, "OUT"};
private:
	int a_;
	float b_;

};

EZORO_REGISTER(EzTask1)