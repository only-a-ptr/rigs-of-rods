// This source file is part of Rigs of Rods
// Copyright 2005-2015 Rigs of Rods contributors

// For more information, see http://www.rigsofrods.com/

// Rigs of Rods is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3, as
// published by the Free Software Foundation.

// Rigs of Rods is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.

#include "MaterialReplacer.h"

#include <Ogre.h>
#include <OgrePrerequisites.h>

void MaterialReplacer::replaceMeshMaterials(Ogre::v1::Entity *e)
{
	Ogre::v1::MeshPtr m = e->getMesh();
	if (!m.isNull())
	{
		for (int n=0; n<(int)m->getNumSubMeshes();n++)
		{
			Ogre::v1::SubMesh *sm = m->getSubMesh(n);
			if (this->hasReplacementForMaterial(sm->getMaterialName()))
			{
				Ogre::String newMat = this->getReplacementForMaterial(sm->getMaterialName());
				sm->setMaterialName(newMat);
			}
		}
	}

	for (int n=0; n<(int)e->getNumSubEntities();n++)
	{
		Ogre::v1::SubEntity *subent = e->getSubEntity(n);
		if (this->hasReplacementForMaterial(subent->getMaterial()->getName()))
		{
			Ogre::String newMat = this->getReplacementForMaterial(subent->getMaterial()->getName());
			subent->setMaterialName(newMat, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
		}
	}
}

int MaterialReplacer::hasReplacementForMaterial(Ogre::String material)
{
	int res = (int)replaceMaterials.count(material);
	if (!res)
    {
		return (int)replaceMaterials.count(material);
    }
	return res;
}

Ogre::String MaterialReplacer::getReplacementForMaterial(Ogre::String material)
{
	Ogre::String res = replaceMaterials[material];
	if (res.empty())
    {
		return replaceMaterials[material];
    }
	return res;
}

int MaterialReplacer::addMaterialReplace(Ogre::String from, Ogre::String to)
{
	replaceMaterials[from] = to;
	return 0;
}
