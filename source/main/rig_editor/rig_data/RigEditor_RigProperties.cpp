/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

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
    @file   RigEditor_RigProperties.cpp
    @date   09/2014
    @author Petr Ohlidal
*/

#include "RigDef_File.h"
#include "RigEditor_RigProperties.h"

namespace RoR {
namespace RigEditor {

struct RigModuleData
{
    std::string                                module_name;
    std::string                                submesh_groundmodel;

    // LAND VEHICLE window
    std::shared_ptr<RigDef::Engine>            engine;
    std::shared_ptr<RigDef::Engoption>         engoption;

    // STUBS (data only stored and exported, no editing)

    std::vector    <RigDef::Airbrake>          airbrakes;
    std::vector    <RigDef::Animator>          animators;
    std::shared_ptr<RigDef::AntiLockBrakes>    anti_lock_brakes;
    std::vector    <RigDef::Axle>              axles;
    std::shared_ptr<RigDef::Brakes>            brakes;
    std::vector    <RigDef::Camera>            cameras;
    std::vector    <RigDef::CameraRail>        camera_rails;
    std::vector    <RigDef::CollisionBox>      collision_boxes;
    std::shared_ptr<RigDef::CruiseControl>     cruise_control;
    std::vector    <RigDef::Node::Ref>         contacters;
    std::shared_ptr<RigDef::Engturbo>          engturbo;
    std::vector    <RigDef::Exhaust>           exhausts;
    std::vector    <RigDef::Node::Ref>         fixes;
    std::vector    <RigDef::Fusedrag>          fusedrag;
    std::vector    <RigDef::Hook>              hooks;
    std::vector    <RigDef::Lockgroup>         lockgroups;
    std::vector    <RigDef::ManagedMaterial>       managed_mats;
    std::vector    <RigDef::MaterialFlareBinding>  mat_flare_bindings;
    std::vector    <RigDef::NodeCollision>         node_collisions;
    std::vector    <RigDef::Particle>          particles;
    std::vector    <RigDef::Pistonprop>        pistonprops;
    std::vector    <RigDef::Prop>              props;
    std::vector    <RigDef::RailGroup>         railgroups;
    std::vector    <RigDef::Ropable>           ropables;
    std::vector    <RigDef::Rotator>           rotators;
    std::vector    <RigDef::Rotator2>          rotators_2;
    std::vector    <RigDef::SlideNode>         slidenodes;
    std::shared_ptr<RigDef::SlopeBrake>        slope_brake;
    std::vector    <RigDef::SoundSource>       soundsources;
    std::vector    <RigDef::SoundSource2>      soundsources_2;
    std::shared_ptr<RigDef::SpeedLimiter>      speed_limiter;
    std::vector    <RigDef::Submesh>           submeshes;
    std::shared_ptr<RigDef::TorqueCurve>       torque_curve;
    std::shared_ptr<RigDef::TractionControl>   traction_control;
    std::vector    <RigDef::Turbojet>          turbojets;
    std::vector    <RigDef::Turboprop2>        turboprops_2;
    std::vector    <RigDef::VideoCamera>       videocameras;
    std::vector    <RigDef::Wing>              wings;

    std::vector<std::shared_ptr<RigDef::Flexbody>>  flexbodies;

