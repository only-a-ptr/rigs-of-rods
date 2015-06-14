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

#include "RigEditor_RigFlareVisuals.h"

#include "RigEditor_Config.h"
#include "RigEditor_HighlightBoxesDynamicMesh.h"
#include "RigEditor_Flare.h"
#include "RigEditor_Main.h"
#include "RigEditor_Node.h"

#include <OgreTechnique.h>
#include <OgreSceneManager.h>
#include <OgreManualObject.h>
#include <vector>

using namespace RoR;
using namespace RigEditor;

void RigFlareVisuals::Init(RigEditor::Main* rig_editor)
{
	// SELECTION + HOVER MESHES
	HighlightBoxesDynamicMesh* dyna_mesh_select = new HighlightBoxesDynamicMesh();
    HighlightBoxesDynamicMesh* dyna_mesh_hover  = new HighlightBoxesDynamicMesh();
	
    dyna_mesh_select->Initialize(rig_editor, rig_editor->GetOgreSceneManager(), "rig-editor-flares-selected-dynamic-mesh", 10);
    dyna_mesh_hover ->Initialize(rig_editor, rig_editor->GetOgreSceneManager(), "rig-editor-flares-hovered-dynamic-mesh",  2);

	m_flares_selected_dynamic_mesh = std::unique_ptr<HighlightBoxesDynamicMesh>(dyna_mesh_select);
	m_flares_hovered_dynamic_mesh  = std::unique_ptr<HighlightBoxesDynamicMesh>(dyna_mesh_hover);
}

void RigFlareVisuals::BuildFlaresGeometryDynamicMesh(
	RigEditor::Main* rig_editor,
	std::vector<Flare*> & flares
	)
{
	assert(rig_editor != nullptr);

	// Prepare material
	if (! Ogre::MaterialManager::getSingleton().resourceExists("rig-editor-skeleton-flares-material"))
	{
		Ogre::MaterialPtr node_mat = static_cast<Ogre::MaterialPtr>(
			Ogre::MaterialManager::getSingleton().create("rig-editor-skeleton-flares-material", 
			Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME)
		);

		node_mat->getTechnique(0)->getPass(0)->createTextureUnitState();
		node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
		node_mat->getTechnique(0)->getPass(0)->getTextureUnitState(0)->setTextureAnisotropy(3);
		node_mat->setLightingEnabled(false);
		node_mat->setReceiveShadows(false);
	}

	// Analyze
	int flares_vertex_count = flares.size() * FlareVisualStar::PER_FLARE_VERTEX_COUNT;
	int flares_index_count  = flares.size() * FlareVisualStar::PER_FLARE_INDEX_COUNT;;

	// Create mesh
	assert(m_flares_dynamic_mesh.get() == nullptr);
	m_flares_dynamic_mesh = std::unique_ptr<Ogre::ManualObject>(
			rig_editor->GetOgreSceneManager()->createManualObject()
		);
	m_flares_dynamic_mesh->estimateVertexCount(flares_vertex_count);
	m_flares_dynamic_mesh->estimateIndexCount(flares_index_count);
	m_flares_dynamic_mesh->setCastShadows(false);
	m_flares_dynamic_mesh->setDynamic(true);
	m_flares_dynamic_mesh->setRenderingDistance(300);

	// Init
	m_flares_dynamic_mesh->begin("rig-editor-skeleton-flares-material", Ogre::RenderOperation::OT_LINE_LIST);

	// Process
    Ogre::ColourValue color = rig_editor->GetConfig()->flare_star_line_color;
    float half_size = rig_editor->GetConfig()->flare_star_half_size;
    GenerateFlaresDynamicMesh(m_flares_dynamic_mesh.get(), flares, color, half_size);
    // Finalize
	m_flares_dynamic_mesh->end();
}

void RigFlareVisuals::GenerateFlaresDynamicMesh(Ogre::ManualObject* dyn_mesh, std::vector<Flare*> & flares, Ogre::ColourValue color, float half_size)
{
    auto flares_end = flares.end();
    for (auto itor = flares.begin(); itor != flares_end; ++itor)
	{
		Flare* flare = *itor;
        FlareVisualStar::AddStar(dyn_mesh, flare->GetVisualPosition(), half_size, color);
	}
}

void RigFlareVisuals::RefreshFlaresDynamicMeshes(
	Ogre::SceneNode* parent_scene_node, 
	RigEditor::Main* rig_editor,
	std::vector<Flare*> & flares
	)
{
	assert(parent_scene_node != nullptr);
	assert(rig_editor != nullptr);

    if (m_flares_dynamic_mesh.get() == nullptr || this->HasFlareCountChanged())
    {
		// Rebuild the flares dynamic mesh
		bool attach_to_scene = false;
		if (m_flares_dynamic_mesh.get() != nullptr)
		{
			attach_to_scene = m_flares_dynamic_mesh->isInScene();
			m_flares_dynamic_mesh->detachFromParent();
			m_flares_dynamic_mesh.reset();
		}
		this->BuildFlaresGeometryDynamicMesh(rig_editor, flares);
		if (attach_to_scene)
		{
			parent_scene_node->attachObject(m_flares_dynamic_mesh.get());
		}
        this->SetHasFlareCountChanged(false);
	}
	else
	{
		// Update existing mesh
		this->UpdateFlaresGeometryDynamicMesh(rig_editor, flares);
	}
	
}

