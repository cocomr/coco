/**
Copyright 2015, Filippo Brizzi"
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@Author 
Filippo Brizzi, PhD Student
fi.brizzi@sssup.it
Emanuele Ruffaldi, PhD
e.ruffaldi@sssup.it

PERCRO, (Laboratory of Perceptual Robotics)
Scuola Superiore Sant'Anna
via Luigi Alamanni 13D, San Giuliano Terme 56010 (PI), Italy
*/
#include "core_impl.hpp"
#include "register.h"

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
		COCO_LOG(2) << this->instantiationName() << " sending " << a_ << std::endl;
		COCO_LOG(2) << this->instantiationName() << " VEC ";
		for (auto v : vec_)
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
	}

	coco::OutputPort<int> size_out_ = {this, "bohOUT"};
private:
	int count_ = 0;
};

COCO_REGISTER(EzTask4)

