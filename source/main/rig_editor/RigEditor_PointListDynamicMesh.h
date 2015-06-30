/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2015 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   
	@author Petr Ohlidal
	@date   06/2015
*/

#pragma once

#include "RigEditor_DynamicMeshBase.h"
#include "RigEditor_ForwardDeclarations.h"

#include <OgreMaterialManager.h>

namespace RoR
{

namespace RigEditor
{

/// Simplistic wrapper of Ogre::ManualObject
class PointListDynamicMesh: public DynamicMeshBase
{
public:
    PointListDynamicMesh(
        RigEditor::Main* rig_editor,
        float point_size,
        size_t estimate_point_count
    );
    void AddPoint(Ogre::Vector3 const & pos, Ogre::ColourValue const & color);
    

    private:
    
    void CheckAndCreateMaterial(const char* mat_name, float point_size);
};

} // namespace RigEditor

} // namespace RoR
