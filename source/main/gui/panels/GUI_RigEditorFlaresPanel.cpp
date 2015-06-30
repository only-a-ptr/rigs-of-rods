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

#include "GUI_RigEditorFlaresPanel.h"
#include "RigEditor_Config.h"
#include "RigEditor_Main.h"
#include "RigEditor_Node.h"
#include "RigEditor_RigElementsAggregateData.h"

#include <MyGUI.h>

using namespace RoR;
using namespace GUI;

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------

#define SETUP_WIDGET_ALL(FUNCNAME, FIELD, LABEL, WIDGET, UNIFLAG, SRC, SRC_TYPE) \
	FUNCNAME(&(FIELD), LABEL, WIDGET, \
		m_data.GetFlagsPtr(), RigEditor::RigAggregateFlaresData::UNIFLAG, ((void*) &(SRC)), SRC_TYPE);

#define SETUP_EDITBOX(NAME, UNIFLAG, SRC, SRC_TYPE) \
	SETUP_WIDGET_ALL(SetupEditboxField, NAME##_field, NAME##_label, NAME##_editbox, UNIFLAG, SRC, EditboxFieldSpec::SRC_TYPE)

#define SETUP_COMBOBOX(NAME, UNIFLAG, SRC, SRC_TYPE) \
	SETUP_WIDGET_ALL(SetupGenericField, NAME##_field, NAME##_label, NAME##_combobox, UNIFLAG, SRC, GenericFieldSpec::SRC_TYPE)

RigEditorFlaresPanel::RigEditorFlaresPanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config):
	RigEditor::RigElementGuiPanelBase(rig_editor_interface, config, m_flares_panel, nullptr)
{
	// TODO // m_flags_tooltip_label->setTextColour(m_text_color_tooltip);
    
    // TODO // EditboxFieldSpec m_ref_node_field;
    // TODO // EditboxFieldSpec m_x_node_field;
    // TODO // EditboxFieldSpec m_y_node_field;
    SETUP_EDITBOX( m_ref_node            , FIELD_REF_NODE_IS_UNIFORM       , m_data.ref_node,                           SOURCE_DATATYPE_OBJECT_NODE);
    SETUP_EDITBOX( m_x_node              , FIELD_X_NODE_IS_UNIFORM         , m_data.x_node,                             SOURCE_DATATYPE_OBJECT_NODE);
    SETUP_EDITBOX( m_y_node              , FIELD_Y_NODE_IS_UNIFORM         , m_data.y_node,                             SOURCE_DATATYPE_OBJECT_NODE);

    // EditboxFieldSpec m_pos_x_field;
    // EditboxFieldSpec m_pos_y_field;
    // EditboxFieldSpec m_pos_z_field;
    SETUP_EDITBOX( m_x_offset,             FIELD_X_OFFSET_IS_UNIFORM,        m_data.x_offset,                           SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_y_offset,             FIELD_Y_OFFSET_IS_UNIFORM,        m_data.y_offset,                           SOURCE_DATATYPE_FLOAT);
    SETUP_EDITBOX( m_z_offset,             FIELD_Z_OFFSET_IS_UNIFORM,        m_data.z_offset,                           SOURCE_DATATYPE_FLOAT);
    // EditboxFieldSpec m_size_field;                                                                                   
    SETUP_EDITBOX( m_size,                 FIELD_SIZE_IS_UNIFORM,            m_data.size,                               SOURCE_DATATYPE_FLOAT);
    // EditboxFieldSpec m_flare_material_field;
    SETUP_EDITBOX( m_flare_material,       FIELD_FLARE_MAT_NAME_IS_UNIFORM,  m_data.flare_material_name,                SOURCE_DATATYPE_STRING);
    // EditboxFieldSpec m_materialflarebinding_field;
    SETUP_EDITBOX( m_materialflarebinding, FIELD_MATFLAREBINDING_IS_UNIFORM, m_data.materialflarebinding_material_name, SOURCE_DATATYPE_STRING);
    // EditboxFieldSpec m_blink_delay_field;
    SETUP_EDITBOX( m_blink_delay,          FIELD_BLINK_DELAY_IS_UNIFORM,     m_data.blink_delay_ms,                     SOURCE_DATATYPE_FLOAT);
    
    // GenericFieldSpec m_type_field;
    SETUP_COMBOBOX( m_type,                FIELD_TYPE_IS_UNIFORM,            m_data.type,                               SOURCE_DATATYPE_INT);
    // GenericFieldSpec m_controlnumber_field;                                                                          
    SETUP_COMBOBOX( m_control_number,      FIELD_CONTROL_NUMBER_IS_UNIFORM,  m_data.control_number,                     SOURCE_DATATYPE_INT);

	AlignToScreen(&config->gui_flares_edit_panel_position);
}

void RigEditorFlaresPanel::ButtonCallbackClick(MyGUI::Widget* sender)
{
    if (sender == m_ref_node_set_button)
    {
        RigEditor::Node* node = m_rig_editor_interface->GetCurrentRigLastSelectedNode();
        if (node != nullptr && node->GetRef() != m_data.ref_node)
        {
            m_data.ref_node = node->GetRef();
            m_data.SetWasRefNodeUpdated(true);
        }
    }
    else if (sender == m_x_node_set_button)
    {
        RigEditor::Node* node = m_rig_editor_interface->GetCurrentRigLastSelectedNode();
        if (node != nullptr && node->GetRef() != m_data.x_node)
        {
            m_data.x_node = node->GetRef();
            m_data.SetWasXNodeUpdated(true);
        }
    }
    else if (sender == m_y_node_set_button)
    {
        RigEditor::Node* node = m_rig_editor_interface->GetCurrentRigLastSelectedNode();
        if (node != nullptr && node->GetRef() != m_data.y_node)
        {
            m_data.y_node = node->GetRef();
            m_data.SetWasYNodeUpdated(true);
        }
    }
    else if (sender == m_swap_xy_nodes_button)
    {
        if (m_data.IsXNodeUniform() && m_data.IsYNodeUniform())
        {
            auto swap = m_data.x_node;
            m_data.x_node = m_data.y_node;
            m_data.y_node = swap;
            m_data.SetWasYNodeUpdated(true);
            m_data.SetWasXNodeUpdated(true);   
        }
    }
    else if (sender == m_size_scale_button)
    {
        // TODO
    }
    else if (sender == m_flare_material_reset_button)
    {
        m_data.flare_material_name.clear();
        m_data.SetWasFlareMatUpdated(true);
    }
    else
    {
        assert(false && "Invalid binding of RigEditorFlaresPanel::ButtonCallbackClick(MyGUI::Widget* sender)");
    }
}

void RigEditorFlaresPanel::UpdateFlaresData(RigEditor::RigAggregateFlaresData* data)
{
	m_data = *data;

	// Panel name
	if (data->num_elements == 1)
	{
		m_panel_widget->setCaption(L"Flare");
	}
	else
	{
		wchar_t caption[50];
		swprintf(caption, L"Flares [%d]", data->num_elements);
		m_panel_widget->setCaption(caption);
	}

	// Fields
	EditboxRestoreValue(&m_ref_node_field);
	EditboxRestoreValue(&m_x_node_field);
	EditboxRestoreValue(&m_y_node_field);
	EditboxRestoreValue(&m_x_offset_field);
	EditboxRestoreValue(&m_y_offset_field);
	EditboxRestoreValue(&m_z_offset_field);
	EditboxRestoreValue(&m_size_field);
	EditboxRestoreValue(&m_flare_material_field);
	EditboxRestoreValue(&m_materialflarebinding_field);
	EditboxRestoreValue(&m_blink_delay_field);

	ComboboxRestoreValue(&m_type_field);
	ComboboxRestoreValue(&m_control_number_field);

}
