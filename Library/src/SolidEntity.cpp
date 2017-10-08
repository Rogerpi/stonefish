//
//  SolidEntity.cpp
//  Stonefish
//
//  Created by Patryk Cieslak on 12/29/12.
//  Copyright (c) 2012 Patryk Cieslak. All rights reserved.
//

#include "SolidEntity.h"
#include "MathsUtil.hpp"
#include "Console.h"
#include "Ocean.h"

SolidEntity::SolidEntity(std::string uniqueName, Material m, int _lookId) : Entity(uniqueName)
{
	mat = m;
    mass = btScalar(0);
	Ipri = btVector3(0,0,0);
    volume = btScalar(0);
	thickness = btScalar(0);
	//aMass = btVector3(0,0,0);
    //dragCoeff = btVector3(0,0,0);
    CoB = btVector3(0,0,0);
    localTransform = btTransform::getIdentity(); //CoG = (0,0,0)
	computeHydro = true;
    	
    linearAcc = btVector3(0,0,0);
    angularAcc = btVector3(0,0,0);
    
    rigidBody = NULL;
    multibodyCollider = NULL;
    
	mesh = NULL;
	lookId = _lookId;
	objectId = -1;
	
    dispCoordSys = false;
}

SolidEntity::~SolidEntity()
{
    if(mesh != NULL)
	{
		delete mesh;
		mesh = NULL;
	}
	
    rigidBody = NULL;
    multibodyCollider = NULL;
}

EntityType SolidEntity::getType()
{
    return ENTITY_SOLID;
}

void SolidEntity::SetHydrodynamicProperties(const eigMatrix6x6& addedMass, const eigMatrix6x6& damping, const btTransform& cobTransform)
{
}

void SolidEntity::SetArbitraryPhysicalProperties(btScalar mass, const btVector3& inertia, const btTransform& cogTransform)
{
    if(rigidBody != NULL)
    {
        this->mass = UnitSystem::SetMass(mass);
        Ipri = UnitSystem::SetInertia(inertia);
        rigidBody->setMassProps(this->mass, Ipri);
        
        btTransform oldLocalTransform = localTransform;
        localTransform = UnitSystem::SetTransform(cogTransform);
        btCompoundShape* colShape = (btCompoundShape*)rigidBody->getCollisionShape();
        rigidBody->setCenterOfMassTransform(oldLocalTransform.inverse() * localTransform * rigidBody->getCenterOfMassTransform());
        colShape->updateChildTransform(0, localTransform.inverse());
    }
    else if(rigidBody == NULL && multibodyCollider == NULL)
    {
        this->mass = UnitSystem::SetMass(mass);
        Ipri = UnitSystem::SetInertia(inertia);
        localTransform = UnitSystem::SetTransform(cogTransform);
    }
}

void SolidEntity::setComputeHydrodynamics(bool flag)
{
	computeHydro = flag;
}

void SolidEntity::SetLook(int newLookId)
{
    lookId = newLookId;
}

void SolidEntity::setDisplayCoordSys(bool enabled)
{
    dispCoordSys = enabled;
}

bool SolidEntity::isCoordSysVisible()
{
    return dispCoordSys;
}

int SolidEntity::getLook()
{
    return lookId;
}

int SolidEntity::getObject()
{
	return objectId;
}

btRigidBody* SolidEntity::getRigidBody()
{
    return rigidBody;
}

btMultiBodyLinkCollider* SolidEntity::getMultibodyLinkCollider()
{
    return multibodyCollider;
}

void SolidEntity::GetAABB(btVector3& min, btVector3& max)
{
    rigidBody->getAabb(min, max);
}

std::vector<Renderable> SolidEntity::Render()
{
	std::vector<Renderable> items(0);
	
	if((rigidBody != NULL || multibodyCollider != NULL) && objectId >= 0 && isRenderable())
	{
		btTransform oTrans =  getTransform() * localTransform.inverse();
		
		Renderable item;
		item.objectId = objectId;
		item.lookId = lookId;
		item.dispCoordSys = dispCoordSys;
		item.model = glMatrixFromBtTransform(oTrans);
		item.csModel = glMatrixFromBtTransform(getTransform());
        items.push_back(item);
	}
	
	return items;
}

btTransform SolidEntity::getTransform() const
{
    if(rigidBody != NULL)
    {
        btTransform trans;
        rigidBody->getMotionState()->getWorldTransform(trans);
        return trans;
    }
    else if(multibodyCollider != NULL)
    {
        return multibodyCollider->getWorldTransform();
    }
    else
        return btTransform::getIdentity();
}

