#include "core_impl.hpp"
#include "register.h"


class EzTask2: public coco::TaskContext
{
public:
	coco::Attribute<int> ac_ = {this, "c", c_};
	coco::Attribute<float> ad_ = {this, "d", d_};
	coco::InputPort<int> in_ = {this, "IN", true};
	coco::Operation<void(int) > op = {this, "ciao", &EzTask2::ciao, this};
	EzTask2()
	{
	}
	
	void ciao(int x)
	{
		std::cout << "ciao: " << x << std::endl;
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

		static int count = 0;
		auto f = this->getOperation<void(int)>("ciao");
		if (f)
			f(count++);
	}
	
private:
	int c_;
	float d_;

};

COCO_REGISTER(EzTask2)