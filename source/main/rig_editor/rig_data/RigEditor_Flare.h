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
	@date   06/2015
	@author Petr Ohlidal
*/

#pragma once

#include "RigDef_Prerequisites.h"
#include "RigDef_File.h" // For RigDef::Flare
#include "RigEditor_ForwardDeclarations.h"
#include "RoRPrerequisites.h"

namespace RoR
{

namespace RigEditor
{

class Flare
{
	friend class RigEditor::Rig;

public:

	Flare(RigDef::Flare2& source, Node* ref_node, Node* x_node, Node* y_node);

	~Flare();

    RigDef::Flare2 &       GetUpdatedDefinition();

	// Getters
	inline Node*           GetReferenceNode() const { return m_ref_node; }
    inline Node*           GetXNode() const         { return m_x_node; }
    inline Node*           GetYNode() const         { return m_y_node; }

    BITMASK_PROPERTY(m_flags,  1, IS_GEOMETRY_DIRTY,          IsGeometryDirty,         SetGeometryIsDirty)
	BITMASK_PROPERTY(m_flags,  2, IS_SELECTION_STATE_DIRTY,   IsSelectionStateDirty,   SetSelectionStateIsDirty)
	BITMASK_PROPERTY(m_flags,  3, IS_SELECTED,                IsSelected,              SetIsSelected)
	BITMASK_PROPERTY(m_flags,  4, IS_HOVERED,                 IsHovered,               SetIsHovered)
    // When user clicks "select this element", the element is scheduled for selection
    // Actual selection is performed during main-loop update
    BITMASK_PROPERTY(m_flags,  6, IS_SCHEDULED_FOR_SELECT,    IsScheduledForSelect,    SetIsScheduledForSelect)
    BITMASK_PROPERTY(m_flags,  7, IS_SCHEDULED_FOR_DESELECT,  IsScheduledForDeselect,  SetIsScheduledForDeselect)
	
protected:

    RigEditor::Node*       m_ref_node;
    RigEditor::Node*       m_x_node;
	RigEditor::Node*       m_y_node;
    RigDef::Flare2         m_source;
    unsigned int           m_flags;
};

} // namespace RigEditor

} // namespace RoR
