/*    
    This file is a part of Stonefish.

    Stonefish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Stonefish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

//
//  Compound.h
//  Stonefish
//
//  Created by Patryk Cieslak on 19/09/17.
//  Copyright (c) 2017-2019 Patryk Cieslak. All rights reserved.
//

#ifndef __Stonefish_Compound__
#define __Stonefish_Compound__

#include "entities/SolidEntity.h"

namespace sf
{
    //! A structure representing one part of the compound body.
    typedef struct
    {
        SolidEntity* solid;
        Transform origin;
        bool isExternal;
    } CompoundPart;
    
    //! A class representing a rigid body built of multiple other rigid bodies.
    class Compound : public SolidEntity
    {
    public:
        //! A constructor.
        /*!
         \param uniqueName a name for the body
         \param firstExternalPart a pointer to the first external rigid body
         \param origin a transformation from the compound body origin to the first part origin
         \param bpt an enum defining the type of physics computations required for the body (currently bodies cannot transfer between mediums)
         */
        Compound(std::string uniqueName, SolidEntity* firstExternalPart, const Transform& origin, BodyPhysicsType bpt);
        
        //! A destructor.
        ~Compound();
        
        //! A method adding new internal rigid body to the compound body.
        /*!
         \param solid a pointer to the rigid part
         \param origin a tranformation from the compound body origin to the part origin
         */
        void AddInternalPart(SolidEntity* solid, const Transform& origin);
        
        //! A method adding new external rigid body to the compound body.
        /*!
         \param solid a pointer to the rigid part
         \param origin a tranformation from the compound body origin to the part origin
         */
        void AddExternalPart(SolidEntity* solid, const Transform& origin);
        
        //! A method which computes hydrodynamic forces acting on the compound body.
        /*!
         \param settings a structure holding settings of the hydrodynamic computation
         \param ocn a pointer to a fluid entity (only Ocean supported now)
         */
        void ComputeHydrodynamicForces(HydrodynamicsSettings settings, Ocean* ocn);
        
        //! A method that computes aerodynamics.
        /*!
         \param atm a pointer to the atmosphere entity
        */
        void ComputeAerodynamicForces(Atmosphere* atm);
        
        //! A method returning the material of the body.
        Material getMaterial(size_t partId) const;
        
        //! A method returning the part id for the collision shape id
        size_t getPartId(size_t collisionShapeId) const;
        
        //! A method that returns the type of solid.
        SolidType getSolidType();
        
        //! A method that returns a copy of all physics mesh vertices in body origin frame.
        std::vector<Vertex>* getMeshVertices();
        
        //! A method that constructs a collision shape for the body.
        btCollisionShape* BuildCollisionShape();
        
        //! A method that builds a graphical object for the body.
        void BuildGraphicalObject();
        
        //! A method that returns elements that have to be rendered for the body.
        std::vector<Renderable> Render();
        
    private:
        std::vector<CompoundPart> parts; //Parts of the compound solid
        std::vector<size_t> collisionPartId;
        
        void RecalculatePhysicalProperties();
    };

}

#endif
