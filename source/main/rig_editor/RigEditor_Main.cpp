/*
    This source file is part of Rigs of Rods
    Copyright 2013-2017 Petr Ohlidal & contributors

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
    @file   RigEditor_Main.cpp
    @date   06/2014
    @author Petr Ohlidal
*/

#include "RigEditor_Main.h"

#include "AppContext.h"
#include "Application.h"
#include "CacheSystem.h"
#include "Console.h"
#include "ContentManager.h"
#include "GUI_RigEditorBeamsPanel.h"
#include "GUI_RigEditorCommands2Panel.h"
#include "GUI_RigEditorDeleteMenu.h"
#include "GUI_RigEditorFlexBodyWheelsPanel.h"
#include "GUI_RigEditorHelpWindow.h"
#include "GUI_RigEditorHydrosPanel.h"
#include "GUI_RigEditorLandVehiclePropertiesWindow.h"
#include "GUI_RigEditorMenubar.h"
#include "GUI_RigEditorMeshWheels2Panel.h"
#include "GUI_RigEditorNodePanel.h"
#include "GUI_RigEditorRigPropertiesWindow.h"
#include "GUI_RigEditorShocksPanel.h"
#include "GUI_RigEditorShocks2Panel.h"
#include "GUIManager.h"
#include "InputEngine.h"
#include "PlatformUtils.h"
#include "RigDef_Parser.h"
#include "RigDef_Serializer.h"
#include "RigDef_Validator.h"
#include "RigEditor_CameraHandler.h"
#include "RigEditor_Config.h"
#include "RigEditor_InputHandler.h"
#include "RigEditor_Node.h"
#include "RigEditor_Rig.h"
#include "RigEditor_RigProperties.h"
#include "RigEditor_RigElementsAggregateData.h"
#include "RigEditor_RigWheelsAggregateData.h"

#include <OISKeyboard.h>
#include <OgreEntity.h>
#include <OgreMaterialManager.h>
#include <OgreMaterial.h>
#include <OgreMovableObject.h>
#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <sstream>

using namespace RoR;
using namespace RoR::RigEditor;

Main::Main():
    m_scene_manager(nullptr),
    m_camera(nullptr),
    m_viewport(nullptr),
    m_rig_entity(nullptr),
    m_input_handler(nullptr),
    m_rig(nullptr),
    m_state_flags(0)
{
    // Load resources
    RoR::App::GetContentManager()->AddResourcePack(ContentManager::ResourcePack::ACTOR_EDITOR);

    // Load config file
    m_config = new RigEditor::Config(PathCombine(App::sys_config_dir->GetStr(), "rig_editor.cfg"));

    // Setup 3D engine 
    m_scene_manager = App::GetAppContext()->GetOgreRoot()->createSceneManager(Ogre::ST_GENERIC, "rig_editor_scene_manager");
    m_scene_manager->setAmbientLight(m_config->scene_ambient_light_color);

    // Camera 
    m_camera = m_scene_manager->createCamera("rig_editor_camera");
    m_camera->setNearClipDistance(m_config->camera_near_clip_distance);
    m_camera->setFarClipDistance(m_config->camera_far_clip_distance);
    m_camera->setFOVy(Ogre::Degree(m_config->camera_FOVy_degrees));
    m_camera->setAutoAspectRatio(true);

    // Setup input 
    m_input_handler = new InputHandler();

    // Camera handling 
    m_camera_handler = new CameraHandler(m_camera);
    m_camera_handler->SetOrbitTarget(m_scene_manager->getRootSceneNode());	
    m_camera_handler->SetOrthoZoomRatio(m_config->ortho_camera_zoom_ratio);
    m_camera->setPosition(Ogre::Vector3(10,5,10));

    // Register DearIMGUI for rendering
    m_scene_manager->addRenderQueueListener(&App::GetGuiManager()->GetImGui());
}

Main::~Main()
{
    // GUI objects are cleaned up by std::unique_ptr

    if (m_rig != nullptr)
    {
        m_rig->DetachFromScene();
        delete m_rig;
        m_rig = nullptr;
    }
}

void Main::BringUp()
{
    /* Setup 3D engine */
    m_viewport = App::GetAppContext()->GetRenderWindow()->addViewport(nullptr);
    int viewport_width = m_viewport->getActualWidth();
    m_viewport->setBackgroundColour(m_config->viewport_background_color);
    m_camera->setAspectRatio(m_viewport->getActualHeight() / viewport_width);
    m_viewport->setCamera(m_camera);

    this->InitializeOrRestoreGui();
}

void Main::PutOff()
{
    /* Hide GUI */
    m_gui_menubar->Hide();
    m_gui_delete_menu->Hide();
    // Supress node/beam panels (if visible)
    m_nodes_panel    ->HideTemporarily();
    m_beams_panel    ->HideTemporarily();
    m_hydros_panel   ->HideTemporarily();
    m_commands2_panel->HideTemporarily();
    m_shocks_panel   ->HideTemporarily();
    m_shocks2_panel  ->HideTemporarily();
    m_meshwheels2_panel     ->HideTemporarily();
    m_flexbodywheels_panel  ->HideTemporarily();
}