btVector3 SolidEntity::getLinearVelocity()
{
    if(rigidBody != NULL)
    {
        return rigidBody->getLinearVelocity();
    }
    else if(multibodyCollider != NULL)
    {
        //Get multibody and link id
        btMultiBody* multiBody = multibodyCollider->m_multiBody;
        int index = multibodyCollider->m_link;
        
        //Start with base velocity
        btVector3 linVelocity = multiBody->getBaseVel(); //Global
        
        if(index >= 0) //If collider is not base
        {
            for(unsigned int i = 0; i <= index; i++) //Accumulate velocity resulting from joints
            {
                if(multiBody->getLink(i).m_jointType == btMultibodyLink::ePrismatic) //Just add linear velocity
                {
                    btVector3 axis = multiBody->getLink(i).getAxisBottom(0); //Local axis
                    btVector3 vel = multiBody->getJointVel(i) * axis; //Local velocity
                    btVector3 gvel = multiBody->localDirToWorld(i, vel); //Global velocity
                    linVelocity += gvel;
                }
                else if(multiBody->getLink(i).m_jointType == btMultibodyLink::eRevolute) //Add linear velocity due to rotation
                {
                    btVector3 axis = multiBody->getLink(i).getAxisBottom(0); //Local linear motion
                    btVector3 vel = multiBody->getJointVel(i) * axis; //Local velocity
                    btVector3 gvel = multiBody->localDirToWorld(i, vel); //Global velocity
                    linVelocity += gvel;
                }
            }
        }
        
        return linVelocity;
    }
    else
        return btVector3(0.,0.,0.);
}

btVector3 SolidEntity::getAngularVelocity()
{
    if(rigidBody != NULL)
    {
        return rigidBody->getAngularVelocity();
    }
    else if(multibodyCollider != NULL)
    {
        //Get multibody and link id
        btMultiBody* multiBody = multibodyCollider->m_multiBody;
        int index = multibodyCollider->m_link;
        
        //Start with base velocity
        btVector3 angVelocity = multiBody->getBaseOmega(); //Global
        
        if(index >= 0)
        {
            for(unsigned int i = 0; i <= index; i++) //Accumulate velocity resulting from joints
                if(multiBody->getLink(i).m_jointType == btMultibodyLink::eRevolute) //Only revolute joints can change angular velocity
                {
                    btVector3 axis = multiBody->getLink(i).getAxisTop(0); //Local axis
                    btVector3 vel = multiBody->getJointVel(i) * axis; //Local velocity
                    btVector3 gvel = multiBody->localDirToWorld(i, vel); //Global velocity
                    angVelocity += gvel;
                }
        }
        
        return angVelocity;
    }
    else
        return btVector3(0.,0.,0.);
}


btVector3 SolidEntity::getLinearVelocityInLocalPoint(const btVector3& relPos)
{
    if(rigidBody != NULL)
    {
        return rigidBody->getVelocityInLocalPoint(relPos);
    }
    else if(multibodyCollider != NULL)
    {
        return getLinearVelocity() + getAngularVelocity().cross(relPos);
    }
    else
        return btVector3(0.,0.,0.);
}

btVector3 SolidEntity::getLinearAcceleration()
{
    return linearAcc;
}

btVector3 SolidEntity::getAngularAcceleration()
{
    return angularAcc;
}

btScalar SolidEntity::getVolume()
{
    return volume;
}

btTransform SolidEntity::getLocalTransform()
{
    return localTransform;
}

btVector3 SolidEntity::getMomentsOfInertia()
{
    return Ipri;
}

btScalar SolidEntity::getMass()
{
    return mass;
}

Material SolidEntity::getMaterial()
{
    return mat;
}

void SolidEntity::BuildGraphicalObject()
{
	if(mesh == NULL)
		return;
		
	objectId = OpenGLContent::getInstance()->BuildObject(mesh);	
}

