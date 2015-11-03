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
		* owner    = this
		* name     = port name (unique in the same class)
		* is_event = specify wether the input port is an event port, this implies that when data is received the compoenent is triggered
	* `coco::OutputPort<T>(coco::TaskContext *owner, std::string name)`
		* owner = this
		* name  = attribute name (unique in the same class)
* Operation: allows to specify a function as an operation, this allows others to call your function (this is particulary usefull for peer compoenent as we will see later)
	* `coco::Operation<Function::Signature>(coco::TaskContext *owner, std::string name, Function func, Class obj)`
		* owner = this
		* name  = name of the operation, not mandatory to be equal to the name of the function
		* func  = the real function
		* obj   = the object containing the function
	* How to use operation:
		* Enqueue it in the owner task pending operation:
			`coco::TaskContext *task = COCO_TASK("TaskName")`
			`task->enqueueOperation<void(int)>("operation_name", parameters ...);`
		* Call it directly:
			`coco::TaskContext *task = COCO_TASK("TaskName")`
			`task->getOperation<void(int)>("operation_name")(parameter);`
			
In addition to base Task there is also Peer.
A `coco::PeerTask` is a `coco::TaskContex` which cannot run alone but must be embedded inside a Task.

To create a peer just inherite from `coco::PeerTask` and register the peer `COCO_REGISTER(ClassName)`.

Your peer class has to to ovveride only `info()` and `onConfig()`. The purpose of a Peer is to provide operations to whatever component needs it.

See the examples in the folder samples for more informations.ù

###Launcher###
Once you have created your componets and you have compiled them in shared libraries you have to specify to coco how this compoenents have to be run.

To do so you have to create an xml file with the following specifications:

* Outher tag:
```xml
<package>
</package>
```
* logconfig: (more information on how to use the log utilities will be provided later)
```xml
<logconfig>
    <levels>0 1 2 3 4</levels> <!-- logging enabled levels -->
    <outfile>log2.txt</outfile> <!--file where to write the logging, if empty or missing no log file produced -->
    <types>debug err log</types> <!-- enabled types -->
</logconfig>
```
* resourcepaths: allows to specify the path where to find the compoennt libraries and all the resources file passed to the attributes
```xml
<resourcespaths>
	<librariespath>path/to/component/libraries</librariespath>
	<path>path/to/resources</path>
	<path>path/to/more/resources</path>
</resourcespaths>
```
* activities specification: each activity represent a thread and can contains multiple component. Each component in an activty is executed according to the order in the xml file sequentially-
```xml
<activities>
	<activity>
		...
	</activity>
	...
	<activity>
		...
	</activity>
</activities>
```
* activity: each activity specify how it has to be executed and the component to execute
```xml
<activity>
	<schedulepolicy activity="parallel" type="periodic" value="1000" />
	<components>
		<compoenent>
			...
		</compoenent>
		...
		<compoenent>
			...
		</compoenent>
	</components>
</activity>
```

* schedulepolicy:
	* activity:
		* parallel: executed in a new dedicated thread
		 sequential: executed in the main process thread. No more than one sequential activity is allowed
	* type:
	* periodic: the activity run periodically with period "value" expressed in millisecond
		* triggered: the activity run only when triggered by receiving data in an event port of one of its component; "value" is ignored for triggered activity	

NOTE: if a triggered activity contains more than one compoent when it is triggered it will execute all the compoenent inside, no matter for which compoenent the triggered was ment.	

* compoenents: list of component
	* component: represent a TaskContext specification
```xml
<component>
	<task>TaskName</task>
	<name>NameForExecution</name> <!-- This name is mandatory only if we want to instantiate multiple compoenent of the same type and must be unique -->
	<library>libray_name</library> <!-- Name of the library without prefix and suffix-->
	<attributes>
		<attribute name="attr1" value="2" /> <!-- simply attribute, the type will be deduced at runtime and casted -->
		<attribute name="attr2" value="hello!" />
		<!-- To be more platform independent the launcher allow to specify the path where to find files at the beginning, the launcher will than look in all the path to find the resource if its type is set to "file" -->
		<attribute name="resource_file" value="data.txt" type="file" /> 
	</attributes>
	<components> <!-- List of peers attached to this components. A compoenent can have all the peers it wants. Also peers can have their own peer going deeper as wanted -->
		<compoenent>
			...
		</component>
	</components>
</component>
```
* connections: specifies the connections between compoenents ports.
```xml
<connections>
	<connection>
		...
	</connection>
	<connection>
		...
	</connection>
</connections>
```
* connection:
```xml
<connection data="BUFFER" policy="LOCKED" transport="LOCAL" buffersize="10">
	<src task="NameForExecution1" port="port_name_out"/>
	<dest task="NameForExecution2" port="port_name_in"/>
</connection>	
```
* data: type of port buffer
	* DATA: buffer lenght 1
	* BUFFER: FIFO buffer of lenght "buffersize"
	* CIRCULAR: circula FIFO buffer of lenght "buffersize"
* policy: policy of the locking system to be used
	* LOCKED: guarantee mutually exclusive access
	* UNSYNC: no synchronization applied
* transport: 
	* LOCAL: shared memory for thread
	* IPC: communication between processes
		
		
###Utils###
####Logging####

####Timing#####

	