void Main::UpdateInputEvents(float dt)
{
    /* Update input events */
    m_input_handler->ResetEvents();
    App::GetInputEngine()->Capture(); // Also injects input to GUI (through RigEditor::InputHandler)

    /* Handle key presses */
    m_camera_ortho_toggled = false;
    m_camera_view_changed = false;
    if (m_input_handler->WasEventFired(InputHandler::Event::CAMERA_VIEW_TOGGLE_PERSPECTIVE))
    {
        m_camera_handler->ToggleOrtho();
        m_camera_ortho_toggled = true;
    }

    // Orientation:
    // Front->Back = X axis
    // Right->Left = Z axis
    // Top->Down   = Y axis negative
    // Inspired by:
    // * Gavril MZ2
    // * Tatra-T813-Dakar.truck

    OIS::Keyboard* ois_keyboard = App::GetInputEngine()->GetOisKeyboard();
    bool ctrl_is_down = ois_keyboard->isKeyDown(OIS::KC_RCONTROL) || ois_keyboard->isKeyDown(OIS::KC_LCONTROL);
    if (m_input_handler->WasEventFired(InputHandler::Event::CAMERA_VIEW_FRONT))
    {
        m_camera_handler->LookInDirection(ctrl_is_down ? Ogre::Vector3::NEGATIVE_UNIT_X : Ogre::Vector3::UNIT_X);
        m_camera_view_changed = true;
    }
    if (m_input_handler->WasEventFired(InputHandler::Event::CAMERA_VIEW_SIDE))
    {
        m_camera_handler->LookInDirection(ctrl_is_down ? Ogre::Vector3::UNIT_Z : Ogre::Vector3::NEGATIVE_UNIT_Z);
        m_camera_view_changed = true;
    }
    if (m_input_handler->WasEventFired(InputHandler::Event::CAMERA_VIEW_TOP))
    {
        m_camera_handler->TopView(! ctrl_is_down);
        m_camera_view_changed = true;
    }

    /* Handle camera control */
    
    if (m_input_handler->GetMouseMotionEvent().HasMoved() || m_input_handler->GetMouseMotionEvent().HasScrolled())
    {
        bool res = m_camera_handler->InjectMouseMove(
            m_input_handler->GetMouseButtonEvent().IsRightButtonDown(), /* (bool do_orbit) */
            m_input_handler->GetMouseMotionEvent().rel_x,
            m_input_handler->GetMouseMotionEvent().rel_y,
            m_input_handler->GetMouseMotionEvent().rel_wheel
        );
        if (res)
        {
            m_camera_view_changed = true;
        }
    }
}