void SolidEntity::ComputeEquivEllipsoid()
{
	if(mesh == NULL)
		return;
	
	if(mesh->vertices.size() < 9) //Need at least 9 vertices to fit unique ellipsoid in 3D
		return;
	
	//Fill points matrix
	eigMatrix P(mesh->vertices.size(), 3);
	for(unsigned int i=0; i<mesh->vertices.size(); ++i)
		P.row(i) << mesh->vertices[i].pos.x, mesh->vertices[i].pos.y, mesh->vertices[i].pos.z;
	
	//Compute contraints
	eigMatrix A(mesh->vertices.size(), 9);
	A.col(0) = P.col(0).array() * P.col(0).array() + P.col(1).array() * P.col(1).array() - 2 * P.col(2).array() * P.col(2).array();
	A.col(1) = P.col(0).array() * P.col(0).array() + P.col(2).array() * P.col(2).array() - 2 * P.col(1).array() * P.col(1).array();
	A.col(2) = 2 * P.col(0).array() * P.col(1).array();
	A.col(3) = 2 * P.col(0).array() * P.col(2).array();
	A.col(4) = 2 * P.col(1).array() * P.col(2).array();
	A.col(5) = 2 * P.col(0);
	A.col(6) = 2 * P.col(1);
	A.col(7) = 2 * P.col(2);
	A.col(8) = eigMatrix::Ones(mesh->vertices.size(), 1);
	
	//Solve Least-Squares problem Ax=b
	eigMatrix b(mesh->vertices.size(), 1);
	eigMatrix x(9, 1);	
	//squared norm
	b = P.col(0).array() * P.col(0).array() + P.col(1).array() * P.col(1).array() + P.col(2).array() * P.col(2).array();
	//solution
	//x = (A.transpose() * A).ldlt().solve(A.transpose() * b); //normal equations
	x = A.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(b); 
	
	//Find ellipsoid parameters
	eigMatrix p(10, 1);
	p(0) = x(0) + x(1) - 1;
	p(1) = x(0) - 2 * x(1) - 1;
	p(2) = x(1) - 2 * x(0) - 1;
	p(3) = x(2);
	p(4) = x(3);
	p(5) = x(4);
	p(6) = x(5);
	p(7) = x(6);
	p(8) = x(7);
	p(9) = x(8);
	
	eigMatrix E(4, 4);
	E << p(0), p(3), p(4), p(6),
		 p(3), p(1), p(5), p(7),
		 p(4), p(5), p(2), p(8),
		 p(6), p(7), p(8), p(9);
		 
	//Compute center
	eigMatrix c(3, 1);
	c = -E.block(0, 0, 3, 3).jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(p.block(6, 0, 3, 1));
	std::cout << "Center: " << c << std::endl;
	
	//Compute transform matrix
	eigMatrix T(4, 4);
	T.setIdentity();
	T.block(3, 0, 1, 3) = c.transpose();
	T = T * E * T.transpose();
	
	//Compute axes
	Eigen::SelfAdjointEigenSolver<eigMatrix> eigenSolver(T.block(0, 0, 3, 3)/(-T(3,3)));
	if(eigenSolver.info() != Eigen::Success) 
	{
		cError("Error computing ellipsoid for %s!", getName().c_str());
		return;
	}
	
	eigMatrix r(3, 1);
	r = Eigen::sqrt(1/Eigen::abs(eigenSolver.eigenvalues().array()));
	std::cout << "Radii: " << r << std::endl;
	
	eigMatrix axes(3, 3);
	axes = eigenSolver.eigenvectors().array();
	std::cout << "Axes: " << axes << std::endl;
}

void SolidEntity::BuildRigidBody()
{
    if(rigidBody == NULL)
    {
        btDefaultMotionState* motionState = new btDefaultMotionState();
        
        btCompoundShape* colShape = new btCompoundShape();
        colShape->addChildShape(localTransform.inverse(), BuildCollisionShape());
        colShape->setMargin(UnitSystem::Length(UnitSystems::MKS, UnitSystem::GetInternalUnitSystem(), 0.001));
        
        btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(mass, motionState, colShape, Ipri);
        rigidBodyCI.m_friction = rigidBodyCI.m_rollingFriction = rigidBodyCI.m_restitution = btScalar(0.); //not used
        rigidBodyCI.m_linearDamping = rigidBodyCI.m_angularDamping = btScalar(0.); //not used
		rigidBodyCI.m_linearSleepingThreshold = rigidBodyCI.m_angularSleepingThreshold = btScalar(0.); //not used
        rigidBodyCI.m_additionalDamping = false;
        
        rigidBody = new btRigidBody(rigidBodyCI);
        rigidBody->setUserPointer(this);
        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
        rigidBody->setFlags(rigidBody->getFlags() | BT_ENABLE_GYROSCOPIC_FORCE_IMPLICIT_BODY);
        //rigidBody->setActivationState(DISABLE_DEACTIVATION);
        //rigidBody->setCcdMotionThreshold(0.01);
        //rigidBody->setCcdSweptSphereRadius(0.9);
    }
}

