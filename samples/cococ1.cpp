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

	 EzTask1()
	 {
	 	setDoc("Basic EzTask1 Component");
	 	aa_.setDoc("attribute aa");
	 	ab_.setDoc("attribute ab");
		out_.setDoc("someport");
	 }

	 int adder(int a, int b)
	 {
	 	return a+b;
	 }

	virtual std::string info() { return ""; }
	virtual void init()
	{
	}

	virtual void onConfig() 
	{

	}

	virtual void onUpdate() 
	{
		COCO_LOG(2) << this->instantiationName() << " sending " << a_ << std::endl;
		out_.write(a_);
		++a_;
		out_.write(a_);
		++a_;
		out_.write(a_);
		++a_;		

		coco::TaskContext *t = COCO_TASK("EzTask2")
		static int count = 0;
		if (t)
			t->enqueueOperation<void(int)>("ciao", count ++);

		for (auto peer : getPeers())
		{
			auto op = peer->getOperation<void(int)>("run");
			if (op)
				op(count * 2);
		}

	}
	coco::OutputPort<int> out_ = {this, "OUT"};
private:
	int a_;
	float b_;

};

COCO_REGISTER(EzTask1)

class EzTask3 : public coco::PeerTask
{
public:
	std::string info() { return ""; }
	void onConfig()
	{

	}

	void run(int a)
	{
		COCO_LOG(2) << "Peer: " << a;
	}

	coco::Operation<void(int)> orun_ = {this, "run", &EzTask3::run, this};
private:
};

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
		static int count = 0;
		COCO_LOG(2) << "EzTask4 Exectuing " << count ++;
	}
};

COCO_REGISTER(EzTask4)

