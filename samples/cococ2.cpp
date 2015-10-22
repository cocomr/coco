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


class EzTask2: public coco::TaskContext
{
public:
	coco::Attribute<int> ac_ = {this, "c", c_};
	coco::Attribute<float> ad_ = {this, "d", d_};
	coco::InputPort<int> in_ = {this, "IN", true};
	coco::Operation<void(int) > op = {this, "ciao", &EzTask2::ciao, this};

	EzTask2()
	{
	}
	
	void ciao(int x)
	{
		COCO_LOG(2) << "ciao: " << x;
	}

	virtual std::string info() { return ""; }
	void init()
	{
		std::cout << "attribute c: " << c_ << std::endl;
		std::cout << "attribute d: " << d_ << std::endl;
	}

	virtual void onConfig() 
	{

	}

	virtual void onUpdate() 
	{	
		static int count = 0;
		COCO_LOG(2) << this->instantiationName() << " executiong update " << count ++;
		
		if (in_.read(c_) == coco::NEW_DATA)
		{
			COCO_LOG(2) << this->instantiationName() << " receiving " << c_;
		}

		// static int count = 0;
		// auto f = this->getOperation<void(int)>("ciao");
		// if (f)
		// 	f(count++);
	}
	
private:
	int c_;
	float d_;

};

COCO_REGISTER(EzTask2)