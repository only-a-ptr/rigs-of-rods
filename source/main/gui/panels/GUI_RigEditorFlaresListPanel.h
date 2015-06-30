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

#include "GUI_RigEditorFlaresListPanelLayout.h"

#include "ForwardDeclarations.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RigEditor_IMain.h"
#include "RigEditor_GuiDynamicListPanelBase.h"


namespace RoR
{

namespace GUI
{

class RigEditorFlaresListPanel: public RigEditorFlaresListPanelLayout, public RigEditor::GuiDynamicListPanelBase
{

public:

	RigEditorFlaresListPanel(RigEditor::IMain* rig_editor_interface, RigEditor::Config* config);

    inline void ClearFlaresList() { ClearList(); }

	void UpdateFlaresList(std::vector<RigEditor::Flare*> flares_list);

protected:

    // Overrides from RigEditor::GuiDynamicListPanelBase
    virtual void OnListItemMouseFocusGained   (unsigned int item_index, GuiListWidgetUserdata* userdata);
    virtual void OnListItemMouseFocusLost     (unsigned int item_index, GuiListWidgetUserdata* userdata);
	virtual void OnListItemMouseClicked       (unsigned int item_index, GuiListWidgetUserdata* userdata);

    virtual void OnAddItemMouseClicked        (MyGUI::Button* button);

    virtual void OnSelectByTypeItemHover      (unsigned int item_value);
    virtual void OnSelectByTypeItemClick      (unsigned int item_value);

	virtual void OnSelectAllMouseClicked      (MyGUI::Button* button);
	virtual void OnSelectAllMouseFocusGained  (MyGUI::Button* button);
	virtual void OnSelectAllMouseFocusLost    (MyGUI::Button* button);
	                                                          
	virtual void OnDeselectAllMouseClicked    (MyGUI::Button* button);
	virtual void OnDeselectAllMouseFocusGained(MyGUI::Button* button);
	virtual void OnDeselectAllMouseFocusLost  (MyGUI::Button* button);

    RigEditor::IMain*      m_rig_editor_interface;
    RigEditor::Config*     m_config;
};

} // namespace GUI

} // namespace RoR
