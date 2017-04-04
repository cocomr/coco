
# Build

Use cmake, then make

# Make Package

Build and then make package

# Make ROS Launcher

source ROS
cd ros_launcher
catkin_make
bloom-generate rosdebian --os-name ubuntu --os-version trusty --ros-distro indigo
cd src/ros_launcher && fakeroot debian/rules binary