void Main::UpdateEditorLoop()
{
    OIS::Keyboard* ois_keyboard = App::GetInputEngine()->GetOisKeyboard();
    bool ctrl_is_down = ois_keyboard->isKeyDown(OIS::KC_RCONTROL) || ois_keyboard->isKeyDown(OIS::KC_LCONTROL);

    // RIG MANIPULATION \\

    if (m_rig != nullptr)
    {
        bool node_selection_changed = false;
        bool node_hover_changed = false;
        bool node_mouse_selecting_disabled = m_gui_delete_menu->IsVisible();// || m_gui_rig_properties_window->IsVisible();
        bool rig_updated = false;
        bool all_nodes_selection_toggled = false;
        bool rig_must_deselect_all_nodes = false;
        RigEditor::Node* node_to_set_selected = nullptr;
        Vector2int mouse_screen_position = m_input_handler->GetMouseMotionEvent().GetAbsolutePosition();

        // Handle event 'extrude selected', because it changes selection and invokes Mode::GRAB_NODES
        // TODO: Make this work with GUI panel updates. It will require implementing array of nodes to set selected.
        if	(	(m_input_handler->WasEventFired(InputHandler::Event::NODES_EXTRUDE_SELECTED))
            &&	(! m_input_handler->IsModeActive(InputHandler::Mode::CREATE_NEW_NODE))
            &&	(! m_input_handler->IsModeActive(InputHandler::Mode::GRAB_NODES))
            )
        {
            m_rig->ExtrudeSelectedNodes();
            m_input_handler->EnterMode(InputHandler::Mode::GRAB_NODES);
        }
        
        if	(	(m_input_handler->WasModeEntered(InputHandler::Mode::CREATE_NEW_NODE))
            ||	(m_input_handler->WasModeEntered(InputHandler::Mode::GRAB_NODES))
            )
        {
            m_rig->ClearMouseHoveredNode();
            node_hover_changed = true;
            node_mouse_selecting_disabled = true;
        }

        // The 'delete menu' dialog
        if (m_input_handler->WasEventFired(InputHandler::Event::GUI_SHOW_DELETE_MENU))
        {
            m_gui_delete_menu->Show();
            m_gui_delete_menu->SetPosition(
                    mouse_screen_position.x + m_config->gui_dialog_delete_placement_x_px,
                    mouse_screen_position.y + m_config->gui_dialog_delete_placement_y_px
                );
        }
        if (m_gui_delete_menu->IsVisible())
        {
            if (! m_gui_delete_menu->TestCursorInRange(mouse_screen_position.x, mouse_screen_position.y, m_config->gui_dialog_delete_cursor_fence_px))
            {
                m_gui_delete_menu->Hide();
            }
        }

        // Deselect/select all nodes
        if	(	(m_input_handler->WasEventFired(InputHandler::Event::NODES_DESELECT_OR_SELECT_ALL))
            &&	(! m_input_handler->IsModeActive(InputHandler::Mode::CREATE_NEW_NODE))
            &&	(! m_input_handler->IsModeActive(InputHandler::Mode::GRAB_NODES))
            )
        {
            all_nodes_selection_toggled = true;
            node_selection_changed = true;
        }

        // Creating new nodes with mouse
        if (m_input_handler->IsModeActive(InputHandler::Mode::CREATE_NEW_NODE))
        {
            node_mouse_selecting_disabled = true;
            if (m_input_handler->GetMouseButtonEvent().WasLeftButtonPressed())
            {
                if (! ctrl_is_down)
                {
                    rig_must_deselect_all_nodes = true;
                    node_selection_changed = true;
                }
                node_to_set_selected = &m_rig->CreateNewNode(
                        m_camera_handler->ConvertScreenToWorldPosition(mouse_screen_position, Ogre::Vector3::ZERO)
                    );
                m_rig->RefreshNodeScreenPosition(*node_to_set_selected, m_camera_handler);
                node_selection_changed = true;
                rig_updated = true;
            }
        }

        // Grabbing nodes with mouse
        if (m_input_handler->IsModeActive(InputHandler::Mode::GRAB_NODES))
        {
            if (m_input_handler->GetMouseMotionEvent().HasMoved())
            {
                // Translate selected nodes
                Ogre::Vector3 mouse_world_pos = m_camera_handler->ConvertScreenToWorldPosition(mouse_screen_position, Ogre::Vector3::ZERO);
                Ogre::Vector3 previous_world_pos = m_camera_handler->ConvertScreenToWorldPosition(
                    m_input_handler->GetMouseMotionEvent().GetPreviousAbsolutePosition(),
                    Ogre::Vector3::ZERO
                );

                m_rig->TranslateSelectedNodes(mouse_world_pos - previous_world_pos, m_camera_handler);
                rig_updated = true;
            }

            if (m_input_handler->GetMouseButtonEvent().WasLeftButtonPressed())
            {
                // Exit grab mode and commit changes
                m_input_handler->ExitMode(InputHandler::Mode::GRAB_NODES);
                m_rig->SelectedNodesCommitPositionUpdates();
            }
            else if (m_input_handler->WasEventFired(InputHandler::Event::ESCAPE))
            {
                // Exit grab mode and revert changes
                m_input_handler->ExitMode(InputHandler::Mode::GRAB_NODES);
                m_rig->SelectedNodesCancelPositionUpdates();
                rig_updated = true;
            }
        }
        

        if (m_camera_view_changed || m_camera_ortho_toggled)
        {
            m_rig->RefreshAllNodesScreenPositions(m_camera_handler);
        }
        
        if	(	(m_input_handler->WasModeExited(InputHandler::Mode::CREATE_NEW_NODE))
            ||	(m_input_handler->WasModeExited(InputHandler::Mode::GRAB_NODES))
            ||	(	(! m_input_handler->IsModeActive(InputHandler::Mode::CREATE_NEW_NODE))
                &&	((m_input_handler->GetMouseMotionEvent().HasMoved() || m_camera_view_changed || m_camera_ortho_toggled))
                )
            )
        {
            if (m_rig->RefreshMouseHoveredNode(mouse_screen_position))
            {
                node_hover_changed = true;
            }
        }

        // Selecting nodes with LMB
        if	(	(! node_mouse_selecting_disabled) 
            &&	(! node_hover_changed) 
            &&	(! m_input_handler->WasModeExited(InputHandler::Mode::GRAB_NODES))
            &&	(m_input_handler->GetMouseButtonEvent().WasLeftButtonPressed())
            )
        {
            if (! ctrl_is_down)
            {
                rig_must_deselect_all_nodes = true;
                node_selection_changed = true;
            }
            if (m_rig->GetMouseHoveredNode() != nullptr)
            {
                node_selection_changed = true;
                node_to_set_selected = m_rig->GetMouseHoveredNode();
            }
        }

        if (node_selection_changed)
        {
            // ==== Apply changes from GUI to rig ====

            if (m_nodes_panel->GetData()->num_selected != 0)
            {
                m_rig->SelectedNodesUpdateAttributes(m_nodes_panel->GetData());
                if (m_rig->GetNumSelectedBeams() != 0)
                {
                    if (m_beams_panel->HasMixedBeamTypes())
                    {
                        MixedBeamsAggregateData data;
                        m_beams_panel->GetMixedBeamsData(&data);
                        m_rig->SelectedMixedBeamsUpdateAttributes(&data);
                    }
                    else
                    {
                        if (m_beams_panel->GetPlainBeamsData()->num_selected != 0)
                        {
                            m_rig->SelectedPlainBeamsUpdateAttributes(m_beams_panel->GetPlainBeamsData());
                        }
                        else if (m_shocks_panel->GetShocksData()->num_selected != 0)
                        {
                            m_rig->SelectedShocksUpdateAttributes(m_shocks_panel->GetShocksData());
                        }
                        else if (m_shocks2_panel->GetShocks2Data()->num_selected != 0)
                        {
                            m_rig->SelectedShocks2UpdateAttributes(m_shocks2_panel->GetShocks2Data());
                        }
                        else if (m_hydros_panel->GetHydrosData()->num_selected != 0)
                        {
                            m_rig->SelectedHydrosUpdateAttributes(m_hydros_panel->GetHydrosData());
                        }
                        else if (m_commands2_panel->GetCommands2Data()->num_selected != 0)
                        {
                            m_rig->SelectedCommands2UpdateAttributes(m_commands2_panel->GetCommands2Data());
                        }
                    }		
                }
            }

            HideAllNodeBeamGuiPanels(); // Reset GUI

            // ==== Perform rig selection updates ====
            if (all_nodes_selection_toggled)
            {
                m_rig->DeselectOrSelectAllNodes();
            }
            if (rig_must_deselect_all_nodes)
            {
                m_rig->DeselectAllNodes();
            }
            if (node_to_set_selected != nullptr)
            {
                node_to_set_selected->SetSelected(true);
            }

            // ==== Query updated data ====

            // Update "nodes" panel
            RigAggregateNodesData query;
            m_rig->QuerySelectedNodesData(&query);

            if (query.num_selected != 0)
            {
                m_nodes_panel->UpdateNodeData(&query);
                m_nodes_panel->Show();
            }

            // Update BEAM panels
            if (query.num_selected >= 2)
            {
                RigAggregateBeams2Data beam_query;
                m_rig->QuerySelectedBeamsData(&beam_query);
                if (beam_query.GetTotalNumSelectedBeams() != 0)
                {
                    if (beam_query.HasMixedBeamTypes())
                    {
                        m_beams_panel->UpdateMixedBeamData(&beam_query);
                        m_beams_panel->Show();
                    }
                    else if (beam_query.plain_beams.num_selected != 0)
                    {
                        m_beams_panel->UpdatePlainBeamData(&beam_query.plain_beams);
                        m_beams_panel->Show();
                    }
                    else if (beam_query.hydros.num_selected != 0)
                    {
                        m_hydros_panel->UpdateHydrosData(&beam_query.hydros);
                        m_hydros_panel->Show();
                    }
                    else if (beam_query.commands2.num_selected != 0)
                    {
                        m_commands2_panel->UpdateCommand2Data(&beam_query.commands2);
                        m_commands2_panel->Show();
                    }
                    else if (beam_query.shocks.num_selected != 0)
                    {
                        m_shocks_panel->UpdateShockData(&beam_query.shocks);
                        m_shocks_panel->Show();
                    }
                    else if (beam_query.shocks2.num_selected != 0)
                    {
                        m_shocks2_panel->UpdateShock2Data(&beam_query.shocks2);
                        m_shocks2_panel->Show();
                    }
                }
            } // if (query.num_selected >= 2)
        } // if (node_selection_changed)

        bool must_refresh_wheels_mesh = false;

        if (this->IsAnyWheelSelectionChangeScheduled())
        {
            // Apply changes from panels
            AllWheelsAggregateData wheel_update;
            wheel_update.flexbodywheels_data = *m_flexbodywheels_panel->GetFlexBodyWheelsData();
            wheel_update.num_elements += wheel_update.flexbodywheels_data.num_elements;
            wheel_update.meshwheels2_data = *m_meshwheels2_panel->GetMeshWheel2Data();
            wheel_update.num_elements += wheel_update.meshwheels2_data.num_elements;
            must_refresh_wheels_mesh = m_rig->UpdateSelectedWheelsData(&wheel_update);

            // Update selection
            const bool any_change = m_rig->PerformScheduledWheelSelectionUpdates(this);
            this->ResetAllScheduledWheelSelectionChanges();

            if (any_change)
            {
                HideAllWheelGuiPanels();
            
                // Query selected wheels data
                AllWheelsAggregateData wheel_query;
                m_rig->QuerySelectedWheelsData(&wheel_query);
                if (wheel_query.num_elements != 0)
                {
                    if(wheel_query.ContainsMultipleWheelTypes())
                    {
                        // To be done.
                    }
                    else if (wheel_query.meshwheels2_data.num_elements != 0)
                    {
                        m_meshwheels2_panel->UpdateMeshWheels2Data(&wheel_query.meshwheels2_data);
                        m_meshwheels2_panel->Show();
                    }
                    else if (wheel_query.flexbodywheels_data.num_elements != 0)
                    {
                        m_flexbodywheels_panel->UpdateFlexBodyWheelsData(&wheel_query.flexbodywheels_data);
                        m_flexbodywheels_panel->Show();
                    }
                }
            }
        }
        else
        {
            AllWheelsAggregateData wheel_update;
            if (m_flexbodywheels_panel->IsImmediateRigUpdateNeeded())
            {
                wheel_update.flexbodywheels_data = *m_flexbodywheels_panel->GetFlexBodyWheelsData();
                wheel_update.num_elements += wheel_update.flexbodywheels_data.num_elements;
                m_flexbodywheels_panel->SetIsImmediateRigUpdateNeeded(false);
            }
            if (m_meshwheels2_panel->IsImmediateRigUpdateNeeded())
            {
                wheel_update.meshwheels2_data = *m_meshwheels2_panel->GetMeshWheel2Data();
                wheel_update.num_elements += wheel_update.meshwheels2_data.num_elements;
                m_meshwheels2_panel->SetIsImmediateRigUpdateNeeded(false);
            }
            if (wheel_update.num_elements != 0)
            {
                must_refresh_wheels_mesh = m_rig->UpdateSelectedWheelsData(&wheel_update);
            }
        }

        // ==== Update visuals ====
        Ogre::SceneNode* parent_scene_node = m_scene_manager->getRootSceneNode();
        if (rig_updated || node_selection_changed || node_hover_changed)
        {
            m_rig->RefreshNodesDynamicMeshes(parent_scene_node);
        }
        if (rig_updated)
        {
            m_rig->RefreshBeamsDynamicMesh();
        }
        bool force_refresh_wheel_selection_boxes = false;
        if (must_refresh_wheels_mesh)
        {
            m_rig->RefreshWheelsDynamicMesh(parent_scene_node, this);
            force_refresh_wheel_selection_boxes = true;
        }
        m_rig->CheckAndRefreshWheelsSelectionHighlights(this, parent_scene_node, force_refresh_wheel_selection_boxes);
        m_rig->CheckAndRefreshWheelsMouseHoverHighlights(this, parent_scene_node);
    }

    // Draw debug box
    ImGui::SetNextWindowPos(ImVec2(100,100));
    ImGui::Begin("rigeditor dbg");
    Str<100> mouse_node;
    if (m_rig != nullptr && m_rig->GetMouseHoveredNode() != nullptr)
    {
        mouse_node << m_rig->GetMouseHoveredNode()->GetId().ToString();
    }
    else
    {
        mouse_node << "[none]";
    }
    ImGui::Text("mouse node: '%s'", mouse_node.ToCStr());
    ImGui::End();
}

