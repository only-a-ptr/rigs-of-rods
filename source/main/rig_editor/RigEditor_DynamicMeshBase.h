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

#include "RigEditor_ForwardDeclarations.h"

#include <OgreMaterialManager.h>

namespace RoR
{

namespace RigEditor
{

/// Simplistic wrapper of Ogre::ManualObject
class DynamicMeshBase
{
public:
    DynamicMeshBase():
        m_scene_node(nullptr)
    {}

    void BeginUpdate();
    void EndUpdate();
    void AttachToScene(Ogre::SceneNode* parent_scene_node);
    void DetachFromScene();
    void SetPosition(Ogre::Vector3 pos);
    void SetOrientation(Ogre::Quaternion rot);

protected:

    /// Creates+inits manual object. Material must be already defined.
    void Initialize(
        RigEditor::Main* rig_editor,
        size_t estimate_vertex_count
    );
    
    std::unique_ptr<Ogre::ManualObject> m_dynamic_mesh;
    Ogre::SceneNode*                    m_scene_node;
};

} // namespace RigEditor

} // namespace RoR
