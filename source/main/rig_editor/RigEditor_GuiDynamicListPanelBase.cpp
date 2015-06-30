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

#include "RigEditor_GuiDynamicListPanelBase.h"

#include <MyGUI.h>

using namespace RoR;
using namespace RigEditor;

GuiDynamicListPanelBase::GuiDynamicListPanelBase(
		MyGUI::Window*                parent_panel,
        MyGUI::ListBox*               items_listbox,
	    MyGUI::Button*                select_all_button,
	    MyGUI::Button*                deselect_all_button,
        MyGUI::ComboBox*              select_by_type_combobox,
        MyGUI::Button*                add_item_button
		):
    GuiPanelBase(parent_panel),
    m_base_parent_panel           (parent_panel),
    m_base_items_listbox          (items_listbox),
    m_base_select_all_button      (select_all_button),
    m_base_deselect_all_button    (deselect_all_button),
    m_base_select_by_type_combobox(select_by_type_combobox),
    m_base_add_item_button        (add_item_button),
    m_curr_hovered_item_index     (0xFFFFFFFF)
{
	assert(parent_panel != nullptr);
    this->Hide();

	m_base_select_all_button  ->eventMouseSetFocus    += MyGUI::newDelegate(this, &GuiDynamicListPanelBase::ButtonCallback_MouseFocusGained);
	m_base_select_all_button  ->eventMouseLostFocus   += MyGUI::newDelegate(this, &GuiDynamicListPanelBase::ButtonCallback_MouseFocusLost);
	m_base_select_all_button  ->eventMouseButtonClick += MyGUI::newDelegate(this, &GuiDynamicListPanelBase::ButtonCallback_MouseClicked);
                                                                                                       
	m_base_deselect_all_button->eventMouseSetFocus    += MyGUI::newDelegate(this, &GuiDynamicListPanelBase::ButtonCallback_MouseFocusGained);
	m_base_deselect_all_button->eventMouseLostFocus   += MyGUI::newDelegate(this, &GuiDynamicListPanelBase::ButtonCallback_MouseFocusLost);
	m_base_deselect_all_button->eventMouseButtonClick += MyGUI::newDelegate(this, &GuiDynamicListPanelBase::ButtonCallback_MouseClicked);

    if (add_item_button != nullptr)
    {
        m_base_add_item_button->eventMouseButtonClick += MyGUI::newDelegate(this, &GuiDynamicListPanelBase::ButtonCallback_MouseClicked);
    }

    items_listbox->eventListMouseItemFocus       += MyGUI::newDelegate(this, &GuiDynamicListPanelBase::ListCallback_ItemHoverChanged);
    items_listbox->eventListMouseItemActivate    += MyGUI::newDelegate(this, &GuiDynamicListPanelBase::ListCallback_ItemClicked);

    // TODO: Select by type 
}

void GuiDynamicListPanelBase::ClearList()
{
	m_base_items_listbox->removeAllItems();
    m_curr_hovered_item_index = 0xFFFFFFFF;
}

unsigned int GuiDynamicListPanelBase::AddItemToList(MyGUI::UString const & name, void* bound_ptr)
{
    size_t index = m_base_items_listbox->getItemCount();
	// Create list item
    GuiListWidgetUserdata userdata;
	userdata.bound_object_ptr = bound_ptr;
    userdata.bound_object_index = index;
    m_base_items_listbox->addItem(name, userdata);
    return index;
}

void GuiDynamicListPanelBase::ButtonCallback_MouseFocusGained(MyGUI::Widget* old_focus, MyGUI::Widget* sender)
{
	if (sender == m_base_select_all_button)
	{
		this->OnSelectAllMouseFocusGained(static_cast<MyGUI::Button*>(sender));
	}
	else if (sender == m_base_deselect_all_button)
	{
		this->OnDeselectAllMouseFocusGained(static_cast<MyGUI::Button*>(sender));
	}
    else
    {
        assert(false && "Invalid callback binding");
    }
}

void GuiDynamicListPanelBase::ButtonCallback_MouseFocusLost(MyGUI::Widget* sender, MyGUI::Widget* new_focus)
{
	if (sender == m_base_select_all_button)
	{
		this->OnSelectAllMouseFocusLost(static_cast<MyGUI::Button*>(sender));
	}
	else if (sender == m_base_deselect_all_button)
	{
		this->OnDeselectAllMouseFocusLost(static_cast<MyGUI::Button*>(sender));
	}
	else if (sender == m_base_add_item_button)
	{
		this->OnAddItemMouseClicked(static_cast<MyGUI::Button*>(sender));
	}
    else
    {
        assert(false && "Invalid callback binding");
    }
}

void GuiDynamicListPanelBase::ButtonCallback_MouseClicked(MyGUI::Widget* sender)
{
	if (sender == m_base_select_all_button)
	{
		this->OnSelectAllMouseClicked(static_cast<MyGUI::Button*>(sender));
	}
	else if (sender == m_base_deselect_all_button)
	{
		this->OnDeselectAllMouseClicked(static_cast<MyGUI::Button*>(sender));
	}
	else if (sender == m_base_add_item_button)
	{
		this->OnAddItemMouseClicked(static_cast<MyGUI::Button*>(sender));
	}
    else
    {
        assert(false && "Invalid callback binding");
    }
}

void GuiDynamicListPanelBase::ListCallback_ItemClicked    (MyGUI::Widget* list, size_t index)
{
    MyGUI::ListBox* listbox = static_cast<MyGUI::ListBox*>(list);
    GuiListWidgetUserdata* userdata = listbox->getItemDataAt<GuiListWidgetUserdata>(index, false);
    OnListItemMouseClicked(static_cast<unsigned int>(index), userdata);
}

void GuiDynamicListPanelBase::ListCallback_ItemHoverChanged    (MyGUI::Widget* list, size_t index)
{
    MyGUI::ListBox* listbox = static_cast<MyGUI::ListBox*>(list);
    // Unfocus previous (if assigned)
    if (m_curr_hovered_item_index != 0xFFFFFFFF)
    {
        GuiListWidgetUserdata* unfocus_userdata = listbox->getItemDataAt<GuiListWidgetUserdata>(m_curr_hovered_item_index, false);
        this->OnListItemMouseFocusLost(m_curr_hovered_item_index, unfocus_userdata);
        m_curr_hovered_item_index = 0xFFFFFFFF;
    }
    if (index == 0xFFFFFFFF) // Focus lost
    {
        return;
    }
    // Focus new
    m_curr_hovered_item_index = index;
    GuiListWidgetUserdata* userdata = listbox->getItemDataAt<GuiListWidgetUserdata>(index, false);
    
    this->OnListItemMouseFocusGained(index, userdata);
    return;
}


