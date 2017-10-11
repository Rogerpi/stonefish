//
//  JointsTestApp.h
//  Stonefish
//
//  Created by Patryk Cieslak on 03/03/2014.
//  Copyright (c) 2014-2017 Patryk Cieslak. All rights reserved.
//

#ifndef __Stonefish__JointsTestApp__
#define __Stonefish__JointsTestApp__

#include "SimulationApp.h"
#include "JointsTestManager.h"

class JointsTestApp : public SimulationApp
{
public:
    JointsTestApp(std::string dataDirPath, std::string shaderDirPath, int width, int height, JointsTestManager* sim);
};

#endif
