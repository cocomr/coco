[![Build Status](https://travis-ci.org/cocomr/coco.svg?branch=master)](https://travis-ci.org/cocomr/coco)

#Compact Component Library: CoCo#

A C++ framework for high-performance multi-thread shared memory applications.
Split your application in component that can run in parallel to each other or be encapsulated one in the other.
Components can run periodically or be triggered by events. 
CoCo provides also a port system allowing the components to communicate with very low overhead without worrying of concurrency issues.

It has been widely used in robotics applications, ranging from Augmented Reality for teleoperation to haptic-rendering for virtual training. 

It is also compatible with [ROS](http://www.ros.org) allowing it to be used in any kind of high-level control applications.

##Usage##

The documentation for installing and using CoCo can be found on [readthedocs](http://coco.readthedocs.io/en/latest/)

If you have any problem understanding the documentation or you think the docs is not clear please write at filippobrizzi at gmail.com
or use the issue system of github.

##Use in Scientific Work##
In case you are going to use this work in a scientific publication please cite the following paper

  Ruffaldi E. & Brizzi F. (2016). CoCo - A framework for multicore visuo-haptics in mixed reality. In SALENTO AVR, 3rd International Conference on Augmented Reality, Virtual Reality and Computer Graphics (pp. 339-357). Springer. [doi:10.1007/978-3-319-40621-3_24](http://dx.doi.org/10.1007/978-3-319-40621-3_24) isbn:978-3-319-40620-6

Note that CoCo has been used in a number of other pubblications listed in References at the end of this README.md

##Contribution##

We are always open to people willing to help! If you found any bug or want to propose some new feature or improvement use the github issue system or write at filippobrizzi at gmail.com


##Authors##

This framework has been developed in the PERCRO Laboratory of the Scuola Superiore Sant'Anna, Pisa by
Filippo Brizzi and [Emanuele Ruffaldi](http://eruffaldi.com)
A special thanks to Nicola Giordani for the web viewer and the review of the code.

##References##

- (CoCo) Ruffaldi E. & Brizzi F. (2016). CoCo - A framework for multicore visuo-haptics in mixed reality. In SALENTO AVR, 3rd International Conference on Augmented Reality, Virtual Reality and Computer Graphics (pp. 339-357). Springer. [doi:10.1007/978-3-319-40621-3_24](http://dx.doi.org/10.1007/978-3-319-40621-3_24) isbn:978-3-319-40620-6 - [Slides PDF](https://dl.dropboxusercontent.com/u/146279/papers/2016_C_RuffaldiAVRCOCO/slides.pdf)


- (AR with Baxter) Ruffaldi E., Brizzi F., Bacinelli S. & Tecchia F. (2016). Third point of view Augmented Reality for robot intentions visualization. In SALENTO AVR, 3rd International Conference on Augmented Reality, Virtual Reality and Computer Graphics (pp. 471-478). Springer. [doi:10.1007/978-3-319-40621-3_35](http://dx.doi.org/10.1007/978-3-319-40621-3_35) isbn:978-3-319-40620-6

- (AR Teleop with Baxter) Peppoloni L., Brizzi F., Ruffaldi E. & Avizzano C.A. (2015). Augmented Reality-aided Tele-presence System for Robot Manipulation in Industrial Manufacturing. In Proceedings of the 21st ACM Symposium on Virtual Reality Software and Technology (VRST) (pp. 237-240). ACM. [doi:10.1145/2821592.2821620](http://dx.doi.org/10.1145/2821592.2821620) isbn:978-1-4503-3990-2

- (AR Encountered Haptics V2) Filippeschi A., Brizzi F., Ruffaldi E., Jacinto J.M. & Avizzano C.A. (2015). Encountered-type haptic interface for virtual interaction with real objects based on implicit surface haptic rendering for remote palpation. In IEEE IROS Proceedings (pp. 5904-5909). . [doi:10.1109/IROS.2015.7354216](http://dx.doi.org/10.1109/IROS.2015.7354216)

- (Co-Located Haptics) Ruffaldi E., Brizzi F., Filippeschi A. & Avizzano C.A. (2015). Co-Located haptic interaction for Virtual USG exploration. In Proceedings of IEEE EMBC (pp. 1548-1551). . [doi:10.1109/EMBC.2015.7318667](http://dx.doi.org/10.1109/EMBC.2015.7318667)

- (AR Teleop with Youbot) Peppoloni L., Brizzi F., Avizzano C.A. & Ruffaldi E., Immersive ROS-integrated framework for robot teleoperatio, Proceedings of IEEE 3DUI (pp. 177-178). ,2015 [doi:10.1109/3DUI.2015.7131758](http://dx.doi.org/10.1109/3DUI.2015.7131758) - [Video mp4](https://dl.dropboxusercontent.com/u/146279/papers/2015_C_3DUI_Peppoloni/video.mp4)

- (AR Encountered Haptivs V1) Ruffaldi Emanuele, Filippeschi Alessandro, Jacinto Juan Manuel, Brizzi Filippo & Avizzano Carlo Alberto, Encountered haptic Augmented Reality interface for remote examination. Proceedings of IEEE 3DUI (pp. 179-180) ,2015 [doi:10.1109/3DUI.2015.7131759](http://dx.doi.org/10.1109/3DUI.2015.7131759) - [Video mp4](https://dl.dropboxusercontent.com/u/146279/papers/2015_C_3DUI_Ruffaldi/video.mp4).