void Main::CommandShowDialogOpenRigFile()
{
  //TODO: NotifyFileSelectorEnded()
}

void Main::CommandShowDialogSaveRigFileAs()
{
  //TODO: NotifyFileSelectorEnded()
}

void Main::CommandSaveRigFile()
{
    // TODO
}

void Main::CommandCloseCurrentRig()
{
    if (m_rig != nullptr)
    {
        // Remove rig
        m_rig->DetachFromScene();
        delete m_rig;
        m_rig = nullptr;

        // Restore GUI
        HideAllNodeBeamGuiPanels();
        m_gui_menubar->ClearLandVehicleWheelsList();
    }
}

void Main::NotifyFileSelectorEnded(GUI::Dialog* dialog, bool result)
{
    //TODO: 
    //LoadRigDefFile(folder, filename); 
    //SaveRigDefFile(folder, filename);
}

void Main::SaveRigDefFile(MyGUI::UString const & directory, MyGUI::UString const & filename)
{
    //TODO: m_rig->Export();
}

bool Main::LoadRigDefFile(std::string const & filename, MyGUI::UString const & rg_name)
{
    // Close prev. rig
    if (m_rig != nullptr)
    {
        this->CommandCloseCurrentRig();
    }

    // Load and parse new file
    std::shared_ptr<RigDef::File> def;
    try
    {
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(filename, rg_name);

        RigDef::Parser parser;
        parser.Prepare();
        parser.ProcessOgreStream(stream.get(), RGN_ACTOR_PROJECT);
        parser.Finalize();

        def = parser.GetFile();
    } 
    catch (Ogre::Exception& e)
    {
        // OGRE exception already logged
        App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_ACTOR, Console::CONSOLE_SYSTEM_ERROR,
            "RigEditor: Failed to retrieve rig file" + filename + ", Ogre::Exception was thrown with message: " + e.what());
        return false;
    }

    // validate file
    RigDef::Validator validator;
    validator.Setup(def);
    bool valid = validator.Validate();

    if (! valid)
    {
        // TODO: Report error to user

        LOG("RigEditor: Validating failed!");
        return false;
    }

    // Build the editor mesh
    m_rig = new RigEditor::Rig(m_config);
    RigEditor::RigBuildingReport rig_build_report;
    Ogre::SceneNode* parent_scene_node = m_scene_manager->getRootSceneNode();
    m_rig->Build(def, this, parent_scene_node, &rig_build_report);

    // Display mesh
    this->OnNewRigCreatedOrLoaded(parent_scene_node);

    LOG(Ogre::String("RigEditor: Finished loading rig, report:\n") + rig_build_report.ToString());

    return true;
}