void RigFlareVisuals::UpdateFlaresGeometryDynamicMesh(
		RigEditor::Main* rig_editor,
		std::vector<Flare*> & flares
		)
{
	assert(m_flares_dynamic_mesh.get() != nullptr);
	// Init
	auto dyn_mesh = m_flares_dynamic_mesh.get();
    dyn_mesh->beginUpdate(0);
    float half_size = rig_editor->GetConfig()->flare_star_half_size;
    Ogre::ColourValue color = rig_editor->GetConfig()->flare_star_line_color;

	// Process
	GenerateFlaresDynamicMesh(m_flares_dynamic_mesh.get(), flares, color, half_size);
	
	// Finalize
	m_flares_dynamic_mesh->end();
}

void RigFlareVisuals::AttachToScene(Ogre::SceneNode* parent_scene_node)
{
	assert(parent_scene_node != nullptr);
	if (m_flares_dynamic_mesh.get() != nullptr)
	{
		parent_scene_node->attachObject(m_flares_dynamic_mesh.get());
	}
}

void RigFlareVisuals::DetachFromScene()
{
	if (m_flares_dynamic_mesh.get() != nullptr)
	{
		m_flares_dynamic_mesh->detachFromParent();
	}
}

void RigFlareVisuals::UpdateFlaresSelectionHighlightBoxes(
		std::vector<Flare*> & flares, 
		RigEditor::Main* rig_editor,
		Ogre::SceneNode* parent_scene_node
		)
{
	// Check if any wheel is selected
	auto end = flares.end();
	bool selected_found = false;
	for (auto itor = flares.begin(); itor != end; ++itor)
	{
		Flare* wheel = *itor;
		if (wheel->IsSelected())
		{
			selected_found = true;
			break;
		}
	}
	if (!selected_found)
	{
		m_flares_selected_dynamic_mesh->DetachFromScene();
		this->SetIsSelectionDirty(false);
		return;
	}

	// Update selection highlight
	float size              = rig_editor->GetConfig()->flare_selection_boxes_half_size;
    Ogre::ColourValue color = rig_editor->GetConfig()->flare_selection_boxes_color;

	m_flares_selected_dynamic_mesh->BeginUpdate();
	for (auto itor = flares.begin(); itor != end; ++itor)
	{
		Flare* flare = *itor;
		if (flare->IsSelected())
		{
            Ogre::Vector3 pos(flare->GetReferenceNode()->GetPosition());
            Ogre::Vector3 aabb_max(pos.x + size, pos.y + size, pos.z + size);
            Ogre::Vector3 aabb_min(pos.x - size, pos.y - size, pos.z - size);
			m_flares_selected_dynamic_mesh->AddBox(aabb_max, aabb_min, color);
		}
	}
	m_flares_selected_dynamic_mesh->EndUpdate();
	m_flares_selected_dynamic_mesh->AttachToScene(parent_scene_node);
	this->SetIsSelectionDirty(false);
}

void RigFlareVisuals::UpdateFlaresMouseHoverHighlightBoxes(
		std::vector<Flare*> & flares, 
		RigEditor::Main* rig_editor, 
		Ogre::SceneNode* parent_scene_node
		)
{
	// Check if any wheel is hovered
	auto end = flares.end();
	bool hovered_found = false;
	for (auto itor = flares.begin(); itor != end; ++itor)
	{
		Flare* wheel = *itor;
		if (wheel->IsHovered())
		{
			hovered_found = true;
			break;
		}
	}
	if (!hovered_found)
	{
		m_flares_hovered_dynamic_mesh->DetachFromScene();
		this->SetIsHoverDirty(false);
		return;
	}

	// Update hover highlight
    float size              = rig_editor->GetConfig()->flare_hover_highlight_boxes_half_size;
    Ogre::ColourValue color = rig_editor->GetConfig()->flare_hover_highlight_boxes_color;

	m_flares_hovered_dynamic_mesh->BeginUpdate();
	for (auto itor = flares.begin(); itor != end; ++itor)
	{
		Flare* flare = *itor;
		if (flare->IsSelected())
		{
            Ogre::Vector3 pos(flare->GetReferenceNode()->GetPosition());
            Ogre::Vector3 aabb_max(pos.x + size, pos.y + size, pos.z + size);
            Ogre::Vector3 aabb_min(pos.x - size, pos.y - size, pos.z - size);
			m_flares_hovered_dynamic_mesh->AddBox(aabb_max, aabb_min, color);
		}
	}
	m_flares_hovered_dynamic_mesh->EndUpdate();
	m_flares_hovered_dynamic_mesh->AttachToScene(parent_scene_node);
	this->SetIsHoverDirty(false);
}

void FlareVisualStar::AddLine(Ogre::ManualObject* dyn_mesh, float x1, float y1, float z1, float x2, float y2, float z2, Ogre::ColourValue color)
{
    dyn_mesh->position(x1, y1, z1);
	dyn_mesh->colour(color);
	dyn_mesh->position(x2, y2, z2);
	dyn_mesh->colour(color);
}    

// Param h = half_size
void FlareVisualStar::AddStar(Ogre::ManualObject* dyn_mesh, Ogre::Vector3 position, float h, Ogre::ColourValue color)
{
    float x = position.x;
    float y = position.y;
    float z = position.z;

    // Axes
    AddLine(dyn_mesh, x+h, y, z, x-h, y, z, color);
    AddLine(dyn_mesh, x, y+h, z, x, y-h, z, color);
    AddLine(dyn_mesh, x, y, z+h, x, y, z-h, color);

    // Diagonal
    AddLine(dyn_mesh, x+h, y+h, z+h, x-h, y-h, z-h, color);
    AddLine(dyn_mesh, x+h, y+h, z-h, x-h, y-h, z+h, color);
    AddLine(dyn_mesh, x-h, y+h, z+h, x+h, y-h, z-h, color);
    AddLine(dyn_mesh, x-h, y+h, z-h, x+h, y-h, z+h, color);
}
