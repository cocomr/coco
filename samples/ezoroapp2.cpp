#include "ezoro.hpp"
#include <iostream>

int main(int argc, char * argv[])
{
	coco::ComponentRegistry::addLibrary("ezoroc1","lib");

	coco::TaskContext * p = coco::ComponentRegistry::create("EzTask1");

	std::cout << "created " << p << std::endl;
	auto op = p->getOperation<int(int,int)>("adder");

	std::cout << "sum resul: " << op(1,2) << std::endl;

	return 1;
}