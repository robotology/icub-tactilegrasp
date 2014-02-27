
/* 
 * Copyright (C) 2014 Francesco Giovannini, iCub Facility - Istituto Italiano di Tecnologia
 * Authors: Francesco Giovannini
 * email:   francesco.giovannini@iit.it
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

#include <string>

#include <yarp/os/RateThread.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/dev/PolyDriver.h>
#include <yarp/dev/IEncoders.h>
#include <yarp/dev/IPositionControl.h>
#include <yarp/dev/IVelocityControl2.h>
#include <yarp/sig/Vector.h>

#include <iCub/skinDynLib/skinContactList.h>

namespace iCub {
    namespace tactileGrasp {
        class GraspThread : public yarp::os::RateThread {
            private:
                /* ****** Module attributes                             ****** */
                int period;
                std::string robotName;
                std::string whichHand;


                /* ******* Controllers                                  ******* */
                yarp::dev::PolyDriver clientArm;
                yarp::dev::IEncoders *iEncs;
                yarp::dev::IPositionControl *iPos;
                yarp::dev::IVelocityControl2 *iVel;

                /** Robot arm start position. */
                yarp::sig::Vector startPos;


                /* ******* Grasp configuration                          ******* */
                static const int NUM_JOINTS = 5;
                double VELOCITY_CRUSH;
                std::vector<double> touchThresholds;
                std::vector<int> graspJoints;

                
                /* ****** Ports                                         ****** */
                yarp::os::BufferedPort<yarp::sig::Vector> portGraspThreadInSkinComp;
                yarp::os::BufferedPort<yarp::sig::Vector> portGraspThreadInSkinRaw;
                yarp::os::BufferedPort<iCub::skinDynLib::skinContactList> portGraspThreadInSkinContacts;
                

                /* ****** Debug attributes                              ****** */
                std::string dbgTag;

            public:
                GraspThread(const int aPeriod, const std::string &aRobotName, const std::string &aWhichHand);
                virtual ~GraspThread();

                virtual bool threadInit(void);
                virtual void run(void);
                virtual void threadRelease(void);

                bool setTouchThreshold(const int aFinger, const double aThreshold);
                bool openHand(void); 

            private:
                bool detectContact(std::vector<bool> &o_contacts);
                bool detectContact(iCub::skinDynLib::skinContactList &o_contacts);

                bool moveFingers(const std::vector<bool> &i_contacts);

                bool reachArm(void);
        };
    }
}

#endif

