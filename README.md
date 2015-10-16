#Compact Component Library: CoCo#

A C++ framework for high-performance multi-thread shared memory applications.
Split your application in component that can run in parallel to each other or be encapsulated one in the other. These components can run periodically or be triggered by events. CoCo provides also a port system allowing the components to communicate with very low overhead without worrying of concurrency issues.

This framework has been developed in the Percro Laboratory of the Scuola Superiore Sant'Anna, Pisa by Filippo Brizzi and Emanuele Ruffaldi and has been widely used in robotics applications, ranging from Augmented Reality for teleoperation to haptic-rendering for virtual training. It is also compatible with ROS allowing it to be used in any kind of high-level control applications.


##Features and Strenght##
Orocos inspired, but 
* Lightweight library fully based on C++11.
* Timing oriented:
	* Real-time focused
	* Latency oriented
	* Low overhead communications
* ROS compliant


##Folder Structure##
* src: contains the core library features
* util: contains some utilities that are independent from the core files and can also be used standalone. This features are:
    * loggin simple logging capabilities
    * timing allows to simple get duration time in specified intervals and it is used to profile the component execution
* launcher: contains the executable that allows to launch a coco application, this 
* extern: just an xml parser
* samples: contains two sample component togheter with the configuration file to launch them