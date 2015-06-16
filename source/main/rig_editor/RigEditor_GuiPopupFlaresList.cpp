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

#include "RigEditor_GuiPopupFlaresList.h"

#include "RigEditor_IMain.h"

using namespace RoR;
using namespace RigEditor;

GuiPopupFlaresList::GuiPopupFlaresList(
		RigEditor::IMain* rig_editor_interface,
		MyGUI::PopupMenu* parent_menu, 
		MyGUI::MenuItem* select_all_menuitem,  // = nullptr
		MyGUI::MenuItem* deselect_all_menuitem // = nullptr
		):
	GuiPopupDynamicListBase(parent_menu, select_all_menuitem, deselect_all_menuitem),
	m_rig_editor_interface(rig_editor_interface)
{
	assert(rig_editor_interface != nullptr);
}

#define FLARE_FORWARD_CALLBACK(MENUITEM_PTR, USERDATA_PTR, STATE_VALUE, FUNC_NAME) \
{ \
	assert(userdata != nullptr); \
	assert(userdata->bound_object_ptr != nullptr); \
	m_rig_editor_interface->FUNC_NAME( \
		static_cast<Flare*>((USERDATA_PTR)->bound_object_ptr), (USERDATA_PTR)->bound_object_index, (STATE_VALUE)); \
}

void GuiPopupFlaresList::OnItemMouseFocusGained       (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata)
{
	FLARE_FORWARD_CALLBACK(menu_item, userdata, true, CommandSetFlareHovered);
}

void GuiPopupFlaresList::OnItemMouseFocusLost       (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata)
{
	FLARE_FORWARD_CALLBACK(menu_item, userdata, false, CommandSetFlareHovered);
}

void GuiPopupFlaresList::OnItemMouseClicked       (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata)
{
	FLARE_FORWARD_CALLBACK(menu_item, userdata, true, CommandScheduleSetFlareSelected);
}

void GuiPopupFlaresList::OnSelectAllMouseClicked  (MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata)
{
	m_rig_editor_interface->CommandScheduleSetAllFlaresSelected(true);
}

void GuiPopupFlaresList::OnDeselectAllMouseClicked(MyGUI::MenuItem* menu_item, GuiWidgetUserdata* userdata)
{
	m_rig_editor_interface->CommandScheduleSetAllFlaresSelected(false);
}

void GuiPopupFlaresList::UpdateFlaresList(std::vector<Flare*> wheels_list)
{
	this->ClearList();

	auto end = wheels_list.end();
	int index = 0;
	for (auto itor = wheels_list.begin(); itor != end; ++itor)
	{
		Flare* wheel = *itor;
		MyGUI::MenuItem* menuitem = this->AddItemToList(static_cast<void*>(wheel), index);
		menuitem->setVisible(true);
		char caption[25];
		sprintf(caption, "Flare %d", index);
		menuitem->setCaption(caption);
		++index;
	}
}

void GuiPopupFlaresList::OnSelectAllMouseFocusGained(MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *)
{
	m_rig_editor_interface->CommandSetAllFlaresHovered(true);
}

void GuiPopupFlaresList::OnSelectAllMouseFocusLost(MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *)
{
	m_rig_editor_interface->CommandSetAllFlaresHovered(false);
}

void GuiPopupFlaresList::OnDeselectAllMouseFocusGained(MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *)
{}

void GuiPopupFlaresList::OnDeselectAllMouseFocusLost(MyGUI::MenuItem *,RoR::RigEditor::GuiPopupDynamicListBase::GuiWidgetUserdata *)
{}