    void BuildFromModule(std::shared_ptr<RigDef::File::Module> m)
    {
        module_name         = m->name;

        // Engine + Engoption
        engine              = m->engine;
        engoption           = m->engoption;
                            
        // Stubs            
        engturbo            = m->engturbo;
        traction_control    = m->traction_control;
        torque_curve        = m->torque_curve;
        speed_limiter       = m->speed_limiter;
        slope_brake         = m->slope_brake;
        submesh_groundmodel = m->submeshes_ground_model_name;
        anti_lock_brakes    = m->anti_lock_brakes;
        brakes              = m->brakes;
        cruise_control      = m->cruise_control;
        
        rotators       .assign( m->rotators      .begin(),   m->rotators       .end() );
        rotators_2     .assign( m->rotators_2    .begin(),   m->rotators_2     .end() );
        wings          .assign( m->wings         .begin(),   m->wings          .end() );
        videocameras   .assign( m->videocameras  .begin(),   m->videocameras   .end() );
        turboprops_2   .assign( m->turboprops_2  .begin(),   m->turboprops_2   .end() );
        pistonprops    .assign( m->pistonprops   .begin(),   m->pistonprops    .end() );
        turbojets      .assign( m->turbojets     .begin(),   m->turbojets      .end() );
        soundsources   .assign( m->soundsources  .begin(),   m->soundsources   .end() );
        soundsources_2 .assign( m->soundsources2 .begin(),   m->soundsources2  .end() );
        slidenodes     .assign( m->slidenodes    .begin(),   m->slidenodes     .end() );
        submeshes      .assign( m->submeshes     .begin(),   m->submeshes      .end() );
        ropables       .assign( m->ropables      .begin(),   m->ropables       .end() );
        particles      .assign( m->particles     .begin(),   m->particles      .end() );
        airbrakes      .assign( m->airbrakes     .begin(),   m->airbrakes      .end() );
        animators      .assign( m->animators     .begin(),   m->animators      .end() );
        axles          .assign( m->axles         .begin(),   m->axles          .end() );
        cameras        .assign( m->cameras       .begin(),   m->cameras        .end() );
        camera_rails   .assign( m->camera_rails  .begin(),   m->camera_rails   .end() );
        collision_boxes.assign( m->collision_boxes.begin(),  m->collision_boxes.end() );
        contacters     .assign( m->contacters    .begin(),   m->contacters     .end() );
        exhausts       .assign( m->exhausts      .begin(),   m->exhausts       .end() );
        fixes          .assign( m->fixes         .begin(),   m->fixes          .end() );
        flexbodies     .assign( m->flexbodies    .begin(),   m->flexbodies     .end() );
        fusedrag       .assign( m->fusedrag      .begin(),   m->fusedrag       .end() );
        hooks          .assign( m->hooks         .begin(),   m->hooks          .end() );
        lockgroups     .assign( m->lockgroups    .begin(),   m->lockgroups     .end() );
        props          .assign( m->props         .begin(),   m->props          .end() );
        railgroups     .assign( m->railgroups    .begin(),   m->railgroups     .end() );

        managed_mats      .assign( m->managed_materials      .begin(),  m->managed_materials      .end() );
        mat_flare_bindings.assign( m->material_flare_bindings.begin(),  m->material_flare_bindings.end() );
        node_collisions   .assign( m->node_collisions        .begin(),  m->node_collisions        .end() );

        //COPYPASTE
        //               .assign( m-      .begin(),   m-       .end() );
    }

