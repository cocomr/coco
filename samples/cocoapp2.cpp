#include "coco/coco.h"
#include <iostream>

int main(int argc, char * argv[])
{
	coco::ComponentRegistry::addLibrary("ezoroc1","lib");

	coco::TaskContext * p = coco::ComponentRegistry::create("EzTask1","EzTask1");
	p->setEngine(std::make_shared<coco::ExecutionEngine>(p)); // FIX THIS

	std::cout << "created " << p << std::endl;
	auto op = p->getOperation<int(int,int)>("adder");

	std::cout << "sum resul: " << op(1,2) << std::endl;

	return 1;
}