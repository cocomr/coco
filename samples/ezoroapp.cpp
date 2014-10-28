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

/**
 * Idea Alternatives

 extract "_spec" from archive
extern coco::ComponentSpec EzTask1_spec;
coco::ComponentSpec *components [] =
{
	&EzTask1_spec
};
 
 */

int main(int argc, char * argv[])
{
	coco::CocoLauncher launcher(argc > 1 ? argv[1] : "../samples/config.xml");
	launcher.createApp();
	launcher.startApp();
	while(true);
	return 1;

}