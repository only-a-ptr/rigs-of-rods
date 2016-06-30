/*
This source file is part of Rigs of Rods
Copyright 2005-2012 Pierre-Michel Ricordel
Copyright 2007-2012 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "Road.h"

#include <Ogre.h>

Road::Road(Ogre::Vector3 start) :
	  cur_rtype(0)
	, free_rtype(0)
	, lastpturn(0)
	, ppitch(0)
	, ppos(start)
	, pturn(0)
{
	addRoadType("Road");
	addRoadType("RoadBorderLeft");
	addRoadType("RoadBorderRight");
	addRoadType("RoadBorderBoth");
	addRoadType("RoadBridge");
	strcpy(curtype, rtypes[cur_rtype].name);
	tenode = rtypes[cur_rtype].node;
	tenode->setVisible(true);
	preparePending();
};

void Road::preparePending()
{
	//setup rotation points
	lastpturn = pturn;
	protl = ppos+Ogre::Quaternion(Ogre::Degree(pturn), Ogre::Vector3::UNIT_Y)*Ogre::Vector3(0, 0, 4.5);
	protr = ppos+Ogre::Quaternion(Ogre::Degree(pturn), Ogre::Vector3::UNIT_Y)*Ogre::Vector3(0, 0, -4.5);
	tenode->setPosition(ppos);
	tenode->setOrientation(Ogre::Quaternion(Ogre::Degree(pturn), Ogre::Vector3::UNIT_Y)*Ogre::Quaternion(Ogre::Degree(ppitch), Ogre::Vector3::UNIT_Z));
	tenode->pitch(Ogre::Degree(-90));
}

void Road::updatePending()
{
	if (pturn-lastpturn > 0)
	{
		tenode->setPosition(Ogre::Quaternion(Ogre::Degree(pturn-lastpturn), Ogre::Vector3::UNIT_Y)*(ppos-protl)+protl);
	} else
	{
		if (pturn-lastpturn < 0)
			tenode->setPosition(Ogre::Quaternion(Ogre::Degree(pturn-lastpturn), Ogre::Vector3::UNIT_Y)*(ppos-protr)+protr);
		else
			tenode->setPosition(ppos);
	}
	tenode->setOrientation(Ogre::Quaternion(Ogre::Degree(pturn), Ogre::Vector3::UNIT_Y)*Ogre::Quaternion(Ogre::Degree(ppitch), Ogre::Vector3::UNIT_Z));
	tenode->pitch(Ogre::Degree(-90));
}

void Road::reset(Ogre::Vector3 start)
{
	cur_rtype = 0;
	lastpturn = 0;
	ppitch = 0;
	ppos = start;
	pturn = 0;
	strcpy(curtype, rtypes[cur_rtype].name);
	tenode = rtypes[cur_rtype].node;
	tenode->setVisible(true);
	preparePending();
}

void Road::addRoadType(const char* name)
{
	// create visuals
	Ogre::String entity_name = Ogre::String("RoadPreview-").append(name);
	Ogre::String mesh_name = Ogre::String(name).append(".mesh");
	Ogre::v1::Entity *te = gEnv->sceneManager->createEntity(mesh_name);
	te->setName(entity_name);

	te->setCastShadows(false);

	if (free_rtype < MAX_RTYPES)
	{
		rtypes[free_rtype].node = gEnv->sceneManager->getRootSceneNode()->createChildSceneNode();
		rtypes[free_rtype].node->attachObject(te);
		rtypes[free_rtype].node->setVisible(false);
		strcpy(rtypes[free_rtype].name, name);
		free_rtype++;
	}
}

void Road::toggleType()
{
	Ogre::Quaternion rot = tenode->getOrientation();
	Ogre::Vector3 pos = tenode->getPosition();
	cur_rtype++;
	if (cur_rtype >=  free_rtype || cur_rtype >=  MAX_RTYPES)
	{
		cur_rtype = 0;
	}
	strcpy(curtype, rtypes[cur_rtype].name);
	tenode->setVisible(false);
	tenode = rtypes[cur_rtype].node;
	tenode->setOrientation(rot);
	tenode->setPosition(pos);
	tenode->setVisible(true);
}

void Road::dpitch(float v)
{
	ppitch += v;
	updatePending();
}

void Road::dturn(float v)
{
	pturn += v;
	updatePending();
}

void Road::append()
{
	// register pending and set collision boxes
	// first, calculate the real position
	if (pturn-lastpturn > 0)
	{
		rpos = Ogre::Quaternion(Ogre::Degree(pturn-lastpturn), Ogre::Vector3::UNIT_Y)*(ppos-protl)+protl;
	} else
	{
		if (pturn-lastpturn < 0)
			rpos = Ogre::Quaternion(Ogre::Degree(pturn-lastpturn), Ogre::Vector3::UNIT_Y)*(ppos-protr)+protr;
		else
			rpos = ppos;
	}
	// set real rot
	rrot = Ogre::Vector3(0, pturn, ppitch);
	// set new pending coordinates (we keep angles)
	ppos = rpos+(Ogre::Quaternion(Ogre::Degree(pturn), Ogre::Vector3::UNIT_Y)*Ogre::Quaternion(Ogre::Degree(ppitch), Ogre::Vector3::UNIT_Z))*Ogre::Vector3(10,0,0);
	// prepare pending
	preparePending();
}
