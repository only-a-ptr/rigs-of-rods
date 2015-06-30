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

#include "RigEditor_PointListDynamicMesh.h"
#include "RigEditor_Main.h"

#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreTechnique.h>
#include <OgrePass.h>
#include <OgreManualObject.h>
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

#include <memory>

using namespace RoR;
using namespace RigEditor;

PointListDynamicMesh::PointListDynamicMesh(
        RigEditor::Main* rig_editor,
        float point_size,
        size_t estimate_point_count
)
{
    static const char* material_name = "RigEditor_PointListDynamicMeshMaterial";
    this->CheckAndCreateMaterial(material_name, point_size);
    this->Initialize(rig_editor, estimate_point_count);
	

	/* Init */
	m_dynamic_mesh->begin(material_name, Ogre::RenderOperation::OT_POINT_LIST);

	// Init with dummy geometry
	for (unsigned int i = 0; i < estimate_point_count; ++i)
	{
		m_dynamic_mesh->position(Ogre::Vector3::ZERO);
		m_dynamic_mesh->colour(Ogre::ColourValue::ZERO);
	}

	/* Finalize */
	m_dynamic_mesh->end();

    // Clear dummy geometry
    this->BeginUpdate();
    this->EndUpdate();
}

void PointListDynamicMesh::CheckAndCreateMaterial(const char* mat_name, float point_size)
{
	/* Prepare material */
	if (Ogre::MaterialManager::getSingleton().resourceExists(mat_name))
	{
        return;
    }
	Ogre::MaterialPtr node_mat = static_cast<Ogre::MaterialPtr>(
		Ogre::MaterialManager::getSingleton().create(mat_name, 
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
	);

	node_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
	node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
	node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
	node_mat->setLightingEnabled(false);
	node_mat->setReceiveShadows(false);
	node_mat->setPointSize(point_size);
}

void PointListDynamicMesh::AddPoint(Ogre::Vector3 const & pos, Ogre::ColourValue const & color)
{
	m_dynamic_mesh->position(pos);
	m_dynamic_mesh->colour(color);	
}