void SolidEntity::BuildMultibodyLinkCollider(btMultiBody *mb, unsigned int child, btMultiBodyDynamicsWorld *world)
{
    if(multibodyCollider == NULL)
    {
        //Shape with offset
        btCompoundShape* colShape = new btCompoundShape();
        colShape->addChildShape(localTransform.inverse(), BuildCollisionShape());
        colShape->setMargin(UnitSystem::Length(UnitSystems::MKS, UnitSystem::GetInternalUnitSystem(), 0.001));
        
        //Link
        multibodyCollider = new btMultiBodyLinkCollider(mb, child - 1);
        multibodyCollider->setUserPointer(this);
        multibodyCollider->setCollisionShape(colShape);
        multibodyCollider->setFriction(btScalar(0.));
        multibodyCollider->setRestitution(btScalar(0.));
        multibodyCollider->setRollingFriction(btScalar(0.));
        multibodyCollider->setCollisionFlags(multibodyCollider->getCollisionFlags() | btCollisionObject::CF_CUSTOM_MATERIAL_CALLBACK);
		multibodyCollider->setActivationState(DISABLE_DEACTIVATION);
        
        if(child > 0)
            mb->getLink(child - 1).m_collider = multibodyCollider;
        else
            mb->setBaseCollider(multibodyCollider);
        
        world->addCollisionObject(multibodyCollider, MASK_DEFAULT, MASK_STATIC | MASK_DEFAULT);
        
        //Graphics
        BuildGraphicalObject();
    }
}

void SolidEntity::AddToDynamicsWorld(btMultiBodyDynamicsWorld* world)
{
    AddToDynamicsWorld(world, btTransform::getIdentity());
}

void SolidEntity::AddToDynamicsWorld(btMultiBodyDynamicsWorld* world, const btTransform& worldTransform)
{
    if(rigidBody == NULL)
    {
        BuildRigidBody();
		BuildGraphicalObject();
        
        //rigidBody->setMotionState(new btDefaultMotionState(UnitSystem::SetTransform(worldTransform)));
        rigidBody->setCenterOfMassTransform(UnitSystem::SetTransform(worldTransform) * localTransform);
        world->addRigidBody(rigidBody, MASK_DEFAULT, MASK_STATIC | MASK_DEFAULT);
    }
}

void SolidEntity::RemoveFromDynamicsWorld(btMultiBodyDynamicsWorld* world)
{
    if(rigidBody != NULL)
    {
        world->removeRigidBody(rigidBody);
        delete rigidBody;
        rigidBody = NULL;
    }
}

void SolidEntity::UpdateAcceleration()
{
    if(rigidBody != NULL)
    {
        linearAcc = rigidBody->getTotalForce()/mass;
        btVector3 torque = rigidBody->getTotalTorque();
        angularAcc = btVector3(torque.x()/Ipri.x(), torque.y()/Ipri.y(), torque.z()/Ipri.z());
    }
}

void SolidEntity::ApplyGravity()
{
    if(rigidBody != NULL)
    {
        rigidBody->applyGravity();
    }
}

void SolidEntity::ApplyCentralForce(const btVector3& force)
{
    if(rigidBody != NULL)
        rigidBody->applyCentralForce(force);
    else if(multibodyCollider != NULL)
    {
        btMultiBody* multiBody = multibodyCollider->m_multiBody;
        int index = multibodyCollider->m_link;
        
        if(index == -1) //base
            multiBody->addBaseForce(force);
        else
            multiBody->addLinkForce(index, force);
    }
}

void SolidEntity::ApplyTorque(const btVector3& torque)
{
    if(rigidBody != NULL)
        rigidBody->applyTorque(torque);
    else if(multibodyCollider != NULL)
    {
        btMultiBody* multiBody = multibodyCollider->m_multiBody;
        int index = multibodyCollider->m_link;
        
        if(index == -1) //base
            multiBody->addBaseTorque(torque);
        else
            multiBody->addLinkTorque(index, torque);
    }
}

