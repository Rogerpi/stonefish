//
//  RotaryEncoder.h
//  Stonefish
//
//  Created by Patryk Cieslak on 29/03/2014.
//  Copyright (c) 2014 Patryk Cieslak. All rights reserved.
//

#ifndef __Stonefish_RotaryEncoder__
#define __Stonefish_RotaryEncoder__

#include "Sensor.h"
#include "RevoluteJoint.h"

class RotaryEncoder : public Sensor
{
public:
    RotaryEncoder(std::string uniqueName, RevoluteJoint* joint, unsigned int cpr_resolution, bool absolute = false, btScalar frequency = btScalar(-1.), unsigned int historyLength = 0);
    
    void InternalUpdate(btScalar dt);
    void Reset();
    unsigned short getNumOfDimensions();
    
private:
    //params
    RevoluteJoint* revolute;
    unsigned int cpr_res;
    unsigned int abs;
    
    //temporary
    btScalar angle;
    btScalar lastAngle;
};

#endif