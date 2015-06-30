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

#include "GuiPanelBase.h"
#include <MyGUI_Prerequest.h>
#include <MyGUI_UString.h>
#include <vector>

namespace RoR
{

namespace RigEditor
{

class GuiDynamicListPanelBase: public GuiPanelBase
{
public:
	struct GuiListWidgetUserdata
	{
		void* bound_object_ptr;
		int   bound_object_index;
	};

	/// @param parent_menu Required
	/// @param select_all_menuitem Optional
	/// @param deselect_all_menuitem Optional
	GuiDynamicListPanelBase(
		MyGUI::Window*                parent_panel,
        MyGUI::ListBox*               items_listbox,
	    MyGUI::Button*                select_all_button,
	    MyGUI::Button*                deselect_all_button,
        MyGUI::ComboBox*              select_by_type_combobox,
        MyGUI::Button*                add_item_button
		);

protected:

	void ClearList();

	unsigned int AddItemToList(MyGUI::UString const & name, void* bound_ptr);

	virtual void OnListItemMouseFocusGained   (unsigned int item_index, GuiListWidgetUserdata* userdata) = 0;
    virtual void OnListItemMouseFocusLost     (unsigned int item_index, GuiListWidgetUserdata* userdata) = 0;
	virtual void OnListItemMouseClicked       (unsigned int item_index, GuiListWidgetUserdata* userdata) = 0;

    virtual void OnAddItemMouseClicked        (MyGUI::Button* button) = 0;

    virtual void OnSelectByTypeItemHover      (unsigned int item_value) = 0;
    virtual void OnSelectByTypeItemClick      (unsigned int item_value) = 0;

	virtual void OnSelectAllMouseClicked      (MyGUI::Button* button) = 0;
	virtual void OnSelectAllMouseFocusGained  (MyGUI::Button* button) = 0;
	virtual void OnSelectAllMouseFocusLost    (MyGUI::Button* button) = 0;
	                                                          
	virtual void OnDeselectAllMouseClicked    (MyGUI::Button* button) = 0;
	virtual void OnDeselectAllMouseFocusGained(MyGUI::Button* button) = 0;
	virtual void OnDeselectAllMouseFocusLost  (MyGUI::Button* button) = 0;

	void ButtonCallback_MouseFocusGained(MyGUI::Widget* widget_a, MyGUI::Widget* widget_b);
	void ButtonCallback_MouseFocusLost  (MyGUI::Widget* widget_a, MyGUI::Widget* widget_b);
	void ButtonCallback_MouseClicked    (MyGUI::Widget* sender);

    void ListCallback_ItemClicked       (MyGUI::Widget* list, size_t index);
    void ListCallback_ItemHoverChanged  (MyGUI::Widget* list, size_t index);

	MyGUI::Window*                m_base_parent_panel;
    MyGUI::ListBox*               m_base_items_listbox;
	MyGUI::Button*                m_base_select_all_button;
	MyGUI::Button*                m_base_deselect_all_button;
    MyGUI::ComboBox*              m_base_select_by_type_combobox;
    MyGUI::Button*                m_base_add_item_button;

    size_t                        m_curr_hovered_item_index;
};

} // namespace RigEditor

} // namespace RoR
