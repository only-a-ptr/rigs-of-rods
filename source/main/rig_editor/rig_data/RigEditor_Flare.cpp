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
#include "RigEditor_RigElementsAggregateData.h"


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

void Flare::Update(RigAggregateFlaresData *data)
{
    assert(data != nullptr);
    if (data->num_elements == 0)
    {
        return;
    }

    // Position - nodes
    bool position_changed = false;
    if (data->WasRefNodeUpdated() && data->ref_node != m_source.reference_node)
    {
        position_changed = true;
        m_source.reference_node = data->ref_node;
    }
    if (data->WasXNodeUpdated() && data->x_node != m_source.node_axis_x)
    {
        position_changed = true;
        m_source.node_axis_x = data->x_node;
    }
    if (data->WasYNodeUpdated() && data->y_node != m_source.node_axis_y)
    {
        position_changed = true;
        m_source.node_axis_y = data->y_node;
    }
    // Position - offsets
    if (data->WasXOffsetUpdated() && data->x_offset != m_source.offset.x)
    {
        position_changed = true;
        m_source.offset.x = data->x_offset;
    }
    if (data->WasYOffsetUpdated() && data->y_offset != m_source.offset.y)
    {
        position_changed = true;
        m_source.offset.z = data->y_offset;
    }
    if (data->WasZOffsetUpdated() && data->z_offset != m_source.offset.z)
    {
        position_changed = true;
        m_source.offset.z = data->z_offset;
    }
    // Update visuals?
    if (position_changed)
    {
        this->SetGeometryIsDirty(true);
    }

    // ----- Other -----
    if (data->WasSizeUpdated           ()) { m_source.size = data->size; }
    if (data->WasFlareMatUpdated       ()) { m_source.material_name = data->flare_material_name; }
    if (data->WasMatFlareBindingUpdated()) { m_materialflarebinding_material_name = data->materialflarebinding_material_name; }
    if (data->WasTypeUpdated           ()) { m_source.type = RigDef::Flare2::Type(data->type); }
    if (data->WasControlNumberUpdated  ()) { m_source.control_number = data->control_number; }
    if (data->WasBlinkDelayUpdated     ()) { m_source.blink_delay_milis = data->blink_delay_ms; }

}

const wchar_t* Flare::GetTypeNameW()
{
    switch (m_source.type)
    {
        case RigDef::Flare2::TYPE_b_BRAKELIGHT:    return L"Brake";
        case RigDef::Flare2::TYPE_f_HEADLIGHT:     return L"Headlight";
        case RigDef::Flare2::TYPE_l_LEFT_BLINKER:  return L"Signal left";
        case RigDef::Flare2::TYPE_R_REVERSE_LIGHT: return L"Reverse";
        case RigDef::Flare2::TYPE_r_RIGHT_BLINKER: return L"Signal right";
        case RigDef::Flare2::TYPE_u_USER:          return L"User";
        default:                                   return L"Unknown";
    }
}

} // namespace RigEditor

} // namespace RoR
