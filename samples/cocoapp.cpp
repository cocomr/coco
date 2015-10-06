#include "coco/coco.h"
#include <iostream>
#include "coco/loader/loader.h"
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
	std::cout << "CIAO 0 \n";
	launcher.createApp();
	std::cout << "CIAO 1\n";
	launcher.startApp();
	while(true);
	return 1;

}