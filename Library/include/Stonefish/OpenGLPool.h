//
//  OpenGLPool.h
//  Stonefish
//
//  Created by Patryk Cieslak on 19/10/2017.
//  Copyright (c) 2017 Patryk Cieslak. All rights reserved.
//

#ifndef __Stonefish_OpenGLPool__
#define __Stonefish_OpenGLPool__

#include "OpenGLContent.h"
#include "GLSLShader.h"

class OpenGLPool
{
public:
    OpenGLPool();
    ~OpenGLPool();
    
    void InitPool();
    
    
private:    
    GLSLShader* poolShaders[3]; //Surface, backsurface, volume
};

#endif