    void ExportToModule(std::shared_ptr<RigDef::File::Module> m)
    {
        // TODO!
    }
};


RigProperties::RigProperties():
    m_root_data(nullptr),
    m_hide_in_chooser(false),
    m_forward_commands(false),
    m_import_commands(false),
    m_is_rescuer(false),
    m_disable_default_sounds(false),
    m_globals_dry_mass(0.f), // This is a default
    m_globals_load_mass(0.f), // This is a default
    m_minimass(0.f), // This is a default
    m_enable_advanced_deform(true),
    m_rollon(false)
{}

RigProperties::~RigProperties()
{}

void RigProperties::Import(std::shared_ptr<RigDef::File> def_file)
{
    m_title                      = def_file->name;
    m_guid                       = def_file->guid;
    m_hide_in_chooser            = def_file->hide_in_chooser;
    m_forward_commands           = def_file->forward_commands;
    m_import_commands            = def_file->import_commands;
    m_is_rescuer                 = def_file->rescuer;
    m_rollon                     = def_file->rollon;
    m_minimass                   = def_file->minimum_mass;
    m_enable_advanced_deform     = def_file->enable_advanced_deformation;
    m_disable_default_sounds     = def_file->disable_default_sounds;
    m_slidenodes_connect_instant = def_file->slide_nodes_connect_instantly;
    m_collision_range            = def_file->collision_range;
    m_collision_range_defined    = def_file->_collision_range_set;
    m_lockgroup_default_nolock   = def_file->lockgroup_default_nolock;

    auto desc_end = def_file->description.end();
    for (auto itor = def_file->description.begin(); itor != desc_end; ++itor )
    {
        m_description.push_back(*itor);
    }

    auto authors_end = def_file->authors.end();
    for (auto itor = def_file->authors.begin(); itor != authors_end; ++itor )
    {
        m_authors.push_back(*itor);
    }

    RigDef::Fileinfo* fileinfo = def_file->file_info.get();
    if (fileinfo != nullptr)
    {
        m_fileinfo = *fileinfo;
    }

    RigDef::ExtCamera* ext_camera = def_file->root_module->ext_camera.get();
    if (ext_camera != nullptr)
    {
        m_extcamera = *ext_camera;
    }
    
    RigDef::Globals* globals = def_file->root_module->globals.get();
    if (globals != nullptr)
    {
        m_globals_cab_material_name = globals->material_name;
        m_globals_dry_mass          = globals->dry_mass;
        m_globals_load_mass         = globals->cargo_mass;
    }
    
    m_skeleton_settings  = def_file->root_module->skeleton_settings;

    if (m_root_data != nullptr) { delete m_root_data; m_root_data = nullptr; }
    if (m_root_data == nullptr) { m_root_data = new RigModuleData(); }

    m_root_data->BuildFromModule(def_file->root_module);
}

void RigProperties::Export(std::shared_ptr<RigDef::File> def_file)
{
    def_file->name                          = m_title;
    def_file->guid                          = m_guid;
    def_file->hide_in_chooser               = m_hide_in_chooser;
    def_file->forward_commands              = m_forward_commands;
    def_file->import_commands               = m_import_commands;
    def_file->rescuer                       = m_is_rescuer;
    def_file->rollon                        = m_rollon;
    def_file->minimum_mass                  = m_minimass;
    def_file->enable_advanced_deformation   = m_enable_advanced_deform;
    def_file->disable_default_sounds        = m_disable_default_sounds;
    def_file->slide_nodes_connect_instantly = m_slidenodes_connect_instant;
    def_file->collision_range               = m_collision_range;
    def_file->_collision_range_set          = m_collision_range_defined;
    def_file->lockgroup_default_nolock      = m_lockgroup_default_nolock;

    def_file->file_info 
        = std::shared_ptr<RigDef::Fileinfo>(new RigDef::Fileinfo(m_fileinfo));
    def_file->root_module->ext_camera 
        = std::shared_ptr<RigDef::ExtCamera>(new RigDef::ExtCamera(m_extcamera));
    def_file->root_module->skeleton_settings = m_skeleton_settings;

    // Globals
    RigDef::Globals globals;
    globals.cargo_mass               = m_globals_load_mass;
    globals.dry_mass                 = m_globals_dry_mass;
    globals.material_name            = m_globals_cab_material_name;
    def_file->root_module->globals   = std::shared_ptr<RigDef::Globals>(new RigDef::Globals(globals));

    // Description
    for (auto itor = m_description.begin(); itor != m_description.end(); ++itor)
    {
        def_file->description.push_back(*itor);
    }

    // Authors
    for (auto itor = m_authors.begin(); itor != m_authors.end(); ++itor)
    {
        def_file->authors.push_back(*itor);
    }

    m_root_data->ExportToModule(def_file->root_module);
}

std::shared_ptr<RigDef::Engine>    RigProperties::GetEngine()          { return m_root_data->engine; }
std::shared_ptr<RigDef::Engoption> RigProperties::GetEngoption()       { return m_root_data->engoption; }

void  RigProperties::SetEngine     (std::shared_ptr<RigDef::Engine> engine)        { m_root_data->engine      = engine;     }
void  RigProperties::SetEngoption  (std::shared_ptr<RigDef::Engoption> engoption)  { m_root_data->engoption   = engoption;  }

} // namespace RigEditor
} // namespace RoR