void Main::OnNewRigCreatedOrLoaded(Ogre::SceneNode* parent_scene_node)
{
    m_rig->AttachToScene(parent_scene_node);
    /* Handle mouse selection of nodes */
    m_rig->RefreshAllNodesScreenPositions(m_camera_handler);
    if (m_rig->RefreshMouseHoveredNode(m_input_handler->GetMouseMotionEvent().GetAbsolutePosition()))
    {
        m_rig->RefreshNodesDynamicMeshes(parent_scene_node);
    }
    /* Update GUI */
    m_gui_menubar->ClearLandVehicleWheelsList();
    m_gui_menubar->UpdateLandVehicleWheelsList(m_rig->GetWheels());
}

void Main::CommandCurrentRigDeleteSelectedNodes()
{
    m_gui_delete_menu->Hide();
    HideAllNodeBeamGuiPanels();
    assert(m_rig != nullptr);
    m_rig->DeleteSelectedNodes();
    m_rig->RefreshBeamsDynamicMesh();
    m_rig->RefreshNodesDynamicMeshes(m_scene_manager->getRootSceneNode());
}

void Main::HideAllNodeBeamGuiPanels()
{
    m_nodes_panel    ->HideAndReset();
    m_beams_panel    ->HideAndReset();
    m_hydros_panel   ->HideAndReset();
    m_commands2_panel->HideAndReset();
    m_shocks2_panel  ->HideAndReset();
    m_shocks_panel   ->HideAndReset();
}

