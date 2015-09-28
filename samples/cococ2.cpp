#include "coco/coco.h"


class EzTask2: public coco::TaskContextT<EzTask2>
{
public:
	coco::Attribute<int> ac_ = {this, "c", c_};
	coco::Attribute<float> ad_ = {this, "d", d_};

	EzTask2() {
		coco::SchedulePolicy policy(coco::SchedulePolicy::TRIGGERED);
    	this->setActivity(createParallelActivity(policy, engine_));
	}
	
	virtual std::string info() { return ""; }
	void init() {
		std::cout << "attribute c: " << c_ << std::endl;
		std::cout << "attribute d: " << d_ << std::endl;
	}

	virtual void onConfig() 
	{

	}

	virtual void onUpdate() 
	{	
		if (in_.read(c_) == coco::NEW_DATA)
			std::cout << "c: " << c_ << std::endl;
	}
	coco::InputPort<int> in_ = {this, "IN", true};
private:
	int c_;
	float d_;

};

COCO_REGISTER(EzTask2)