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

#include <coco/coco.h>


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
	}
	
private:
	int c_;
	float d_;
	int count_ = 0;
};

COCO_REGISTER(EzTask2)
