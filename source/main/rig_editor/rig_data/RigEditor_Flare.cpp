/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   
	@date   06/2015
	@author Petr Ohlidal
*/

#include "RigEditor_Flare.h"

#include "RigDef_File.h"
#include "RigEditor_Node.h"


namespace RoR
{

namespace RigEditor
{

Flare::Flare(RigDef::Flare2& source, Node* ref_node, Node* x_node, Node* y_node):
    m_source(source),
    m_ref_node(ref_node),
    m_x_node(x_node),
    m_y_node(y_node),
    m_flags(0)
{}

Flare::~Flare()
{}

Ogre::Vector3 Flare::GetPosition()
{
    Ogre::Vector3 ref_pos = m_ref_node->GetPosition();
    Ogre::Vector3 x_axis  = m_x_node->GetPosition() - ref_pos;
    Ogre::Vector3 y_axis  = m_y_node->GetPosition() - ref_pos;
    Ogre::Vector3 normal  = y_axis.crossProduct(x_axis);
    normal.normalise();

    return ref_pos + (x_axis * m_source.offset.x) + (y_axis * m_source.offset.y) + (normal * m_source.offset.z);
}

Ogre::Vector3 Flare::GetVisualPosition()
{
    Ogre::Vector3 ref_pos = m_ref_node->GetPosition();
    Ogre::Vector3 x_axis  = m_x_node->GetPosition() - ref_pos;
    Ogre::Vector3 y_axis  = m_y_node->GetPosition() - ref_pos;
    Ogre::Vector3 normal  = y_axis.crossProduct(x_axis);
    normal.normalise();

    // Roughly matches calculation at gameplay, needs more work.
    Ogre::Vector3 pos =  ref_pos + (x_axis * m_source.offset.x) + (y_axis * m_source.offset.y);
    return pos - (0.1 * normal * m_source.offset.z);
}

RigDef::Flare2 & Flare::GetUpdatedDefinition()
{
    m_source.reference_node = m_ref_node->GetRef();
    m_source.node_axis_x    = m_x_node->GetRef();
    m_source.node_axis_y    = m_y_node->GetRef();

    return m_source;   
}

} // namespace RigEditor

} // namespace RoR