void Main::CommandCurrentRigDeleteSelectedBeams()
{
    m_gui_delete_menu->Hide();
    HideAllNodeBeamGuiPanels();
    assert(m_rig != nullptr);
    m_rig->DeleteBeamsBetweenSelectedNodes();
    m_rig->RefreshBeamsDynamicMesh();
    m_rig->RefreshNodesDynamicMeshes(m_scene_manager->getRootSceneNode());
}

void Main::CommandShowRigPropertiesWindow()
{
    if (m_rig != nullptr)
    {
        m_gui_rig_properties_window->Import(m_rig->GetProperties());
        m_gui_rig_properties_window->Show();
        m_gui_rig_properties_window->CenterToScreen();
    }
}

void Main::CommandSaveContentOfRigPropertiesWindow()
{
    if (m_rig != nullptr && m_gui_rig_properties_window->IsVisible())
    {
        m_gui_rig_properties_window->Export(m_rig->GetProperties());
    }
}

void Main::CommandShowLandVehiclePropertiesWindow()
{
    if (m_rig != nullptr)
    {
        m_gui_land_vehicle_properties_window->Import(
            m_rig->GetProperties()->GetEngine(),
            m_rig->GetProperties()->GetEngoption()
            );
        m_gui_land_vehicle_properties_window->Show();
        m_gui_land_vehicle_properties_window->CenterToScreen();		
    }
}

void Main::CommandSaveLandVehiclePropertiesWindowData()
{
    if (m_rig != nullptr && m_gui_land_vehicle_properties_window->IsVisible())
    {
        m_rig->GetProperties()->SetEngine(m_gui_land_vehicle_properties_window->ExportEngine());
        m_rig->GetProperties()->SetEngoption(m_gui_land_vehicle_properties_window->ExportEngoption());
    }
}

void Main::CommandShowHelpWindow()
{
    m_gui_help_window->Show();
    m_gui_help_window->CenterToScreen();
}

#define INIT_OR_RESTORE_RIG_ELEMENT_PANEL(VAR, CLASSNAME) \
    if ((VAR).get() == nullptr) \
        (VAR) = std::unique_ptr<GUI::CLASSNAME>(new GUI::CLASSNAME(this, m_config)); \
    else \
        (VAR)->ShowIfHiddenTemporarily();

