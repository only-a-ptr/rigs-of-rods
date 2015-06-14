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

#include "ConfigFile.h"
#include "GUI_OpenSaveFileDialog.h"
#include "RigDef_Prerequisites.h"
#include "RigEditor_IMain.h"
#include "RigEditor_ForwardDeclarations.h"
#include "RoRPrerequisites.h"

namespace RoR
{

namespace RigEditor
{

struct FlareVisualStar
{
    static const int PER_FLARE_VERTEX_COUNT = 14;
    static const int PER_FLARE_INDEX_COUNT  = 14;
    
    static void AddStar(Ogre::ManualObject* dyn_mesh, Ogre::Vector3 position, float half_size, Ogre::ColourValue color);
    static void AddLine(Ogre::ManualObject* dyn_mesh, float x1, float y1, float z1, float x2, float y2, float z2, Ogre::ColourValue color);
};

class RigFlareVisuals
{
public:
	RigFlareVisuals():
		m_flags(0)
	{}

	void Init(RigEditor::Main* rig_editor);

	// Visibility

	void AttachToScene(Ogre::SceneNode* parent_scene_node);
	void DetachFromScene();

	// Flare visualization

	/// Updates or rebuilds dyn. mesh as needed.
	void RefreshFlaresDynamicMeshes(
		Ogre::SceneNode* parent_scene_node, 
		RigEditor::Main* rig_editor,
		std::vector<Flare*> & flares
		);

	// Selection/hover highlights

	void UpdateFlaresSelectionHighlightBoxes(
			std::vector<Flare*> & flares, 
			RigEditor::Main* rig_editor, 
			Ogre::SceneNode* parent_scene_node
		);
	void UpdateFlaresMouseHoverHighlightBoxes(
			std::vector<Flare*> & flares, 
			RigEditor::Main* rig_editor, 
			Ogre::SceneNode* parent_scene_node
		);

	BITMASK_PROPERTY(m_flags,  1,   FLAG_IS_HOVER_DIRTY,       IsHoverDirty,           SetIsHoverDirty)
	BITMASK_PROPERTY(m_flags,  2,   FLAG_IS_SELECTION_DIRTY,   IsSelectionDirty,       SetIsSelectionDirty)
    BITMASK_PROPERTY(m_flags,  3,   FLAG_FLARE_COUNT_CHANGED,  HasFlareCountChanged,   SetHasFlareCountChanged)

protected:

	/// Forces rebuild of dynamic mesh
	void BuildFlaresGeometryDynamicMesh(
		RigEditor::Main* rig_editor,
		std::vector<Flare*> & flares
		);
	/// Forces update of dynamic mesh
	void UpdateFlaresGeometryDynamicMesh(
		RigEditor::Main* rig_editor, 
		std::vector<Flare*> & flares
		);

    static void GenerateFlaresDynamicMesh(
        Ogre::ManualObject* dyn_mesh, 
        std::vector<Flare*> & flares, 
        Ogre::ColourValue color, 
        float half_size
        );

	std::unique_ptr<Ogre::ManualObject>         m_flares_dynamic_mesh;
	std::unique_ptr<HighlightBoxesDynamicMesh>  m_flares_selected_dynamic_mesh;
	std::unique_ptr<HighlightBoxesDynamicMesh>  m_flares_hovered_dynamic_mesh;
	unsigned int                                m_flags;
};

} // namespace RigEditor

} // namespace RoR
