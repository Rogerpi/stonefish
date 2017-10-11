//
//  Thruster.cpp
//  Stonefish
//
//  Created by Patryk Cieslak on 10/10/2017.
//  Copyright (c) 2017 Patryk Cieslak. All rights reserved.
//

#include "Thruster.h"
#include "SimulationApp.h"
#include "OpenGLContent.h"
#include "MathsUtil.hpp"

Thruster::Thruster(std::string uniqueName, SolidEntity* propeller, btScalar diameter, btScalar thrustCoeff, btScalar torqueCoeff, btScalar omegaMax) : Actuator(uniqueName)
{
    D = UnitSystem::SetLength(diameter);
    kT = thrustCoeff;
    kQ = torqueCoeff;
    kp = btScalar(10.0);
    ki = btScalar(1.0);
    iLim = btScalar(1.0);
    omegaLim = omegaMax;
    
    theta = btScalar(0);
    omega = btScalar(0);
    thrust = btScalar(0);
    torque = btScalar(0);
    setpoint = btScalar(0);
    iError = btScalar(0);
    
    prop = propeller;
    prop->BuildGraphicalObject();
    attach = NULL;
    attachFE = NULL;
    linkId = 0;
    pos.setIdentity();
}

Thruster::~Thruster()
{
    delete prop;
}

ActuatorType Thruster::getType()
{
    return ACTUATOR_THRUSTER;
}


void Thruster::setSetpoint(btScalar value)
{
    setpoint = value < btScalar(-1) ? btScalar(-1) : (value > btScalar(1) ? btScalar(1) : value);
}

btScalar Thruster::getSetpoint()
{
    return setpoint;
}

btScalar Thruster::getOmega()
{
    return omega;
}

btScalar Thruster::getThrust()
{
    return thrust;
}

void Thruster::AttachToSolid(SolidEntity* solid, const btTransform& position)
{
    attach = solid;
    pos = position;
}

void Thruster::AttachToSolid(FeatherstoneEntity* fe, unsigned int link, const btTransform& position)
{
    attachFE = fe;
    linkId = link;
    pos = position;
}

std::vector<Renderable> Thruster::Render()
{
    std::vector<Renderable> items(0);
    btTransform thrustTrans = btTransform::getIdentity();
    
    if(attach != NULL)
    {
        thrustTrans = attach->getTransform() * attach->getLocalTransform().inverse() * pos;
    }
    else if(attachFE != NULL)
    {
        FeatherstoneLink link = attachFE->getLink(linkId);
        thrustTrans = link.solid->getTransform() * link.solid->getLocalTransform().inverse() * pos;
    }
    else
    {
        return items;
    }
    
    //Rotate propeller
    thrustTrans *= btTransform(btQuaternion(0, 0, theta), btVector3(0,0,0));
    
    //Add renderable
    Renderable item;
    item.objectId = prop->getObject();
    item.lookId = prop->getLook();
	item.dispCoordSys = false;
	item.model = glMatrixFromBtTransform(thrustTrans);
    item.csModel = item.model;
	items.push_back(item);
    return items;
}

void Thruster::Update(btScalar dt)
{
    //Update thruster velocity
    btScalar error = setpoint*omegaLim - omega;
    btScalar epsilon = error*kp + iError*ki;
    iError += error*dt;
    iError = iError > iLim ? iLim : iError;
    
    omega += epsilon*dt;
    theta += omega*dt;
        
    //Calculate thrust
    Ocean* ocean = SimulationApp::getApp()->getSimulationManager()->getOcean();
    btScalar omega1overS = omega/(2.0*M_PI); 
    thrust = ocean->getFluid()->density * kT * btFabs(omega1overS)*omega1overS * D*D*D*D;
    torque = ocean->getFluid()->density * kQ * btFabs(omega1overS)*omega1overS * D*D*D*D*D;
    btVector3 thrustV(thrust, 0, 0);
    btVector3 torqueV(torque, 0, 0);
    
    //Apply forces and torques
    if(attach != NULL)
    {
        btTransform solidTrans = attach->getTransform() * attach->getLocalTransform().inverse();
        btTransform thrustTrans = solidTrans * pos;
        
        attach->ApplyCentralForce(thrustTrans.getBasis() * thrustV);
        attach->ApplyTorque((thrustTrans.getOrigin() - solidTrans.getOrigin()).cross(thrustTrans.getBasis() * thrustV));
        attach->ApplyTorque(thrustTrans.getBasis() * torqueV);
    }
    else if(attachFE != NULL)
    {
        FeatherstoneLink link = attachFE->getLink(linkId);
        btTransform linkTrans = link.solid->getTransform() * link.solid->getLocalTransform().inverse();
        btTransform thrustTrans = linkTrans * pos;
        
        attachFE->AddLinkForce(linkId, thrustTrans.getBasis() * thrustV);
        attachFE->AddLinkTorque(linkId, (thrustTrans.getOrigin() - linkTrans.getOrigin()).cross(thrustTrans.getBasis() * thrustV));
        attachFE->AddLinkTorque(linkId, thrustTrans.getBasis() * torqueV);
    }
}