iCub Tactile Grasp
=================

The Tactile Grasp is a module which perfors several types of object grasping using different levels of control and compliance.
Available grasping modes are:
 * Soft grasp
 * Crush grasp


##Soft Grasp
The Soft Grasp uses feedback from the fingertip tactile sensors to stop the grasping action once contact with an object is detected.


##Crush Grasp
The Crush Grasp does not use any feedback and will continue with the grasping action regardless if the fingertips are sensing anything.


##Documentation
To generate the documentation for this project run:
```bash
    doxygen conf/Doxyfile
```
from the root of the cloned repository.
The documentation will be generated in the _doc/_ directory.



Project Info
============

####Build Status
[![Build Status](https://travis-ci.org/JoErNanO/icub-tactilegrasp.svg?branch=master)](https://travis-ci.org/JoErNanO/icub-tactilegrasp)
