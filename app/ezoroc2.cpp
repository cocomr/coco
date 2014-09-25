#include "ezoro.hpp"


class EzTask2: public coco::TaskContextT<EzTask2>
{
public:
	 coco::Property<int> a = {this,"b"};

	virtual void onConfig() 
	{

	}

	virtual void onUpdate() 
	{

	}

};

EZORO_REGISTER(EzTask2)