void Main::InitializeOrRestoreGui()
{
    INIT_OR_RESTORE_RIG_ELEMENT_PANEL( m_nodes_panel,            RigEditorNodePanel);
    INIT_OR_RESTORE_RIG_ELEMENT_PANEL( m_beams_panel,            RigEditorBeamsPanel);
    INIT_OR_RESTORE_RIG_ELEMENT_PANEL( m_hydros_panel,           RigEditorHydrosPanel);
    INIT_OR_RESTORE_RIG_ELEMENT_PANEL( m_commands2_panel,        RigEditorCommands2Panel);
    INIT_OR_RESTORE_RIG_ELEMENT_PANEL( m_shocks_panel,           RigEditorShocksPanel);
    INIT_OR_RESTORE_RIG_ELEMENT_PANEL( m_shocks2_panel,          RigEditorShocks2Panel);
    INIT_OR_RESTORE_RIG_ELEMENT_PANEL( m_meshwheels2_panel,      RigEditorMeshWheels2Panel);
    INIT_OR_RESTORE_RIG_ELEMENT_PANEL( m_flexbodywheels_panel,   RigEditorFlexBodyWheelsPanel);

    App::GetGuiManager()->SetSceneManagerForMyGuiRendering(m_scene_manager);
    if (m_gui_menubar.get() == nullptr)
    {
        m_gui_menubar = std::unique_ptr<GUI::RigEditorMenubar>(new GUI::RigEditorMenubar(this));
    }
    else
    {
        m_gui_menubar->Show();
    }
    m_gui_menubar->StretchWidthToScreen();

    if (m_gui_delete_menu.get() == nullptr)
    {
        m_gui_delete_menu = std::unique_ptr<GUI::RigEditorDeleteMenu>(new GUI::RigEditorDeleteMenu(this));
    }
    if (m_gui_rig_properties_window.get() == nullptr)
    {
        m_gui_rig_properties_window 
            = std::unique_ptr<GUI::RigEditorRigPropertiesWindow>(new GUI::RigEditorRigPropertiesWindow(this));
    }
    if (m_gui_land_vehicle_properties_window.get() == nullptr)
    {
        m_gui_land_vehicle_properties_window 
            = std::unique_ptr<GUI::RigEditorLandVehiclePropertiesWindow>(new GUI::RigEditorLandVehiclePropertiesWindow(this));
    }
    if (m_gui_help_window.get() == nullptr)
    {
        m_gui_help_window = std::unique_ptr<GUI::RigEditorHelpWindow>(new GUI::RigEditorHelpWindow(this));
    }
}

void Main::HideAllWheelGuiPanels()
{
    m_meshwheels2_panel->Hide();
    m_flexbodywheels_panel->Hide();
}

// ----------------------------------------------------------------------------
// Rig Updaters
// ----------------------------------------------------------------------------

void Main::CommandRigSelectedNodesUpdateAttributes(const RigAggregateNodesData* data)
{
    m_rig->SelectedNodesUpdateAttributes(data);
}

void Main::CommandRigSelectedPlainBeamsUpdateAttributes(const RigAggregatePlainBeamsData* data)
{
    m_rig->SelectedPlainBeamsUpdateAttributes(data);
}

void Main::CommandRigSelectedShocksUpdateAttributes(const RigAggregateShocksData*     data)
{
    m_rig->SelectedShocksUpdateAttributes(data);
}

void Main::CommandRigSelectedShocks2UpdateAttributes(const RigAggregateShocks2Data*    data)
{
    m_rig->SelectedShocks2UpdateAttributes(data);
}

void Main::CommandRigSelectedHydrosUpdateAttributes(const RigAggregateHydrosData*     data)
{
    m_rig->SelectedHydrosUpdateAttributes(data);
}

void Main::CommandRigSelectedCommands2UpdateAttributes(const RigAggregateCommands2Data*  data)
{
    m_rig->SelectedCommands2UpdateAttributes(data);
}

void Main::CommandScheduleSetWheelSelected(LandVehicleWheel* wheel_ptr, int wheel_index, bool state_selected)
{
    if (m_rig == nullptr)
    {
        return;
    }
    const bool selection_changes = m_rig->ScheduleSetWheelSelected(wheel_ptr, wheel_index, state_selected, this);
    if (selection_changes)
    {
        if (state_selected)
        {
            this->SetIsSelectWheelScheduled(true);
            this->SetIsDeselectWheelScheduled(false);
        }
        else
        {
            this->SetIsSelectWheelScheduled(false);
            this->SetIsDeselectWheelScheduled(true);
        }
    }
}

void Main::CommandSetWheelHovered (LandVehicleWheel* wheel_ptr, int wheel_index, bool state_hovered)
{
    if (m_rig == nullptr)
    {
        return;
    }
    m_rig->SetWheelHovered(wheel_ptr, wheel_index, state_hovered, this);
}

