#include "ezoro.hpp"
#include <iostream>


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

int main(int argc, char * argv[])
{
	coco::ComponentRegistry::addLibrary("ezoroc1");
	coco::ComponentRegistry::addLibrary("ezoroc2");

	std::cout << "build\n";
	coco::TaskContext * c1 = coco::ComponentRegistry::create("EzTask1");
	std::cout << "built " << c1 << std::endl;
	coco::TaskContext * c2 = coco::ComponentRegistry::create("EzTask1");
	std::cout << "built " << c2 << std::endl;

	std::cout <<"invokin op" << std::endl;
	std::cout << "res " << c1->operations	().front()->as<float (int x)>()(10) << std::endl;

	std::cout <<"connecting\n";
	((EzTask3*)c1)->p1.connectTo(&((EzTask3*)c2)->p2,coco::ConnectionPolicy());
	std::cout <<"connected\n";


	c1->start();
	c2->start();

}