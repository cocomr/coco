#include "coco/coco.h"


class EzTask1: public coco::TaskContext
{
public:

	coco::Attribute<int> aa_ = {this, "a", a_};
	coco::Attribute<float> ab_ = {this, "b", b_};

	 EzTask1()
	 {
	 	setDoc("Basic EzTask1 Component");
	 	aa_.setDoc("attribute aa");
	 	ab_.setDoc("attribute ab");
		out_.setDoc("someport");
	 	//coco::SchedulePolicy policy(coco::SchedulePolicy::PERIODIC, 1000);
    	//this->setActivity(createParallelActivity(policy, engine_));
    	//addOperation("adder",&EzTask1::adder,this);
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

		coco::TaskContext *t = coco::ComponentRegistry::task("EzTask2");
		static int count = 0;
		if (t)
			t->enqueueOperation<void(int)>("ciao", count ++);

	}
	coco::OutputPort<int> out_ = {this, "OUT"};
private:
	int a_;
	float b_;

};

COCO_REGISTER(EzTask1)