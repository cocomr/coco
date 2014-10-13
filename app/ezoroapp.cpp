#include "ezoro.hpp"
#include <iostream>

/*
class EzTask3: public coco::TaskContextT<EzTask3>
{
public:
	 coco::Property<int> a = {this,"a"};
	 coco::InputPort<int> p1 = {this,"p1"};
	 coco::OutputPort<int> p2 = {this,"p2"};

	virtual void onConfig()
	{

	}

	virtual void onUpdate() 
	{

	}

};

EZORO_REGISTER(EzTask3)
*/
int main(int argc, char * argv[])
{
	coco::CocoLauncher launcher("../app/config.xml");
	launcher.createApp();
	launcher.startApp();
	while(true);
	return 1;

}