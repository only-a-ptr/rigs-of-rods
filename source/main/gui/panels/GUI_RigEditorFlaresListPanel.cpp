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

#include "GUI_RigEditorFlaresListPanel.h"

using namespace RoR;
using namespace GUI;
using namespace RigEditor;

RigEditorFlaresListPanel::RigEditorFlaresListPanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config):
    // Superclass
    GuiDynamicListPanelBase(
        m_flares_list_panel,       //parent_panel,
        m_items_listbox,           //items_listbox,
	    m_select_all_button,       //select_all_button,
	    m_deselect_all_button,     //deselect_all_button,
        m_select_by_type_combobox, //select_by_type_combobox,
        m_add_new_button           //add_item_button
    ),
    m_rig_editor_interface(rig_editor_interface),
    m_config(config)
{}

#define FLARE_FORWARD_CALLBACK(USERDATA_PTR, STATE_VALUE, FUNC_NAME) \
{ \
	assert(userdata != nullptr); \
	assert(userdata->bound_object_ptr != nullptr); \
	m_rig_editor_interface->FUNC_NAME( \
		static_cast<RigEditor::Flare*>((USERDATA_PTR)->bound_object_ptr), (USERDATA_PTR)->bound_object_index, (STATE_VALUE)); \
}

void RigEditorFlaresListPanel::OnListItemMouseFocusGained   (unsigned int item_index, GuiListWidgetUserdata* userdata)
{
    FLARE_FORWARD_CALLBACK(userdata, true, CommandSetFlareHovered);
}

void RigEditorFlaresListPanel::OnListItemMouseFocusLost   (unsigned int item_index, GuiListWidgetUserdata* userdata)
{
    FLARE_FORWARD_CALLBACK(userdata, false, CommandSetFlareHovered);
}

void RigEditorFlaresListPanel::OnListItemMouseClicked       (unsigned int item_index, GuiListWidgetUserdata* userdata)
{
	FLARE_FORWARD_CALLBACK(userdata, true, CommandScheduleSetFlareSelected);
}

void RigEditorFlaresListPanel::OnAddItemMouseClicked        (MyGUI::Button* button)
{
    // TODO: "add flare" panel
}

void RigEditorFlaresListPanel::OnSelectByTypeItemHover      (unsigned int item_value)
{

}

void RigEditorFlaresListPanel::OnSelectByTypeItemClick      (unsigned int item_value)
{}

void RigEditorFlaresListPanel::OnSelectAllMouseClicked      (MyGUI::Button* button)
{
    m_rig_editor_interface->CommandScheduleSetAllFlaresSelected(true);
}
void RigEditorFlaresListPanel::OnSelectAllMouseFocusGained  (MyGUI::Button* button)
{
    m_rig_editor_interface->CommandSetAllFlaresHovered(true);
}
void RigEditorFlaresListPanel::OnSelectAllMouseFocusLost    (MyGUI::Button* button)
{
    m_rig_editor_interface->CommandSetAllFlaresHovered(false);
}                                                  
void RigEditorFlaresListPanel::OnDeselectAllMouseClicked    (MyGUI::Button* button)
{
    m_rig_editor_interface->CommandScheduleSetAllFlaresSelected(false);
}
void RigEditorFlaresListPanel::OnDeselectAllMouseFocusGained(MyGUI::Button* button)
{}
void RigEditorFlaresListPanel::OnDeselectAllMouseFocusLost  (MyGUI::Button* button)
{}

void RigEditorFlaresListPanel::UpdateFlaresList(std::vector<Flare*> item_list)
{
	this->ClearList();

	auto end = item_list.end();
    int sequence = 0;
	for (auto itor = item_list.begin(); itor != end; ++itor)
	{
		Flare* flare = *itor;
        wchar_t name[100];
        swprintf(name, L"Flare %d", sequence); // FIXME: Generate proper name using Flare::GetTypeNameW()
		unsigned int index = this->AddItemToList(name, static_cast<void*>(flare));
        sequence++;
	}
}