void SolidEntity::ComputeFluidForces(const HydrodynamicsSettings& settings, const Ocean* fluid, const btTransform& cogTransform, const btTransform& geometryTransform, const btVector3& v, const btVector3& omega, const btVector3& a, const btVector3& epsilon, btVector3& Fb, btVector3& Tb, btVector3& Fd, btVector3& Td, btVector3& Fa, btVector3& Ta)
{
    //Set zeros
	Fb.setZero();
	Tb.setZero();
	Fd.setZero();
	Td.setZero();
	Fa.setZero();
	Ta.setZero();
	
	if(!computeHydro || mesh == NULL)
		return;
    
	//Calculate fluid dynamics forces and torques
    btVector3 p = cogTransform.getOrigin();
    
	//Loop through all faces...
    for(int i=0; i<mesh->faces.size(); ++i)
    {
        //Global coordinates
		glm::vec3 p1gl = mesh->vertices[mesh->faces[i].vertexID[0]].pos;
		glm::vec3 p2gl = mesh->vertices[mesh->faces[i].vertexID[1]].pos;
		glm::vec3 p3gl = mesh->vertices[mesh->faces[i].vertexID[2]].pos;
        btVector3 p1 = geometryTransform * btVector3(p1gl.x,p1gl.y,p1gl.z);
        btVector3 p2 = geometryTransform * btVector3(p2gl.x,p2gl.y,p2gl.z);
        btVector3 p3 = geometryTransform * btVector3(p3gl.x,p3gl.y,p3gl.z);
        
        //Check if underwater
        btScalar pressure = (fluid->GetPressure(p1) + fluid->GetPressure(p2) + fluid->GetPressure(p3))/btScalar(3);
        if(pressure <= btScalar(0.))
            continue;
        
        //Calculate face features
        btVector3 fv1 = p2-p1; //One side of the face (triangle)
        btVector3 fv2 = p3-p1; //Another side of the face (triangle)
        btVector3 fc = (p1+p2+p3)/btScalar(3); //Face centroid
        btVector3 fn = fv1.cross(fv2); //Normal of the face (length != 1)
        btScalar len = fn.length();
        btVector3 fn1 = fn/len; //Normalised normal (length = 1)
        btScalar A = len/btScalar(2); //Area of the face (triangle)
        
        //Buoyancy force
        btVector3 Fbi = -fn/btScalar(2)*pressure; //Buoyancy force per face (based on pressure)
        
        //Accumulate
        Fb += Fbi;
        Tb += (fc - p).cross(Fbi);
        
        //Damping force
        if(settings.dampingForces)
		{
            //Skin drag force
            btVector3 vc = fluid->GetFluidVelocity(fc) - (v + omega.cross(fc - p)); //Water velocity at face center
            btVector3 vt = vc - (vc.dot(fn)*fn)/fn.length2(); //Water velocity projected on face (tangent to face)
            btVector3 Fds = fluid->getFluid()->viscosity * vt * A / btScalar(0.0001);
            //btVector3 Fds = vt.safeNormalize()*btScalar(0.5)*fluid->getFluid()->density*btScalar(1.328)/1000.0*vt.length2()*fn.length()/btScalar(2);
        
            //Pressure drag force
            btVector3 vn = vc - vt; //Water velocity normal to face
            btVector3 Fdp(0,0,0);
            if(fn.dot(vn) < btScalar(0))
                Fdp = btScalar(0.5)*fluid->getFluid()->density * vn * vn.length() * A;
            
            //Accumulate
            Fd += Fds + Fdp;
            Td += (fc - p).cross(Fds + Fdp);
        }
        
        //Added mass effect
		if(settings.addedMassForces)
        {
            btVector3 ac = -(a + epsilon.cross(fc - p)); //Water acceleration at face center (velocity of fluid is constant)
            btVector3 Fai(0,0,0);
            btScalar an; //acceleration normal to face
            
            if((an = fn1.dot(ac)) < btScalar(0))
            {
                btScalar d = btScalar(1.)/(-an + btScalar(1.)); //Positive thickness of affected layer of fluid
                Fai = fluid->getFluid()->density * A * d * an * fn1; //Fa = rho V a = rho A d a
            }
            
            //Accumulate
            Fa += Fai;
            Ta += (fc - p).cross(Fai);
        }
    }
}

void SolidEntity::ComputeFluidForces(const HydrodynamicsSettings& settings, const Ocean* fluid, btVector3& Fb, btVector3& Tb, btVector3& Fd, btVector3& Td, btVector3& Fa, btVector3& Ta)
{
    btTransform T = getTransform() * localTransform.inverse();
    btVector3 v = getLinearVelocity();
    btVector3 omega = getAngularVelocity();
	btVector3 a = getLinearAcceleration();
    btVector3 epsilon = getAngularAcceleration();
	ComputeFluidForces(settings, fluid, getTransform(), T, v, omega, a, epsilon, Fb, Tb, Fd, Td, Fa, Ta);
}

void SolidEntity::ApplyFluidForces(const HydrodynamicsSettings& settings, const Ocean* fluid)
{
    btVector3 Fb;
    btVector3 Tb;
    btVector3 Fd;
    btVector3 Td;
    btVector3 Fa;
    btVector3 Ta;
    ComputeFluidForces(settings, fluid, Fb, Tb, Fd, Td, Fa, Ta);
    
    ApplyCentralForce(Fb + Fd + Fa);
    ApplyTorque(Tb + Td + Ta);
}
