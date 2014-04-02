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



#include "iCub/tactileGrasp/GraspThread.h"

#include <iostream>
#include <algorithm>

#include <yarp/os/Property.h>

using std::cerr;
using std::cout;
using std::string;

using iCub::tactileGrasp::GraspThread;

using yarp::os::RateThread;
using yarp::os::Value;

#define TACTILEGRASP_DEBUG 1

/* *********************************************************************************************************************** */
/* ******* Constructor                                                      ********************************************** */   
GraspThread::GraspThread(const int aPeriod, const yarp::os::ResourceFinder &aRf) 
    : RateThread(aPeriod) {
        period = aPeriod;
        rf = aRf;

        nJoints = 0;

        dbgTag = "GraspThread: ";
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Destructor                                                       ********************************************** */   
GraspThread::~GraspThread() {}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Initialise thread                                                ********************************************** */
bool GraspThread::threadInit(void) {
    using yarp::os::Property;
    using yarp::os::Bottle;

    cout << dbgTag << "Initialising. \n";

    /* ******* Extract configuration files          ******* */
    string robotName = rf.check("robot", Value("icub"), "The robot name.").asString().c_str();
    string whichHand = rf.check("whichHand", Value("right"), "The hand to be used for the grasping.").asString().c_str();


    // Build grasp parameters
    Bottle &confGrasp = rf.findGroup("graspTh");
    if (!confGrasp.isNull()) {
        // Joints to be controlled
        Bottle *confJoints = confGrasp.find("joints").asList();
        // Individual touch thresholds per fingertip
        Bottle *confTouchThr = confGrasp.find("touchThresholds").asList();

        if (!(confJoints->isNull() || confTouchThr->isNull())) {
            // Get number of joints
            nJoints = confJoints->size();
            if (confTouchThr->size() == nJoints) {
                // Generate parameter vectors
                for (int i = 0; i < nJoints; ++i) {
                    graspJoints.push_back(confJoints->get(i).asInt());
                    touchThresholds.push_back(confTouchThr->get(i).asDouble());
                }
            } else {
                cerr << dbgTag << "One or more parameter lists contain either too few or too many parameters. \n";
                cerr << dbgTag << "Expecting lists of size equal to the number of parameters. \n";
                return false;
            }
        } else {
            cerr << dbgTag << "Could not find one or more configuration parameters in the specified configuration file under the [graspTh] parameter group. \n";
            cerr << dbgTag << "Expecting the following parameter lists: joints, touchThresholds. \n";
            return false;
        }
    } else {
        cerr << dbgTag << "Could not find grasp configuration [graspTh] group in the specified configuration file. \n";
        return false;
    }


#ifdef TACTILEGRASP_DEBUG
    cout << dbgTag << "Configured joints and thresholds: \t\t";
    for (int i = 0; i < nJoints; ++i) {
        cout << graspJoints[i] << " " << touchThresholds[i] << "\t";
    }
    cout << "\n";
#endif

    /* ******* Ports                                ******* */
    portGraspThreadInSkinComp.open("/TactileGrasp/skin/" + whichHand + "_hand_comp:i");
    portGraspThreadInSkinRaw.open("/TactileGrasp/skin/" + whichHand + "_hand_raw:i");
    portGraspThreadInSkinContacts.open("/TactileGrasp/skin/contacts:i");

    /* ******* Joint interfaces                     ******* */
    string arm = whichHand + "_arm";
    Property options;
    options.put("robot", robotName.c_str()); 
    options.put("device", "remote_controlboard");
//    options.put("writeStrict", "on");
    options.put("part", arm.c_str());
    options.put("local", ("/TactileGrasp/" + arm).c_str());
    options.put("remote", ("/" + robotName + "/" + arm).c_str());
    
    // Open driver
    if (!clientArm.open(options)) {
        return false;
    }
    // Open interfaces
    clientArm.view(iEncs);
    if (!iEncs) {
        return false;
    }
    clientArm.view(iPos);
    if (!iPos) {
        return false;
    }
    clientArm.view(iVel);
    if (!iVel) {
        return false;
    }
    clientArm.view(iVel2);
    if (!iVel2) {
        return false;
    }
    // Set velocity control parameters
    iVel->getAxes(&nJointsVel);
    std::vector<double> refAccels(nJointsVel, 10^6);
    iVel->setRefAccelerations(&refAccels[0]);
//    iVel2->setRefAccelerations(&refAccels);

    /* ******* Store position prior to acquiring control.           ******* */
    int nnJoints;
    iPos->getAxes(&nnJoints);
    startPos.resize(nnJoints);
    iEncs->getEncoders(startPos.data());

#if TACTILEGRASP_DEBUG
    for (int i = 0; i < startPos.size(); ++i) {
        cout << startPos[i] << " ";
    }
    cout << "\n";
#endif

    // Put arm in position
    reachArm();

    
    cout << dbgTag << "Initialised correctly. \n";
    
    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Run thread                                                       ********************************************** */
void GraspThread::run(void) {
    using std::deque;

//    vector<bool> contacts;
//    if (detectContact(contacts)) {
//        if (!moveFingers(contacts)) {
//            cout << dbgTag << "Could not perform the require grasp motion. \n";
//        }
//    } else {
//        cout << dbgTag << "foo \n";
//    }

//    static int counter = 0;
//    vector<double> graspVelocities(nJointsVel, 0);
//    
//    if (counter <= 5) {
//        for (size_t i = 0; i < 5; ++i) {
//            graspVelocities[11+i] = 50;
//        }
//        cout << dbgTag << "Moving. \n";
//    } else {
//        cout << dbgTag << "Stopping. \n";
//    }
//    counter++;


    using std::vector;
//    deque<bool> contacts;
    deque<bool> contacts (false, 5);
    vector<double> graspVelocities(nJointsVel, 0);
//    if (detectContact(contacts)) {
    if (true) {   
        for (size_t i = 0; i < 5; ++i) {
//        for (size_t i = 0; i < 1; ++i) {
            if (!contacts[i]) {
                graspVelocities[11+i] = velocities.grasp[i];
//                graspVelocities[3] = velocities.grasp[i];
//                graspVelocities[3+i] = velocities.grasp[i];
            } else {
                graspVelocities[11+i] = -velocities.grasp[i];
//                graspVelocities[11+i] = velocities.stop[i];
//                graspVelocities[3+i] = -velocities.grasp[i];
//                graspVelocities[3] = -velocities.grasp[i];
//                graspVelocities[3] = -velocities.stop[i];
            }
        }
    } else {
        cout << dbgTag << "No contact. \n";
    }

    cout << dbgTag << "Moving joints at velocities: \t";
    for (size_t i = 11; i < graspVelocities.size(); ++i) {
//    for (size_t i = 3; i < 4; ++i) {
        cout << i << " " << graspVelocities[i] << "\t";
    }
    cout << "\n";

    iVel->velocityMove(&graspVelocities[0]);
}  
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Release thread                                                   ********************************************** */
void GraspThread::threadRelease(void) {
    cout << dbgTag << "Releasing. \n";
    
    // Close ports
    portGraspThreadInSkinComp.interrupt();
    portGraspThreadInSkinRaw.interrupt();
    portGraspThreadInSkinContacts.interrupt();
    portGraspThreadInSkinComp.close();
    portGraspThreadInSkinRaw.close();
    portGraspThreadInSkinContacts.close();

    // Stop interfaces
    if (iVel) {
        iVel->stop();
    }
    if (iVel2) {
        iVel2->stop();
    }
    if (iPos) {
        iPos->stop();
        // Restore initial robot position
        //iPos->positionMove(startPos.data());
    }

    // Close driver
    clientArm.close();

    cout << dbgTag << "Released. \n";
}
/* *********************************************************************************************************************** */

/* *********************************************************************************************************************** */
/* ******* Detect contact on each finger.                                   ********************************************** */
bool GraspThread::detectContact(std::deque<bool> &o_contacts) {
    using yarp::sig::Vector;
    using std::deque;
    using std::vector;

    Vector *inComp = portGraspThreadInSkinComp.read(false);
    if (inComp) {
        // Convert yarp vector to stl vector
        vector<double> contacts(12*nJoints);
        for (size_t i = 0; i < contacts.size(); ++i) {
            contacts[i] = (*inComp)[i];
        }

        // Find maximum for each finger
        vector<double> maxContacts(nJoints);
        vector<double>::iterator start;
        vector<double>::iterator end;
        for (int i = 0; i < nJoints; ++i) {
            start = contacts.begin() + 12*i;
            end = start + 11;
            maxContacts[i] = *std::max_element(start, end);
        }

#if TACTILEGRASP_DEBUG
        cout << dbgTag << "Maximum contact detected: \t\t";
        for (size_t i = 0; i < maxContacts.size(); ++i) {
            cout << maxContacts[i] << " ";
        }
        cout << "\n";
#endif

        // Check if contact is greater than threshold
        o_contacts.resize(nJoints, false);
        for (size_t i = 0; i < maxContacts.size(); ++i) {
            o_contacts[i] = (maxContacts[i] >= touchThresholds[i]);
        }
    } else {
#if TACTILEGRASP_DEBUG
        cout << dbgTag << "No skin data. \n";
#endif
        return false;
    }

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Move fingers to perform grasp movement                           ********************************************** */
bool GraspThread::moveFingers(const std::deque<bool> &i_contacts) {
    using std::deque;
    using std::vector;

//    vector<double> graspVelocities(i_contacts.size(), velocities.grasp);
//    vector<double> graspVelocities(velocities.grasp);
//    for (size_t i = 0; i < i_contacts.size(); ++i) {
//        if (i_contacts[i]) {
//            graspVelocities[i] = velocities.stop[i];
//        }
//    }
//
//#ifdef TACTILEGRASP_DEBUG
//    cout << dbgTag << "Moving joints at speeds: \t\t";
//    for (int i = 0; i < i_contacts.size(); ++i) {
//        cout << graspJoints[i] << " " << graspVelocities[i] << "\t";
//    }
//    cout << "\n";
//#endif
//
//    double vels[5];
//    int join[5];
//    std::copy(graspVelocities.begin(), graspVelocities.end(), vels);
//    std::copy(graspJoints.begin(), graspJoints.end(), vels);

//    return iVel2->velocityMove(graspJoints.size(), &graspJoints[0], &graspVelocities[0]);
//    return iVel2->velocityMove(graspJoints.size(), join, vels);
    
    vector<double> graspVelocities(nJointsVel, 0);
    for (size_t i = 0; i < i_contacts.size(); ++i) {
        if (i_contacts[i]) {
            graspVelocities[11+i] = velocities.stop[i];
        } else {
            graspVelocities[11+i] = velocities.grasp[i];
        }
    }
    
#ifdef TACTILEGRASP_DEBUG
    cout << dbgTag << "Moving joints at speeds: \t\t";
    for (size_t i = 11; i < graspVelocities.size(); ++i) {
        cout << i << " " << graspVelocities[i] << "\t";
    }
    cout << "\n";
#endif

    return iVel->velocityMove(&graspVelocities[0]);
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Set touch threshold.                                             ********************************************** */
bool GraspThread::setTouchThreshold(const int aFinger, const double aThreshold) {
    if ((aFinger > 0) && (aFinger < nJoints)) {
        touchThresholds[aFinger] = aThreshold;
        return true;
    } else {
        cerr << dbgTag << "RPC::setTouchThreshold() - The specified finger is out of range. \n";
        return false;
    }
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Open hand                                                        ********************************************** */
bool GraspThread::openHand(void) {
    iVel->stop();
    iVel2->stop();

    // set the fingers to the original position
    iPos->positionMove(11, 5);
    iPos->positionMove(12, 35);
    iPos->positionMove(13, 15);
    iPos->positionMove(14, 20);
    iPos->positionMove(15, 40);
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Place arm in grasping position                                   ********************************************** */ 
bool GraspThread::reachArm(void) {
    iVel->stop();
    iVel2->stop();

    // set the arm in the starting position
    iPos->positionMove(0 ,-25);
    iPos->positionMove(1 , 35);
    iPos->positionMove(2 , 18);
    iPos->positionMove(3 , 86);
    iPos->positionMove(4 ,-32);
    iPos->positionMove(5 , 9);
    iPos->positionMove(6 , 11);
    iPos->positionMove(7 , 40);
    iPos->positionMove(8 , 60);
    iPos->positionMove(9 , 30);
    iPos->positionMove(10, 30);
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Set the given velocity                                           ********************************************** */
bool GraspThread::setVelocity(const int &i_type, const std::vector<double> &i_vel) {
    switch (i_type) {
        case GraspType::Stop :
            velocities.stop = i_vel;
            break;
        case GraspType::Grasp :
            velocities.grasp = i_vel;
            break;

        default:
            cerr << dbgTag << "Unknown velocity type specified. \n";
            return false;
            break;
    }

    return true;
}
/* *********************************************************************************************************************** */


/* *********************************************************************************************************************** */
/* ******* Set the velocity for the given joint.                            ********************************************** */
bool GraspThread::setVelocity(const int &i_type, const int &i_joint, const double &i_vel) {
    if ((i_joint > 0) && (i_joint < velocities.stop.size())) {
        switch (i_type) {
            case GraspType::Stop :
                velocities.stop[i_joint] = i_vel;
                break;
            case GraspType::Grasp :
                velocities.grasp[i_joint] = i_vel;
                break;

            default:
                cerr << dbgTag << "Unknown velocity type specified. \n";
                return false;
                break;
        }
    } else {
        cerr << dbgTag << "Invalid joint specified. \n";
        return false;
    }

    return true;
}
/* *********************************************************************************************************************** */
