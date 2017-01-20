/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#include <coco/coco.h>
#include <stdlib.h>

class EzTask2: public coco::TaskContext
{
public:
	coco::Attribute<int> ac_ = {this, "c", c_};
	coco::Attribute<float> ad_ = {this, "d", d_};
	coco::InputPort<int> in_ = {this, "IN", true};
	coco::Operation<void(int) > op = {this, "hello", &EzTask2::hello, this};

	EzTask2()
	{
	}
	
	void hello(int x)
	{
		COCO_LOG(2) << "hello: " << x;
	}

	virtual void init()
	{
		
	}

	virtual void onConfig() 
	{

	}

	virtual void onUpdate() 
	{	
		COCO_LOG(2) << this->instantiationName() << " executiong update " << count_ ++;
		int recv;
		if (in_.read(recv) == coco::NEW_DATA)
		{
			COCO_LOG(2) << this->instantiationName() << " receiving " << recv;
		}
#ifndef WIN32
		usleep((random() / (double) RAND_MAX) * 1e6);
#endif
	}
	
private:
	int c_;
	float d_;
	int count_ = 0;
};

COCO_REGISTER(EzTask2)
/*
#ifdef WIN32
extern "C"
{
	__declspec(dllexport) coco::ComponentRegistry ** __stdcall getComponentRegistry() { return getComponentRegistryImpl(); }
}
#endif
*/