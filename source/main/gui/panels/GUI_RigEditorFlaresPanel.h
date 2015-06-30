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

#pragma once

/** 
	@file   
	@author Petr Ohlidal
	@date   06/2015
*/

#include "ForwardDeclarations.h"
#include "GUI_RigEditorFlaresPanelLayout.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_RigElementGuiPanelBase.h"
#include "RigEditor_RigElementsAggregateData.h"

namespace RoR
{

namespace GUI
{

class RigEditorFlaresPanel: public RigEditorFlaresPanelLayout, public RigEditor::RigElementGuiPanelBase
{

public:

	RigEditorFlaresPanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config);

	void UpdateFlaresData(RigEditor::RigAggregateFlaresData* query_result);

	inline RigEditor::RigAggregateFlaresData* GetFlaresData() { return &m_data; }

	inline void HideAndReset()
	{
		Hide();
		m_data.Reset();
	}

private:

	void ButtonCallbackClick(MyGUI::Widget* sender);

		// Aggregate rig data
	RigEditor::RigAggregateFlaresData m_data;

	// GUI form fields
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_ref_node_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_x_node_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_y_node_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_x_offset_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_y_offset_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_z_offset_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_size_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_flare_material_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_materialflarebinding_field;
    RigEditor::RigElementGuiPanelBase::EditboxFieldSpec m_blink_delay_field;
    RigEditor::RigElementGuiPanelBase::GenericFieldSpec m_type_field;
    RigEditor::RigElementGuiPanelBase::GenericFieldSpec m_control_number_field;
};

} // namespace GUI

} // namespace RoR