void Main::CommandScheduleSetAllWheelsSelected(bool state_selected)
{
    if (m_rig == nullptr)
    {
        return;
    }
    if (m_rig->ScheduleSetAllWheelsSelected(state_selected, this))
    {
        if (state_selected)
        {
            this->SetIsSelectAllWheelsScheduled(true);
            this->SetIsDeselectAllWheelsScheduled(false);        
        }
        else
        {
            this->SetIsSelectAllWheelsScheduled(false);
            this->SetIsDeselectAllWheelsScheduled(true);
        }
    }
}

void Main::CommandSetAllWheelsHovered(bool state_hovered)
{
    if (m_rig != nullptr)
    {
        m_rig->SetAllWheelsHovered(state_hovered, this);
    }
}

// Utility macros for creating fresh empty rig + initial cube
#define ADD_NODE(MODULENAME, NODENAME, X, Y, Z) \
{ \
    RigDef::Node n; \
    n.id.SetStr(NODENAME); \
    n.position = Ogre::Vector3(X, Y, Z); \
    MODULENAME->nodes.push_back(n); \
}
#define LINK_NODES(MODULE, NODE1, NODE2)\
{\
    RigDef::Beam b;\
    unsigned flags = RigDef::Node::Ref::REGULAR_STATE_IS_VALID | RigDef::Node::Ref::REGULAR_STATE_IS_NAMED; \
    b.nodes[0] = RigDef::Node::Ref(NODE1, 0u, flags, 0);\
    b.nodes[1] = RigDef::Node::Ref(NODE2, 0u, flags, 0);\
    MODULE->beams.push_back(b);\
}

void Main::CommandCreateNewEmptyRig()
{
    if (m_rig != nullptr)
    {
        return;
    }

    m_rig = new RigEditor::Rig(m_config);
    // Create definition
    auto def = std::shared_ptr<RigDef::File>(new RigDef::File());
    def->name = "Unnamed rig (created in editor)";
    auto module = std::shared_ptr<RigDef::File::Module>(new RigDef::File::Module("_Root_"));
    def->root_module = module;
    // Create special node 0 in _Root_ module
    RigDef::Node node_0;
    node_0.id.SetStr("ZERO");
    node_0.position = Ogre::Vector3::ZERO;
    module->nodes.push_back(node_0);
    // Create cube around node 0
    float size = m_config->new_rig_initial_box_half_size;
    ADD_NODE(module, "CUBE_bottom_oo", -size, -size, -size);
    ADD_NODE(module, "CUBE_bottom_xo",  size, -size, -size);
    ADD_NODE(module, "CUBE_bottom_ox", -size, -size,  size);
    ADD_NODE(module, "CUBE_bottom_xx",  size, -size,  size);
    ADD_NODE(module, "CUBE_top_oo",    -size,  size, -size);
    ADD_NODE(module, "CUBE_top_xo",     size,  size, -size);
    ADD_NODE(module, "CUBE_top_ox",    -size,  size,  size);
    ADD_NODE(module, "CUBE_top_xx",     size,  size,  size);
    // Link nodes                         
    LINK_NODES(module, "CUBE_bottom_oo", "CUBE_bottom_xo"); // Bottom plane...
    LINK_NODES(module, "CUBE_bottom_xo", "CUBE_bottom_xx");
    LINK_NODES(module, "CUBE_bottom_xx", "CUBE_bottom_ox");
    LINK_NODES(module, "CUBE_bottom_ox", "CUBE_bottom_oo");
    LINK_NODES(module, "CUBE_top_oo", "CUBE_top_xo"); // Top plane...
    LINK_NODES(module, "CUBE_top_xo", "CUBE_top_xx");
    LINK_NODES(module, "CUBE_top_xx", "CUBE_top_ox");
    LINK_NODES(module, "CUBE_top_ox", "CUBE_top_oo");
    LINK_NODES(module, "CUBE_bottom_oo", "CUBE_top_oo"); // Top to bottom...
    LINK_NODES(module, "CUBE_bottom_xo", "CUBE_top_xo");
    LINK_NODES(module, "CUBE_bottom_ox", "CUBE_top_ox");
    LINK_NODES(module, "CUBE_bottom_xx", "CUBE_top_xx");
    // Link node 0
    LINK_NODES(module, "CUBE_bottom_oo", "ZERO" );
    LINK_NODES(module, "CUBE_bottom_xo", "ZERO" );
    LINK_NODES(module, "CUBE_bottom_ox", "ZERO" );
    LINK_NODES(module, "CUBE_bottom_xx", "ZERO" );
    LINK_NODES(module, "CUBE_top_oo"   , "ZERO" );
    LINK_NODES(module, "CUBE_top_xo"   , "ZERO" );
    LINK_NODES(module, "CUBE_top_ox"   , "ZERO" );
    LINK_NODES(module, "CUBE_top_xx"   , "ZERO" );
    // Build
    m_rig->Build(def, this, this->m_scene_manager->getRootSceneNode(), nullptr);
    // Accomodate
    this->OnNewRigCreatedOrLoaded(m_scene_manager->getRootSceneNode());
}

