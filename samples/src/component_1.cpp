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

class EzTask1: public coco::TaskContext
{
public:

	coco::Attribute<int> aa_ = {this, "a", a_};
	coco::Attribute<float> ab_ = {this, "b", b_};
	coco::Attribute<std::vector<int> > avec_ = {this, "vec", vec_};

	 EzTask1()
	 {
	 	setDoc("Basic EzTask1 Component");
	 	aa_.setDoc("attribute aa");
	 	ab_.setDoc("attribute ab");
		out_.setDoc("someport");
	 }

	virtual void init()
	{
		// This function is called in the main thread before spawining the thread 
	}

	virtual void onConfig() 
	{
		// This function is called in the dedicated thread
	}

	// The function called in the loop
	virtual void onUpdate() 
	{
		COCO_LOG(2) << "sending " << a_ << std::endl;
		COCO_LOG(2) << "VEC ";

		for (auto &v : vec_)
			COCO_LOG(2) << v;

		out_.write(a_);
		++a_;
		out_.write(a_);
		++a_;
		out_.write(a_);
		++a_;		

        auto task = COCO_TASK("EzTask2"); // This macro allows to retreive any task
		if (task)
			// Enqueue on task "EzTask2" the operation hello()
			// This works only if EzTask2 add hello as operation
			task->enqueueOperation<void(int)>("hello", count_ ++); 

		// Iterate over each peer and call their function run() if they have it
		for (auto peer : peers())
		{
			auto op = peer->operation<void(int)>("run");
			if (op)
				op(count_ * 2);
		}

#ifndef WIN32
		usleep((random() / (double) RAND_MAX) * 1e6);
#endif
	}

	coco::OutputPort<int> out_ = {this, "OUT"}; // Output port to send msgs
private:
	int a_;
	float b_;
	int count_ = 0;
	std::vector<int> vec_;

};

COCO_REGISTER(EzTask1)

// Peer component,
class EzTask3 : public coco::PeerTask
{
public:
	std::string info() { return "This is a peer compoenent"; }
	void onConfig()
	{
		// Do initi stuff here
	}

	void run(int a)
	{
		COCO_LOG(2) << "Peer: " << a;
	}

	coco::Operation<void(int)> orun_ = {this, "run", &EzTask3::run, this}; // Add run as operation
private:
};

// Another simple task
COCO_REGISTER(EzTask3)

class EzTask4 : public coco::TaskContext
{
public:
	std::string info() { return ""; }
	
	void init()
	{

	}
	void onConfig()
	{

	}

	void onUpdate()
	{
		COCO_LOG(2) << "EzTask4 Exectuing " << count_++;

#ifndef WIN32
		usleep((random() / (double) RAND_MAX) * 1e6);
#endif
	}

	coco::OutputPort<int> size_out_ = {this, "bohOUT"};
private:
	int count_ = 0;
};

COCO_REGISTER(EzTask4)

