#Compact Component Library: CoCo#

A C++ framework for high-performance multi-thread shared memory applications.
Split your application in component that can run in parallel to each other or be encapsulated one in the other.
Components can run periodically or be triggered by events. 
CoCo provides also a port system allowing the components to communicate with very low overhead without worrying of concurrency issues.

This framework has been developed in the Percro Laboratory of the Scuola Superiore Sant'Anna, Pisa by Filippo Brizzi and Emanuele Ruffaldi.

It has been widely used in robotics applications, ranging from Augmented Reality for teleoperation to haptic-rendering for virtual training. 

It is also compatible with ROS allowing it to be used in any kind of high-level control applications.

##Why and What##
Orocos inspired, but:

* Lightweight library fully based on C++11.
* Timing oriented:
	* Real-time focused
	* Latency oriented
	* Low overhead communications
* ROS compliant

-> Simpler and Powerful

##Folder Structure##
* src: contains the core library features
* util: contains some utilities that are independent from the core files and can also be used standalone. This features are:
    * loggin simple logging capabilities
    * timing allows to simple get duration time in specified intervals and it is used to profile the component execution
* launcher: contains the executable that allows to launch a coco application, this 
* extern: just an xml parser
* samples: contains two sample component togheter with the configuration file to launch them

##Installation##
Coco comes with a cmake file.
	$ mkdir build && cd build
	$ cmake .. -DCMAKE_INSTALL_PREFIX=prefix/to/install/directory
	$ make -j
	$ sudo make install

The last command will install the binary `coco_launcher` that is the launcher of every coco application and `libcoco.so`.

To compile your application with coco add to your CMakeLists.txt file
find_package(coco REQUIRED)
include_directories(coco_INCLUDE_DIRS)
target_link_libraries(... ${coco_LIBS} ...)

##Usage##
The usage of the framework is divided in two part the first is the real C++ code, while the second is the creation of the configuration file where you specify
all the properties.
###C++ code###
samples/component_1.cpp and component_2.cpp contains examples of how to implement a coco component or peer.

To create a component your class has to inherit from `coco::TaskContext` and you have to register your class with the macro `COCO_REGISTER(ClassName)`.

The class has to override the following method:

* `info() : std::string` returns a description of the compoenent
* `init() : void` called in the main thread before moving the component in its thread
* `onConfig() : void` first function called after been moved in the new thread
* `onUpdate() : void` main function called at every loop iteration

```cpp
class MyComponent : public coco::TaksContext
{
public:
	virtual std::string info()
	{
		return "MyComponent does ...";
	}
	virtual void init()
	{}
	virtual void onConfig()
	{}
	virtual void onUpdate()
	{}
};
COCO_REGISTER(MyCompoent)
```

Each component can embedd the following objects:

* Attribute: allows to specify a class variable as an attribute, this allow to set its variable from the configuration file
	* `coco::Attribute<T>(coco::TaskContext *owner, std::string name, T var)`
		* owner = this
		* name  = attribute name (unique in the same class)
		* var   = class variable that we want to attach to the attribute 
* Port: input or output it is used to communicate with the other components
	* `coco::InputPort<T>(coco::TaskContext *owner, std::string name, bool is_event = false)`
		* owner = this
		* name = port name (unique in the same class)
		* is_event = specify wether the input port is an event port, this implies that when data is received the compoenent is triggered
	* `coco::OutputPort<T>(coco::TaskContext *owner, std::string name)`
		* owner = this
		* name  = attribute name (unique in the same class)
* Operation: allows to specify a function as an operation, this allows others to call your function (this is particulary usefull for peer compoenent as we will see later)
	* `coco::Operation<Function::Signature>(coco::TaskContext *owner, std::string name, Function, Class obj)`



A `coco::PeerTask` is `coco::TaskContex` which cannot run alone but must be embedded inside a Component.

To create a peer just inherite from `coco::PeerTask` and register the peer `COCO_REGISTER(ClassName)`.

Your peer class has to to ovveride only `info()` and `onConfig()'.




###Launcher###
XML launcher file




	