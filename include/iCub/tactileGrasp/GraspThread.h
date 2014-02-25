
/* 
 * Copyright (C) 2009 RobotCub Consortium, European Commission FP6 Project IST-004370
 * Authors: Andrea Del Prete, Alexander Schmitz
 * email:   andrea.delprete@iit.it, alexander.schmitz@iit.it
 * website: www.robotcub.org 
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * http://www.robotcub.org/icub/license/gpl.txt
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
 */


#ifndef __ICUB_TACTILEGRASP_GRASPTHREAD_H__
#define __ICUB_TACTILEGRASP_GRASPTHREAD_H__

#include <iostream>
#include <string>
#include <vector>

#include <yarp/sig/Vector.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Thread.h>
#include <yarp/os/Semaphore.h>
#include <yarp/os/ResourceFinder.h>
#include <yarp/dev/PolyDriver.h> 
#include <yarp/dev/ControlBoardInterfaces.h> 


namespace iCub {
    namespace tactileGrasp {
        class GraspThread : public yarp::os::Thread {
        public:
            typedef enum { control, tough, soft, compliant, openHand, doNothing} GraspCommand;

            /* class methods */
            GraspThread(yarp::os::ResourceFinder* rf, std::string robotName, std::string moduleName, bool rightHand);
            
            bool setGraspCommand(GraspCommand command);
            bool setTouchThreshold(int newTouchThreshold);		// set the touch threshold used in the tough, soft and compliant grasps
            bool setPercentile(std::vector<float> newPercentile);
            bool setKP(float newKP);
            bool setKI(float newKI);
            bool setKD(float newKD);
            bool setKP(unsigned int index, float newKP);
            bool setKI(unsigned int index, float newKI);
            bool setKD(unsigned int index, float newKD);
            bool setTouchDesired(float newTouchDesired);

            bool threadInit();     
            void threadRelease();
            void run(); 

        private:

            struct range{
                    float min;
                    float max;
            };

            /* class constants */	
            static const int MAX_SKIN = 255;			// max value you can read from the skin sensors
            static const int NUM_FINGERS = 4;			// number of fingers used
            static const int SKIN_DIM = 48;				// number of taxels in one hand (actually they are 192, 
                                                                                                    // but only the index, middle, ring and pinky fingertips are used here)
            
            static const int REF_SPEED_ARM = 10;		// reference speed for the arm joints from 0 to 6
            static const int REF_SPEED_FINGER = 100;	// reference speed for the finger joints from 7 to 15
            static const int REF_ACC_ARM = 10;			// reference accelleration for the arm joints from 0 to 6
            static const int REF_ACC_FINGER = 10000;		// reference accelleration for the finger joints from 7 to 15	
            
            static const int MOTOR_MIN[];				// min values of the motor encoders
            static const int MOTOR_MAX[];				// max values of the motor encoders
            static const int VELOCITY_MAX = 70;			// max velocity used for moving the fingers
            static const int VELOCITY_MIN = -70;		// min velocity used for moving the fingers
            
            static const int MAX_KP = 100;				// max value used for the proportional factor in the controller
            static const int MAX_KI = 100;				// max value used for the integrative factor in the controller
            static const int MAX_KD = 100;				// max value used for the derivative factor in the controller

            static const int VELOCITY_TOUGH = 50;		// velocity used when touch is detected during the tough grasp
            static const int VELOCITY_SOFT = 0;			// velocity used when touch is detected during the soft grasp
            static const int VELOCITY_COMPLIANT = -50;	// velocity used when touch is detected during the compliant grasp

            /* class variables */

            // CONTROLLER VARIABLES
            std::vector<float> K_P;							// proportional coefficients (one per finger)
            std::vector<float> K_I;							// integrative coefficients (one per finger)
            std::vector<float> K_D;							// derivative coefficients (one per finger)
            float touch_desired;						// the desired touch value
            std::vector<float> touchError;					// the current touch error (one per finger)
            std::vector<float> touchErrorOld;				// the previous touch error, used to compute the derivative (one per finger)
            std::vector<float> touchErrorSum;				// the sum of the touch errors up to now, i.e. the integral (one per finger)
            std::vector<float> velocities;                   // velocities commanded to the fingers (i.e. output of the controller)
            yarp::os::Semaphore controllerSem;					// semaphore controlling the access to K_P, K_I, K_D and touch_desired
            
            yarp::os::Semaphore touchThresholdSem;
            int touch_threshold;						// if the compensated touch detected is greater than this, 
                                                                                                    // then a contact has happened (default value 1)	

            yarp::os::Semaphore percentileSem;					// semaphore for controlling the access to the percentile
            std::vector<float> percentile;					// 95 percentile of each taxel (minus taxel baseline)
                                    
            std::vector<float> touchPerFinger;				// max touch value of each finger
            std::vector<range> touchBandPerFinger;			// range in which the touch value is, one for each finger
            double speedNoTouch;						// velocity used when no touch is detected
            double speedTouch;							// velocity used when touch is detected
            bool doControl;
                    
           
            // Device attributes 
            yarp::dev::PolyDriver* robotDevice;
            yarp::dev::IPositionControl *pos;
            yarp::dev::IVelocityControl *vel;
            yarp::dev::IEncoders *encs;
            int NUM_JOINTS;								// number of joints of the arm
            yarp::sig::Vector encoders;
            yarp::sig::Vector startPos;

            yarp::os::ResourceFinder* rf;

            GraspCommand graspComm;			// tough grasp, soft grasp, compliant grasp, open hand, do nothing
            yarp::os::Semaphore graspCommSem;

            // input parameters
            std::string robotName;
            std::string moduleName;
            bool rightHand;					// if true then use the right hand, otherwise use the left hand

            /* ports */
            yarp::os::BufferedPort<yarp::sig::Vector> compensatedTactileDataPort;	// input port for compensated tactile data
            yarp::os::BufferedPort<yarp::sig::Vector> monitorPort;                   // output port for monitoring the module behaviour

            /**
             * Debug string used to prefix output/log/error messages.
             */
            std::string dbgTag;

            /* class private methods */
            void grasp();
            void controller();
            void safeVelocityMove(int encIndex, double speed);
            void setArmJointsPos();
            void sendMonitoringData();
        };
    } //namespace tactileGrasp
} //namespace iCub
#endif
