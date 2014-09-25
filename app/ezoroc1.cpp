#include "ezoro.hpp"


class EzTask1: public coco::TaskContextT<EzTask1>
{
public:
	 coco::Property<int> a = {this,"a"};

	 float pippo(int x)
	 {

	 	std::cout << "ciao pippo " << x << std::endl;
	 	return x*2;
	 }

	 EzTask1()
	 {
	 	std::cout << "EzTask1"<<std::endl;
	 	addOperator("pippo",this,&EzTask1::pippo);
	 }

	virtual void onConfig() 
	{

	}

	virtual void onUpdate() 
	{

	}


};

EZORO_REGISTER(EzTask1)