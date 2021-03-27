/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2020 Petr Ohlidal

    For more information, see http://www.rigsofrods.org/

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

/// @file
/// @author Petr Ohlidal
/// @date   12/2013

#include "TruckParser.h"

#include "Application.h"
#include "SimConstants.h"
#include "CacheSystem.h"
#include "Console.h"
#include "TruckRegexes.h"
#include "Utils.h"

#include <fmt/format.h>
#include <OgreException.h>
#include <OgreString.h>
#include <OgreStringVector.h>
#include <OgreStringConverter.h>
#include <regex>

using namespace RoR;
using namespace Truck;

const std::regex IDENTIFY_KEYWORD_IGNORE_CASE( IDENTIFY_KEYWORD_REGEX_STRING, std::regex::ECMAScript | std::regex::icase);

inline bool IsWhitespace(char c)
{
    return (c == ' ') || (c == '\t');
}

inline bool IsSeparator(char c)
{
    return IsWhitespace(c) || (c == ':') || (c == '|') || (c == ',');
}

inline bool StrEqualsNocase(std::string const & s1, std::string const & s2)
{
    if (s1.size() != s2.size()) { return false; }
    for (size_t i = 0; i < s1.size(); ++i)
    {
        if (tolower(s1[i]) != tolower(s2[i])) { return false; }
    }
    return true;
}

#define STR_PARSE_INT(_STR_)  Ogre::StringConverter::parseInt(_STR_)

#define STR_PARSE_REAL(_STR_) Ogre::StringConverter::parseReal(_STR_)

#define STR_PARSE_BOOL(_STR_) Ogre::StringConverter::parseBool(_STR_)

void Parser::ProcessCurrentLine()
{
    if (m_in_block_comment)
    {
        if (StrEqualsNocase(m_current_line, "end_comment"))
        {
            m_in_block_comment = false;
        }
        return;
    }
    else if (StrEqualsNocase(m_current_line, "comment"))
    {
        m_in_block_comment = true;
        return;
    }
    else if (m_in_description_section) // Enter logic is below in 'keywords'
    {
        if (StrEqualsNocase(m_current_line, "end_description"))
        {
            m_in_description_section = false;
        }
        else
        {
            m_current_module->description.push_back(m_current_line);
        }
        return;
    }
    else if (StrEqualsNocase(m_current_line, "comment"))
    {
        m_in_block_comment = true;
        return;
    }
    else if ((m_current_line[0] == ';') || (m_current_line[0] == '/'))
    {
        this->ProcessCommentLine();
        return;
    }

    this->TokenizeCurrentLine();

    // Detect keywords on current line 
    Keyword cur_keyword = IdentifyKeywordInCurrentLine();

    // Keep track of current element type
    if (cur_keyword != Keyword::KEYWORD_INVALID)
    {
        m_last_keyword = cur_keyword;
    }

    switch (cur_keyword)
    {
        case KEYWORD_INVALID: break; // No new section  - carry on with processing data

        case KEYWORD_ADD_ANIMATION:            this->ParseDirectiveAddAnimation();                  return;
        case KEYWORD_AIRBRAKES:                this->ChangeSection(SECTION_AIRBRAKES);        return;
        case KEYWORD_ANIMATORS:                this->ChangeSection(SECTION_ANIMATORS);        return;
        case KEYWORD_ANTI_LOCK_BRAKES:         this->ParseAntiLockBrakes();                         return;
        case KEYWORD_AXLES:                    this->ChangeSection(SECTION_AXLES);            return;
        case KEYWORD_AUTHOR:                   this->ParseAuthor();                                 return;
        case KEYWORD_BACKMESH:                 this->ParseDirectiveBackmesh();                      return;
        case KEYWORD_BEAMS:                    this->ChangeSection(SECTION_BEAMS);            return;
        case KEYWORD_BRAKES:                   this->ChangeSection(SECTION_BRAKES);           return;
        case KEYWORD_CAB:                      this->ProcessKeywordCab();                           return;
        case KEYWORD_CAMERAS:                  this->ChangeSection(SECTION_CAMERAS);          return;
        case KEYWORD_CAMERARAIL:               this->ChangeSection(SECTION_CAMERA_RAIL);      return;
        case KEYWORD_CINECAM:                  this->ChangeSection(SECTION_CINECAM);          return;
        case KEYWORD_COLLISIONBOXES:           this->ChangeSection(SECTION_COLLISION_BOXES);  return;
        case KEYWORD_COMMANDS:                 this->ChangeSection(SECTION_COMMANDS);         return;
        case KEYWORD_COMMANDS2:                this->ChangeSection(SECTION_COMMANDS_2);       return;
        case KEYWORD_CONTACTERS:               this->ChangeSection(SECTION_CONTACTERS);       return;
        case KEYWORD_CRUISECONTROL:            this->ParseCruiseControl();                          return;
        case KEYWORD_DESCRIPTION:              this->ChangeSection(SECTION_NONE); m_in_description_section = true; return;
        case KEYWORD_DETACHER_GROUP:           this->ParseDirectiveDetacherGroup();                 return;
        case KEYWORD_DISABLEDEFAULTSOUNDS:     m_current_module->sequence.push_back(SeqElement(m_last_keyword, -1));               return;
        case KEYWORD_ENABLE_ADVANCED_DEFORMATION:   m_current_module->sequence.push_back(SeqElement(m_last_keyword, -1));          return;
        case KEYWORD_END:                      this->ChangeSection(SECTION_NONE);             return;
        case KEYWORD_END_SECTION:              this->EndModule();              return;
        case KEYWORD_ENGINE:                   this->ChangeSection(SECTION_ENGINE);           return;
        case KEYWORD_ENGOPTION:                this->ChangeSection(SECTION_ENGOPTION);        return;
        case KEYWORD_ENGTURBO:                 this->ChangeSection(SECTION_ENGTURBO);         return;
        case KEYWORD_ENVMAP:                   /* Ignored */                                        return;
        case KEYWORD_EXHAUSTS:                 this->ChangeSection(SECTION_EXHAUSTS);         return;
        case KEYWORD_EXTCAMERA:                this->ParseExtCamera();                              return;
        case KEYWORD_FILEFORMATVERSION:        this->ParseFileFormatVersion();                      return;
        case KEYWORD_FILEINFO:                 this->ParseFileinfo();                               return;
        case KEYWORD_FIXES:                    this->ChangeSection(SECTION_FIXES);            return;
        case KEYWORD_FLARES:                   this->ChangeSection(SECTION_FLARES);           return;
        case KEYWORD_FLARES2:                  this->ChangeSection(SECTION_FLARES_2);         return;
        case KEYWORD_FLEXBODIES:               this->ChangeSection(SECTION_FLEXBODIES);       return;
        case KEYWORD_FLEXBODY_CAMERA_MODE:     this->ParseDirectiveFlexbodyCameraMode();            return;
        case KEYWORD_FLEXBODYWHEELS:           this->ChangeSection(SECTION_FLEX_BODY_WHEELS); return;
        case KEYWORD_FORWARDCOMMANDS:          m_current_module->sequence.push_back(SeqElement(m_last_keyword, -1));     return;
        case KEYWORD_FUSEDRAG:                 this->ChangeSection(SECTION_FUSEDRAG);         return;
        case KEYWORD_GLOBALS:                  this->ChangeSection(SECTION_GLOBALS);          return;
        case KEYWORD_GUID:                     this->ParseGuid();                                   return;
        case KEYWORD_GUISETTINGS:              this->ChangeSection(SECTION_GUI_SETTINGS);     return;
        case KEYWORD_HELP:                     this->ChangeSection(SECTION_HELP);             return;
        case KEYWORD_HIDE_IN_CHOOSER:          m_current_module->sequence.push_back(SeqElement(m_last_keyword, -1));      return;
        case KEYWORD_HOOKGROUP:                /* Obsolete, ignored */                              return;
        case KEYWORD_HOOKS:                    this->ChangeSection(SECTION_HOOKS);            return;
        case KEYWORD_HYDROS:                   this->ChangeSection(SECTION_HYDROS);           return;
        case KEYWORD_IMPORTCOMMANDS:           m_current_module->sequence.push_back(SeqElement(m_last_keyword, -1));                return;
        case KEYWORD_INTERAXLES:               this->ChangeSection(SECTION_INTERAXLES);       return;
        case KEYWORD_LOCKGROUPS:               this->ChangeSection(SECTION_LOCKGROUPS);       return;
        case KEYWORD_LOCKGROUP_DEFAULT_NOLOCK: m_current_module->sequence.push_back(SeqElement(m_last_keyword, -1));              return;
        case KEYWORD_MANAGEDMATERIALS:         this->ChangeSection(SECTION_MANAGED_MATERIALS); return;
        case KEYWORD_MATERIALFLAREBINDINGS:    this->ChangeSection(SECTION_MAT_FLARE_BINDINGS); return;
        case KEYWORD_MESHWHEELS:               this->ChangeSection(SECTION_MESH_WHEELS);      return;
        case KEYWORD_MESHWHEELS2:              this->ChangeSection(SECTION_MESH_WHEELS_2);    return;
        case KEYWORD_MINIMASS:                 this->ChangeSection(SECTION_MINIMASS);         return;
        case KEYWORD_NODECOLLISION:            this->ChangeSection(SECTION_NODE_COLLISION);   return;
        case KEYWORD_NODES:                    this->ChangeSection(SECTION_NODES);            return;
        case KEYWORD_NODES2:                   this->ChangeSection(SECTION_NODES_2);          return;
        case KEYWORD_PARTICLES:                this->ChangeSection(SECTION_PARTICLES);        return;
        case KEYWORD_PISTONPROPS:              this->ChangeSection(SECTION_PISTONPROPS);      return;
        case KEYWORD_PROP_CAMERA_MODE:         this->ParseDirectivePropCameraMode();                return;
        case KEYWORD_PROPS:                    this->ChangeSection(SECTION_PROPS);            return;
        case KEYWORD_RAILGROUPS:               this->ChangeSection(SECTION_RAILGROUPS);       return;
        case KEYWORD_RESCUER:                  m_current_module->sequence.push_back(SeqElement(m_last_keyword, -1));               return;
        case KEYWORD_RIGIDIFIERS:              this->AddMessage(Message::TYPE_WARNING, "Rigidifiers are not supported, ignoring..."); return;
        case KEYWORD_ROLLON:                   m_current_module->sequence.push_back(SeqElement(m_last_keyword, -1));               return;
        case KEYWORD_ROPABLES:                 this->ChangeSection(SECTION_ROPABLES);         return;
        case KEYWORD_ROPES:                    this->ChangeSection(SECTION_ROPES);            return;
        case KEYWORD_ROTATORS:                 this->ChangeSection(SECTION_ROTATORS);         return;
        case KEYWORD_ROTATORS2:                this->ChangeSection(SECTION_ROTATORS_2);       return;
        case KEYWORD_SCREWPROPS:               this->ChangeSection(SECTION_SCREWPROPS);       return;
        case KEYWORD_SECTION:                  this->BeginModule();              return;
        case KEYWORD_SECTIONCONFIG:            /* Ignored */                                        return;
        case KEYWORD_SET_BEAM_DEFAULTS:        this->ParseDirectiveSetBeamDefaults();               return;
        case KEYWORD_SET_BEAM_DEFAULTS_SCALE:  this->ParseDirectiveSetBeamDefaultsScale();          return;
        case KEYWORD_SET_COLLISION_RANGE:      this->ParseSetCollisionRange();                      return;
        case KEYWORD_SET_DEFAULT_MINIMASS:     this->ParseDirectiveSetDefaultMinimass();            return;
        case KEYWORD_SET_INERTIA_DEFAULTS:     this->ParseDirectiveSetInertiaDefaults();            return;
        case KEYWORD_SET_MANAGEDMATERIALS_OPTIONS:  this->ParseDirectiveSetManagedMaterialsOptions();    return;
        case KEYWORD_SET_NODE_DEFAULTS:        this->ParseDirectiveSetNodeDefaults();               return;
        case KEYWORD_SET_SKELETON_SETTINGS:    this->ParseSetSkeletonSettings();                    return;
        case KEYWORD_SHOCKS:                   this->ChangeSection(SECTION_SHOCKS);           return;
        case KEYWORD_SHOCKS2:                  this->ChangeSection(SECTION_SHOCKS_2);         return;
        case KEYWORD_SHOCKS3:                  this->ChangeSection(SECTION_SHOCKS_3);         return;
        case KEYWORD_SLIDENODE_CONNECT_INSTANT:
            m_current_module->sequence.push_back(SeqElement(m_last_keyword, -1));
            return;
        case KEYWORD_SLIDENODES:               this->ChangeSection(SECTION_SLIDENODES);       return;
        case KEYWORD_SLOPE_BRAKE:              this->ParseSlopeBrake();                             return;
        case KEYWORD_SOUNDSOURCES:             this->ChangeSection(SECTION_SOUNDSOURCES);     return;
        case KEYWORD_SOUNDSOURCES2:            this->ChangeSection(SECTION_SOUNDSOURCES2);    return;
        case KEYWORD_SPEEDLIMITER:             this->ParseSpeedLimiter();                           return;
        case KEYWORD_SUBMESH_GROUNDMODEL:      this->ParseSubmeshGroundModel();                     return;
        case KEYWORD_SUBMESH:                  this->ChangeSection(SECTION_SUBMESH);          return;
        case KEYWORD_TEXCOORDS:                this->ProcessKeywordTexcoords();                     return;
        case KEYWORD_TIES:                     this->ChangeSection(SECTION_TIES);             return;
        case KEYWORD_TORQUECURVE:              this->ChangeSection(SECTION_TORQUE_CURVE);     return;
        case KEYWORD_TRACTION_CONTROL:         this->ParseTractionControl();                        return;
        case KEYWORD_TRANSFER_CASE:            this->ChangeSection(SECTION_TRANSFER_CASE);    return;
        case KEYWORD_TRIGGERS:                 this->ChangeSection(SECTION_TRIGGERS);         return;
        case KEYWORD_TURBOJETS:                this->ChangeSection(SECTION_TURBOJETS);        return;
        case KEYWORD_TURBOPROPS:               this->ChangeSection(SECTION_TURBOPROPS);       return;
        case KEYWORD_TURBOPROPS2:              this->ChangeSection(SECTION_TURBOPROPS_2);     return;
        case KEYWORD_VIDEOCAMERA:              this->ChangeSection(SECTION_VIDEO_CAMERA);     return;
        case KEYWORD_WHEELDETACHERS:           this->ChangeSection(SECTION_WHEELDETACHERS);   return;
        case KEYWORD_WHEELS:                   this->ChangeSection(SECTION_WHEELS);           return;
        case KEYWORD_WHEELS2:                  this->ChangeSection(SECTION_WHEELS_2);         return;
        case KEYWORD_WINGS:                    this->ChangeSection(SECTION_WINGS);            return;
    }

    // Parse current section, if any 
    switch (m_current_section)
    {
        case (SECTION_AIRBRAKES):            this->ParseAirbrakes();               return;
        case (SECTION_ANIMATORS):            this->ParseAnimator();                return;
        case (SECTION_AXLES):                this->ParseAxles();                   return;
        case (SECTION_TRUCK_NAME):           this->ParseActorNameLine();           return; 
        case (SECTION_BEAMS):                this->ParseBeams();                   return;
        case (SECTION_BRAKES):               this->ParseBrakes();                  return;
        case (SECTION_CAMERAS):              this->ParseCameras();                 return;
        case (SECTION_CAMERA_RAIL):          this->ParseCameraRails();             return;
        case (SECTION_CINECAM):              this->ParseCinecam();                 return;
        case (SECTION_COMMANDS):
        case (SECTION_COMMANDS_2):           this->ParseCommandsUnified();         return;
        case (SECTION_COLLISION_BOXES):      this->ParseCollisionBox();            return;
        case (SECTION_CONTACTERS):           this->ParseContacter();               return;
        case (SECTION_ENGINE):               this->ParseEngine();                  return;
        case (SECTION_ENGOPTION):            this->ParseEngoption();               return;
        case (SECTION_ENGTURBO) :            this->ParseEngturbo();                return;
        case (SECTION_EXHAUSTS):             this->ParseExhaust();                 return;
        case (SECTION_FIXES):                this->ParseFixes();                   return;
        case (SECTION_FLARES):
        case (SECTION_FLARES_2):             this->ParseFlaresUnified();           return;
        case (SECTION_FLEXBODIES):           this->ParseFlexbody();                return;
        case (SECTION_FLEX_BODY_WHEELS):     this->ParseFlexBodyWheel();           return;
        case (SECTION_FUSEDRAG):             this->ParseFusedrag();                return;
        case (SECTION_GLOBALS):              this->ParseGlobals();                 return;
        case (SECTION_GUI_SETTINGS):         this->ParseGuiSettings();             return;
        case (SECTION_HELP):                 this->ParseHelp();                    return;
        case (SECTION_HOOKS):                this->ParseHook();                    return;
        case (SECTION_HYDROS):               this->ParseHydros();                  return;
        case (SECTION_INTERAXLES):           this->ParseInterAxles();              return;
        case (SECTION_LOCKGROUPS):           this->ParseLockgroups();              return;
        case (SECTION_MANAGED_MATERIALS):    this->ParseManagedMaterials();        return;
        case (SECTION_MAT_FLARE_BINDINGS):   this->ParseMaterialFlareBindings();   return;
        case (SECTION_MESH_WHEELS):
        case (SECTION_MESH_WHEELS_2):        this->ParseMeshWheelUnified();        return;
        case (SECTION_MINIMASS):             this->ParseMinimass();                return;
        case (SECTION_NODE_COLLISION):       this->ParseNodeCollision();           return;
        case (SECTION_NODES):
        case (SECTION_NODES_2):              this->ParseNodesUnified();            return;
        case (SECTION_PARTICLES):            this->ParseParticles();               return;
        case (SECTION_PISTONPROPS):          this->ParsePistonprops();             return;
        case (SECTION_PROPS):                this->ParseProps();                   return;
        case (SECTION_RAILGROUPS):           this->ParseRailGroups();              return;
        case (SECTION_ROPABLES):             this->ParseRopables();                return;
        case (SECTION_ROPES):                this->ParseRopes();                   return;
        case (SECTION_ROTATORS):
        case (SECTION_ROTATORS_2):           this->ParseRotatorsUnified();         return;
        case (SECTION_SCREWPROPS):           this->ParseScrewprops();              return;
        case (SECTION_SHOCKS):               this->ParseShock();                   return;
        case (SECTION_SHOCKS_2):             this->ParseShock2();                  return;
        case (SECTION_SHOCKS_3):             this->ParseShock3();                  return;
        case (SECTION_SLIDENODES):           this->ParseSlidenodes();              return;
        case (SECTION_SOUNDSOURCES):         this->ParseSoundsources();            return;
        case (SECTION_SOUNDSOURCES2):        this->ParseSoundsources2();           return;
        case (SECTION_SUBMESH):              this->ParseSubmesh();                 return;
        case (SECTION_TIES):                 this->ParseTies();                    return;
        case (SECTION_TORQUE_CURVE):         this->ParseTorqueCurve();             return;
        case (SECTION_TRANSFER_CASE):        this->ParseTransferCase();            return;
        case (SECTION_TRIGGERS):             this->ParseTriggers();                return;
        case (SECTION_TURBOJETS):            this->ParseTurbojets();               return;
        case (SECTION_TURBOPROPS):           
        case (SECTION_TURBOPROPS_2):         this->ParseTurbopropsUnified();       return;
        case (SECTION_VIDEO_CAMERA):         this->ParseVideoCamera();             return;
        case (SECTION_WHEELDETACHERS):       this->ParseWheelDetachers();          return;
        case (SECTION_WHEELS):               this->ParseWheel();                   return;
        case (SECTION_WHEELS_2):             this->ParseWheel2();                  return;
        case (SECTION_WINGS):                this->ParseWing();                    return;
        default:;
    };
}

bool Parser::CheckNumArguments(int num_required_args)
{
    if (num_required_args > m_num_args)
    {
        char msg[200];
        snprintf(msg, 200, "Not enough arguments, %d required, got %d. Skipping line.", num_required_args, m_num_args);
        this->AddMessage(Message::TYPE_WARNING, msg);
        return false;
    }
    return true;
}

// -------------------------------------------------------------------------- 
// Parsing individual keywords                                                
// -------------------------------------------------------------------------- 

void Parser::ParseActorNameLine()
{
    m_definition->name = m_current_line; // Already trimmed
    this->ChangeSection(SECTION_NONE);
}

void Parser::ParseWing()
{
    if (!this->CheckNumArguments(16)) { return; }

    Wing wing;

    for (int i = 0; i <  8; i++) { wing.nodes[i]        = this->GetArgNodeRef     (i);  }
    for (int i = 8; i < 16; i++) { wing.tex_coords[i-8] = this->GetArgFloat       (i);  }

    if (m_num_args > 16)         { wing.control_surface = this->GetArgWingSurface (16); }
    if (m_num_args > 17)         { wing.chord_point     = this->GetArgFloat       (17); }
    if (m_num_args > 18)         { wing.min_deflection  = this->GetArgFloat       (18); }
    if (m_num_args > 19)         { wing.max_deflection  = this->GetArgFloat       (19); }
    if (m_num_args > 20)         { wing.airfoil         = this->GetArgStr         (20); }
    if (m_num_args > 21)         { wing.efficacy_coef   = this->GetArgFloat       (21); }

    m_current_module->wings.push_back(wing);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->wings.size() - 1));
}

void Parser::ParseSetCollisionRange()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    CollisionRangePreset preset;

    preset.collrange = this->GetArgFloat(1);
    if (preset.collrange < 0)
    {
        preset.collrange = DEFAULT_COLLISION_RANGE;
    }

    m_current_module->collision_range_preset.push_back(preset);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->collision_range_preset.size() - 1));
}

void Parser::ParseWheel2()
{
    if (!this->CheckNumArguments(17)) { return; }

    Wheel2 wheel_2;

    wheel_2.rim_radius         = this->GetArgFloat        ( 0);
    wheel_2.tyre_radius        = this->GetArgFloat        ( 1);
    wheel_2.width              = this->GetArgFloat        ( 2);
    wheel_2.num_rays           = this->GetArgInt          ( 3);
    wheel_2.nodes[0]           = this->GetArgNodeRef      ( 4);
    wheel_2.nodes[1]           = this->GetArgNodeRef      ( 5);
    wheel_2.rigidity_node      = this->GetArgRigidityNode ( 6);
    wheel_2.braking            = this->GetArgBraking      ( 7);
    wheel_2.propulsion         = this->GetArgPropulsion   ( 8);
    wheel_2.reference_arm_node = this->GetArgNodeRef      ( 9);
    wheel_2.mass               = this->GetArgFloat        (10);
    wheel_2.rim_springiness    = this->GetArgFloat        (11);
    wheel_2.rim_damping        = this->GetArgFloat        (12);
    wheel_2.tyre_springiness   = this->GetArgFloat        (13);
    wheel_2.tyre_damping       = this->GetArgFloat        (14);
    wheel_2.face_material_name = this->GetArgStr          (15);
    wheel_2.band_material_name = this->GetArgStr          (16);

    m_current_module->wheels_2.push_back(wheel_2);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->wheels_2.size() - 1));
}

void Parser::ParseWheel()
{
    if (! this->CheckNumArguments(14)) { return; }

    Wheel wheel;


    wheel.radius             = this->GetArgFloat        ( 0);
    wheel.width              = this->GetArgFloat        ( 1);
    wheel.num_rays           = this->GetArgInt          ( 2);
    wheel.nodes[0]           = this->GetArgNodeRef      ( 3);
    wheel.nodes[1]           = this->GetArgNodeRef      ( 4);
    wheel.rigidity_node      = this->GetArgRigidityNode ( 5);
    wheel.braking            = this->GetArgBraking      ( 6);
    wheel.propulsion         = this->GetArgPropulsion   ( 7);
    wheel.reference_arm_node = this->GetArgNodeRef      ( 8);
    wheel.mass               = this->GetArgFloat        ( 9);
    wheel.springiness        = this->GetArgFloat        (10);
    wheel.damping            = this->GetArgFloat        (11);
    wheel.face_material_name = this->GetArgStr          (12);
    wheel.band_material_name = this->GetArgStr          (13);

    m_current_module->wheels.push_back(wheel);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->wheels.size() - 1));
}

void Parser::ParseWheelDetachers()
{
    if (! this->CheckNumArguments(2)) { return; }

    WheelDetacher wheeldetacher;

    wheeldetacher.wheel_id       = this->GetArgInt(0);
    wheeldetacher.detacher_group = this->GetArgInt(1);

    m_current_module->wheeldetachers.push_back(wheeldetacher);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->wheeldetachers.size() - 1));
}

void Parser::ParseTractionControl()
{
    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line + 15, ","); // "TractionControl" = 15 characters
    if (tokens.size() < 2)
    {
        this->AddMessage(Message::TYPE_ERROR, "Too few arguments");
        return;
    }

    TractionControl tc;
                             tc.regulation_force = this->ParseArgFloat(tokens[0].c_str());
                             tc.wheel_slip       = this->ParseArgFloat(tokens[1].c_str());
    if (tokens.size() > 2) { tc.fade_speed       = this->ParseArgFloat(tokens[2].c_str()); }
    if (tokens.size() > 3) { tc.pulse_per_sec    = this->ParseArgFloat(tokens[3].c_str()); }

    for (unsigned int i=4; i<tokens.size(); i++)
    {
        Ogre::StringVector args2 = Ogre::StringUtil::split(tokens[i], ":");
        Ogre::StringUtil::trim(args2[0]);
        Ogre::StringUtil::toLowerCase(args2[0]);

        if (args2[0] == "mode" && args2.size() == 2)
        {
            Ogre::StringVector attrs = Ogre::StringUtil::split(args2[1], "&");
            auto itor = attrs.begin();
            auto endi = attrs.end();
            for (; itor != endi; ++itor)
            {
                std::string attr = *itor;
                Ogre::StringUtil::trim(attr);
                Ogre::StringUtil::toLowerCase(attr);
                     if (strncmp(attr.c_str(), "nodash", 6)   == 0) { tc.attr_no_dashboard = true;  }
                else if (strncmp(attr.c_str(), "notoggle", 8) == 0) { tc.attr_no_toggle    = true;  }
                else if (strncmp(attr.c_str(), "on", 2)       == 0) { tc.attr_is_on        = true;  }
                else if (strncmp(attr.c_str(), "off", 3)      == 0) { tc.attr_is_on        = false; }
            }
        }
        else
        {
            this->AddMessage(Message::TYPE_ERROR, "TractionControl Mode: missing");
            tc.attr_no_dashboard = false;
            tc.attr_no_toggle = false;
            tc.attr_is_on = true;
        }
    }

    m_current_module->traction_control.push_back(tc);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->traction_control.size() - 1));
}

void Parser::ParseTransferCase()
{
    if (! this->CheckNumArguments(2)) { return; }

    TransferCase tc;

    tc.a1 = this->GetArgInt(0) - 1;
    tc.a2 = this->GetArgInt(1) - 1;
    if (m_num_args > 2) { tc.has_2wd    = this->GetArgInt(2); }
    if (m_num_args > 3) { tc.has_2wd_lo = this->GetArgInt(3); }
    for (int i = 4; i < m_num_args; i++) { tc.gear_ratios.push_back(this->GetArgFloat(i)); }

    m_current_module->transfer_case.push_back(tc);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->transfer_case.size() - 1));
}

void Parser::ParseSubmeshGroundModel()
{
    if (!this->CheckNumArguments(2)) { return; } // Items: keyword, arg

    m_current_module->submesh_groundmodel.push_back(this->GetArgStr(1));
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->submesh_groundmodel.size() - 1));
}

void Parser::ParseSpeedLimiter()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    SpeedLimiter sl;
    sl.max_speed = this->GetArgFloat(1);
    if (sl.max_speed <= 0.f)
    {
        char msg[200];
        snprintf(msg, 200, "Invalid 'max_speed' (%f), must be > 0.0. Using it anyway (compatibility)", sl.max_speed);
        this->AddMessage(Message::TYPE_WARNING, msg);
    }
    m_current_module->speed_limiter.push_back(sl);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->speed_limiter.size() - 1));
}

void Parser::ParseSlopeBrake()
{
    SlopeBrake sb;
    
    if (m_num_args > 1) { sb.regulating_force = this->GetArgFloat(1); }
    if (m_num_args > 2) { sb.attach_angle     = this->GetArgFloat(2); }
    if (m_num_args > 3) { sb.release_angle    = this->GetArgFloat(3); }

    m_current_module->slope_brake.push_back(sb);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->slope_brake.size() - 1));
}

void Parser::ParseSetSkeletonSettings()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    SkeletonSettings skel;
    skel.visibility_range_meters = this->GetArgFloat(1);
    if (m_num_args > 2) { skel.beam_thickness_meters = this->GetArgFloat(2); }

    m_current_module->skeleton_settings.push_back(skel);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->skeleton_settings.size() - 1));
}

void Parser::ParseDirectiveSetNodeDefaults()
{
    if (!this->CheckNumArguments(2)) { return; }

    NodeDefaults def;
    def._num_args = m_num_args;

                        def.loadweight = this->GetArgFloat(1);
    if (m_num_args > 2) def.friction   = this->GetArgFloat(2);
    if (m_num_args > 3) def.volume     = this->GetArgFloat(3);
    if (m_num_args > 4) def.surface    = this->GetArgFloat(4);
    if (m_num_args > 5) def.options    = this->GetArgStr(5);

    m_current_module->node_defaults.push_back(def);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->node_defaults.size() - 1));
}

void Parser::ParseDirectiveSetManagedMaterialsOptions()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    ManagedMatOptions def;
    def.double_sided = this->GetArgInt(1) != 0;

    m_current_module->managed_mat_options.push_back(def);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->managed_mat_options.size() - 1));
}

void Parser::ParseDirectiveSetBeamDefaultsScale()
{
    if (! this->CheckNumArguments(5)) { return; }

    BeamDefaultsScale scale;
    scale._num_args = m_num_args;

    scale.springiness = this->GetArgFloat(1);
    if (m_num_args > 2) { scale.damping_constant = this->GetArgFloat(2); }
    if (m_num_args > 3) { scale.deformation_threshold_constant = this->GetArgFloat(3); }
    if (m_num_args > 4) { scale.breaking_threshold_constant = this->GetArgFloat(4); }

    m_current_module->beam_defaults_scale.push_back(scale);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->beam_defaults_scale.size() - 1));
}

void Parser::ParseDirectiveSetBeamDefaults()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    BeamDefaults d;
    d._num_args = m_num_args;

    d.springiness = this->GetArgFloat(1);
    if (m_num_args > 2) d.damping_constant = this->GetArgFloat(2);
    if (m_num_args > 3) d.deformation_threshold = this->GetArgFloat(3);
    if (m_num_args > 4) d.breaking_threshold = this->GetArgFloat(4);
    if (m_num_args > 5) d.visual_beam_diameter = this->GetArgFloat(5);
    if (m_num_args > 6) d.beam_material_name = this->GetArgStr(6);
    if (m_num_args > 7) d.plastic_deform_coef = this->GetArgFloat(7);

    m_current_module->beam_defaults.push_back(d);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->beam_defaults.size() - 1));
}

void Parser::ParseDirectivePropCameraMode()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    m_current_module->prop_camera_mode.push_back(this->GetArgInt(1));
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->prop_camera_mode.size() - 1));
}

void Parser::ParseDirectiveBackmesh()
{
    if (m_current_section != SECTION_SUBMESH)
    {
        this->AddMessage(Message::TYPE_ERROR, "Misplaced sub-directive 'backmesh' (belongs in section 'submesh'), ignoring...");
        return;
    }

    m_current_module->backmesh.push_back(true);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->backmesh.size() - 1));
}

void Parser::ProcessKeywordTexcoords()
{
    if (m_current_section == SECTION_SUBMESH)
    {
        m_current_subsection = SUBSECTION__SUBMESH__TEXCOORDS;
    }
    else
    {
        this->AddMessage(Message::TYPE_WARNING, "Misplaced sub-section 'texcoords' (belongs in section 'submesh'), falling back to classic unsafe parsing method.");
        this->ChangeSection(SECTION_SUBMESH);
    }
}

void Parser::ProcessKeywordCab()
{
    m_current_subsection = SUBSECTION__SUBMESH__CAB;
    if (m_current_section != SECTION_SUBMESH)
    {
        this->AddMessage(Message::TYPE_WARNING, "Misplaced sub-section 'cab' (belongs in section 'submesh')");
        this->ChangeSection(SECTION_SUBMESH);
    }
}

void Parser::ParseMeshWheelUnified()
{
    if (! this->CheckNumArguments(16)) { return; }

    MeshWheel mesh_wheel;


    mesh_wheel.tyre_radius        = this->GetArgFloat        ( 0);
    mesh_wheel.rim_radius         = this->GetArgFloat        ( 1);
    mesh_wheel.width              = this->GetArgFloat        ( 2);
    mesh_wheel.num_rays           = this->GetArgInt          ( 3);
    mesh_wheel.nodes[0]           = this->GetArgNodeRef      ( 4);
    mesh_wheel.nodes[1]           = this->GetArgNodeRef      ( 5);
    mesh_wheel.rigidity_node      = this->GetArgRigidityNode ( 6);
    mesh_wheel.braking            = this->GetArgBraking      ( 7);
    mesh_wheel.propulsion         = this->GetArgPropulsion   ( 8);
    mesh_wheel.reference_arm_node = this->GetArgNodeRef      ( 9);
    mesh_wheel.mass               = this->GetArgFloat        (10);
    mesh_wheel.spring             = this->GetArgFloat        (11);
    mesh_wheel.damping            = this->GetArgFloat        (12);
    mesh_wheel.side               = this->GetArgWheelSide    (13);
    mesh_wheel.mesh_name          = this->GetArgStr          (14);
    mesh_wheel.material_name      = this->GetArgStr          (15);

    m_current_module->mesh_wheels.push_back(mesh_wheel);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->mesh_wheels.size() - 1));
}

void Parser::ParseHook()
{
    if (! this->CheckNumArguments(1)) { return; }

    Hook hook;
    hook.node = this->GetArgNodeRef(0);

    int i = 1;
    while (i < m_num_args)
    {
        std::string attr = this->GetArgStr(i);
        Ogre::StringUtil::trim(attr);
        const bool has_value = (i < (m_num_args - 1));

        // Values
             if (has_value && (attr == "hookrange")                          ) { hook.option_hook_range       = this->GetArgFloat(++i); }
        else if (has_value && (attr == "speedcoef")                          ) { hook.option_speed_coef       = this->GetArgFloat(++i); }
        else if (has_value && (attr == "maxforce")                           ) { hook.option_max_force        = this->GetArgFloat(++i); }
        else if (has_value && (attr == "timer")                              ) { hook.option_timer            = this->GetArgFloat(++i); }
        else if (has_value && (attr == "hookgroup"  || attr == "hgroup")     ) { hook.option_hookgroup        = this->GetArgInt  (++i); }
        else if (has_value && (attr == "lockgroup"  || attr == "lgroup")     ) { hook.option_lockgroup        = this->GetArgInt  (++i); }
        else if (has_value && (attr == "shortlimit" || attr == "short_limit")) { hook.option_min_range_meters = this->GetArgFloat(++i); }
        // Flags
        else if ((attr == "selflock") ||(attr == "self-lock") ||(attr == "self_lock") ) { hook.flag_self_lock  = true; }
        else if ((attr == "autolock") ||(attr == "auto-lock") ||(attr == "auto_lock") ) { hook.flag_auto_lock  = true; }
        else if ((attr == "nodisable")||(attr == "no-disable")||(attr == "no_disable")) { hook.flag_no_disable = true; }
        else if ((attr == "norope")   ||(attr == "no-rope")   ||(attr == "no_rope")   ) { hook.flag_no_rope    = true; }
        else if ((attr == "visible")  ||(attr == "vis")                               ) { hook.flag_visible    = true; }
        else
        {
            std::string msg = "Ignoring invalid option: " + attr;
            this->AddMessage(Message::TYPE_ERROR, msg.c_str());
        }
        i++;
    }

    m_current_module->hooks.push_back(hook);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->hooks.size() - 1));
}

void Parser::ParseHelp()
{
    m_current_module->help.push_back(m_current_line); // already trimmed
}

void Parser::ParseGuiSettings()
{
    if (! this->CheckNumArguments(2)) { return; }

    // Special behavior - not sequential, at most 1 in module
    if (m_current_module->gui_settings.size() == 0)
    {
        m_current_module->gui_settings.push_back(GuiSettings());
    }

    GuiSettings* gui_settings = &m_current_module->gui_settings.back();

    std::string key = this->GetArgStr(0);
    
    if (key == "debugBeams") {} // Obsolete, ignored
    
    else if (key == "tachoMaterial")  { gui_settings->tacho_material     =  this->GetArgStr(1); }
    else if (key == "speedoMaterial") { gui_settings->speedo_material    =  this->GetArgStr(1); }
    else if (key == "speedoMax")      { gui_settings->speedo_highest_kph =  this->GetArgInt(1); }
    else if (key == "useMaxRPM")      { gui_settings->use_max_rpm        = (this->GetArgInt(1) == 1); }
    else if (key == "helpMaterial")   { gui_settings->help_material      =  this->GetArgStr(1); }
    
    else if (key == "dashboard")        { gui_settings->dashboard_layouts    .push_back(this->GetArgStr(1)); }
    else if (key == "texturedashboard") { gui_settings->rtt_dashboard_layouts.push_back(this->GetArgStr(1)); }
    
    else if (key == "interactiveOverviewMap")
    {
        std::string val = this->GetArgStr(1);
        
             if (val == "off"   ) { gui_settings->interactive_overview_map_mode = GuiSettings::MAP_MODE_OFF;    }
        else if (val == "simple") { gui_settings->interactive_overview_map_mode = GuiSettings::MAP_MODE_SIMPLE; }
        else if (val == "zoom"  ) { gui_settings->interactive_overview_map_mode = GuiSettings::MAP_MODE_ZOOM;   }
        
        else { this->AddMessage(Message::TYPE_ERROR, "Unknown map mode [" + val + "], ignoring..."); }
    }
    else
    {
        this->AddMessage(Message::TYPE_ERROR, "Unknown setting [" + key + "], ignoring...");
    }
}

void Parser::ParseGuid()
{
    if (! this->CheckNumArguments(2)) { return; }

    m_definition->guid = this->GetArgStr(1);
}

void Parser::ParseGlobals()
{
    if (! this->CheckNumArguments(2)) { return; }

    Globals globals;
    globals.dry_mass   = this->GetArgFloat(0);
    globals.cargo_mass = this->GetArgFloat(1);

    if (m_num_args > 2) { globals.material_name = this->GetArgStr(2); }

    m_current_module->globals.push_back(globals);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->globals.size() - 1));
}

void Parser::ParseFusedrag()
{
    if (! this->CheckNumArguments(3)) { return; }

    Fusedrag fusedrag;
    fusedrag.front_node = this->GetArgNodeRef(0);
    fusedrag.rear_node  = this->GetArgNodeRef(1);

    if (this->GetArgStr(2) == "autocalc")
    {
        fusedrag.autocalc = true;

        // Fusedrag autocalculation from truck size
        if (m_num_args > 3) { fusedrag.area_coefficient = this->GetArgFloat(3); }
        if (m_num_args > 4) { fusedrag.airfoil_name     = this->GetArgStr  (4); }
    }
    else
    {
        // Original calculation
        fusedrag.approximate_width = this->GetArgFloat(2);
        
        if (m_num_args > 3) { fusedrag.airfoil_name = this->GetArgStr(3); }
    }

    m_current_module->fusedrag.push_back(fusedrag);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->fusedrag.size() - 1));
}

void Parser::ParseDirectiveFlexbodyCameraMode()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, arg

    m_current_module->flexbody_camera_mode.push_back(this->GetArgInt(1));
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->flexbody_camera_mode.size() - 1));
}

void Parser::ParseSubmesh()
{
    if (m_current_subsection == SUBSECTION__SUBMESH__CAB)
    {
        if (! this->CheckNumArguments(3)) { return; }

        Cab cab;
        cab.nodes[0] = this->GetArgNodeRef(0);
        cab.nodes[1] = this->GetArgNodeRef(1);
        cab.nodes[2] = this->GetArgNodeRef(2);
        if (m_num_args > 3)
        {
            cab.options = 0;
            std::string options_str = this->GetArgStr(3);
            for (unsigned int i = 0; i < options_str.length(); i++)
            {
                switch (options_str.at(i))
                {
                case 'c': cab.options |=  Cab::OPTION_c_CONTACT;                               break;
                case 'b': cab.options |=  Cab::OPTION_b_BUOYANT;                               break;
                case 'D': cab.options |= (Cab::OPTION_c_CONTACT      | Cab::OPTION_b_BUOYANT); break;
                case 'p': cab.options |=  Cab::OPTION_p_10xTOUGHER;                            break;
                case 'u': cab.options |=  Cab::OPTION_u_INVULNERABLE;                          break;
                case 'F': cab.options |= (Cab::OPTION_p_10xTOUGHER   | Cab::OPTION_b_BUOYANT); break;
                case 'S': cab.options |= (Cab::OPTION_u_INVULNERABLE | Cab::OPTION_b_BUOYANT); break; 
                case 'n': break; // Placeholder, does nothing 

                default:
                    char msg[200] = "";
                    snprintf(msg, 200, "'submesh/cab' Ignoring invalid option '%c'...", options_str.at(i));
                    this->AddMessage(Message::TYPE_WARNING, msg);
                    break;
                }
            }
        }

        m_current_module->submeshes.back().cab_triangles.push_back(cab);
    }
    else if (m_current_subsection == SUBSECTION__SUBMESH__TEXCOORDS)
    {
        if (! this->CheckNumArguments(3)) { return; }

        Texcoord texcoord;
        texcoord.node = this->GetArgNodeRef(0);
        texcoord.u    = this->GetArgFloat  (1);
        texcoord.v    = this->GetArgFloat  (2);

        m_current_module->submeshes.back().texcoords.push_back(texcoord);
    }
    else
    {
        AddMessage(Message::TYPE_ERROR, "Section submesh has no subsection defined, line not parsed.");
    }
}

void Parser::ParseFlexbody()
{
    if (m_current_subsection == SUBSECTION__FLEXBODIES__PROPLIKE_LINE)
    {
        if (! this->CheckNumArguments(10)) { return; }

        Flexbody flexbody;
        flexbody.reference_node = this->GetArgNodeRef (0);
        flexbody.x_axis_node    = this->GetArgNodeRef (1);
        flexbody.y_axis_node    = this->GetArgNodeRef (2);
        flexbody.offset.x       = this->GetArgFloat   (3);
        flexbody.offset.y       = this->GetArgFloat   (4);
        flexbody.offset.z       = this->GetArgFloat   (5);
        flexbody.rotation.x     = this->GetArgFloat   (6);
        flexbody.rotation.y     = this->GetArgFloat   (7);
        flexbody.rotation.z     = this->GetArgFloat   (8);
        flexbody.mesh_name      = this->GetArgStr     (9);

        m_current_module->flexbodies.push_back(flexbody);
        m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->flexbodies.size() - 1));

        // Switch subsection
        m_current_subsection =  SUBSECTION__FLEXBODIES__FORSET_LINE;
    }
    else if (m_current_subsection == SUBSECTION__FLEXBODIES__FORSET_LINE)
    {
        if (m_current_module->flexbodies.size() > 0)
        {
            //parsing set definition
            char* pos=m_current_line + 6; // skip 'forset'
            char* end=pos;
            char endwas='G';
            while (endwas!=0)
            {
                unsigned int val1, val2;
                end=pos;
                while (*end!='-' && *end!=',' && *end!=0) end++;
                endwas=*end;
                *end=0;
                val1=strtoul(pos, 0, 10);
                if (endwas=='-')
                {
                    pos=end+1;
                    end=pos;
                    while (*end!=',' && *end!=0) end++;
                    endwas=*end;
                    *end=0;
                    val2=strtoul(pos, 0, 10);
                    m_current_module->flexbodies.back().forset.push_back(Node::Range(Node::Ref(val1), Node::Ref(val2)));
                }
                else
                {
                    m_current_module->flexbodies.back().forset.push_back(Node::Range(Node::Ref(val1), Node::Ref(val1)));
                }
                pos=end+1;
            }
        }

        // Switch subsection 
        m_current_subsection =  SUBSECTION__FLEXBODIES__PROPLIKE_LINE;
    }
    else
    {
        AddMessage(Message::TYPE_FATAL_ERROR, "Internal parser failure, section 'flexbodies' not parsed.");
    }
}

void Parser::ParseFlaresUnified()
{
    const bool is_flares2 = (m_current_section == SECTION_FLARES_2);
    if (! this->CheckNumArguments(is_flares2 ? 6 : 5)) { return; }

    Flare2 flare2;
    int pos = 0;
    flare2.reference_node = this->GetArgNodeRef(pos++);
    flare2.node_axis_x    = this->GetArgNodeRef(pos++);
    flare2.node_axis_y    = this->GetArgNodeRef(pos++);
    flare2.offset.x       = this->GetArgFloat  (pos++);
    flare2.offset.y       = this->GetArgFloat  (pos++);

    if (m_current_section == SECTION_FLARES_2)
    {
        flare2.offset.z = this->GetArgFloat(pos++);
    }

    if (m_num_args > pos) { flare2.type              = this->GetArgFlareType(pos++); }
    if (m_num_args > pos) { flare2.control_number    = this->GetArgInt      (pos++); }
    if (m_num_args > pos) { flare2.blink_delay_milis = this->GetArgInt      (pos++); }
    if (m_num_args > pos) { flare2.size              = this->GetArgFloat    (pos++); }
    if (m_num_args > pos) { flare2.material_name     = this->GetArgStr      (pos++); }

    m_current_module->flares_2.push_back(flare2);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->flares_2.size() - 1));
}

void Parser::ParseFixes()
{
    m_current_module->fixes.push_back(this->GetArgNodeRef(0));
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->fixes.size() - 1));
}

void Parser::ParseExtCamera()
{
    if (! this->CheckNumArguments(2)) { return; }

    ExtCamera extcam;
    auto mode_str = this->GetArgStr(1);
    if (mode_str == "classic")
    {
        extcam.mode = ExtCamera::MODE_CLASSIC;
    }
    else if (mode_str == "cinecam")
    {
        extcam.mode = ExtCamera::MODE_CINECAM;
    }
    else if ((mode_str == "node") && (m_num_args > 2))
    {
        extcam.mode = ExtCamera::MODE_NODE;
        extcam.node = this->GetArgNodeRef(2);
    }

    m_current_module->extcamera.push_back(extcam);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->extcamera.size() - 1));
}

void Parser::ParseExhaust()
{
    if (! this->CheckNumArguments(2)) { return; }

    Exhaust exhaust;
    exhaust.reference_node = this->GetArgNodeRef(0);
    exhaust.direction_node = this->GetArgNodeRef(1);
    
    // Param [2] is unused
    if (m_num_args > 3) { exhaust.particle_name = this->GetArgStr(3); }

    m_current_module->exhausts.push_back(exhaust);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->exhausts.size() - 1));
}

void Parser::ParseFileFormatVersion()
{
    if (! this->CheckNumArguments(2)) { return; }

    m_current_module->fileformatversion.push_back(this->GetArgUint(1));
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->fileformatversion.size() - 1));

    this->ChangeSection(SECTION_NONE);
}

void Parser::ParseDirectiveDetacherGroup()
{
    if (! this->CheckNumArguments(2)) { return; } // 2 items: keyword, param

    DetacherGroupPreset def;
    def._num_args = m_num_args - 1;
    if (this->GetArgStr(1) == "end")
    {
        def.detacher_group = DEFAULT_DETACHER_GROUP;
        def._end = true;
    }
    else
    {
        def.detacher_group = this->GetArgInt(2);
    }

    m_current_module->detacher_group_preset.push_back(def);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->detacher_group_preset.size() - 1));
}

void Parser::ParseCruiseControl()
{
    if (! this->CheckNumArguments(3)) { return; } // keyword + 2 params

    CruiseControl cruise_control;
    cruise_control.min_speed = this->GetArgFloat(1);
    cruise_control.autobrake = this->GetArgInt(2);

    m_current_module->cruise_control.push_back(cruise_control);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->cruise_control.size() - 1));
}

void Parser::ParseDirectiveAddAnimation()
{
    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line + 14, ","); // "add_animation " = 14 characters

    if (tokens.size() < 4)
    {
        AddMessage(Message::TYPE_ERROR, "Not enough arguments, skipping...");
        return;
    }

    Animation animation;
    animation.ratio       = this->ParseArgFloat(tokens[0].c_str());
    animation.lower_limit = this->ParseArgFloat(tokens[1].c_str());
    animation.upper_limit = this->ParseArgFloat(tokens[2].c_str());

    for (auto itor = tokens.begin() + 3; itor != tokens.end(); ++itor)
    {
        Ogre::StringVector entry = Ogre::StringUtil::split(*itor, ":");
        Ogre::StringUtil::trim(entry[0]);
        if (entry.size() > 1) Ogre::StringUtil::trim(entry[1]); 

        const int WARN_LEN = 500;
        char warn_msg[WARN_LEN] = "";

        if (entry.size() == 1) // Single keyword
        {
            if      (entry[0] == "autoanimate") { animation.mode |= Animation::MODE_AUTO_ANIMATE; }
            else if (entry[0] == "noflip")      { animation.mode |= Animation::MODE_NO_FLIP; }
            else if (entry[0] == "bounce")      { animation.mode |= Animation::MODE_BOUNCE; }
            else if (entry[0] == "eventlock")   { animation.mode |= Animation::MODE_EVENT_LOCK; }

            else { snprintf(warn_msg, WARN_LEN, "Invalid keyword: %s", entry[0].c_str()); }
        }
        else if (entry.size() == 2 && (entry[0] == "mode" || entry[0] == "event" || entry[0] == "source"))
        {
            Ogre::StringVector values = Ogre::StringUtil::split(entry[1], "|");
            if (entry[0] == "mode")
            {
                for (auto itor = values.begin(); itor != values.end(); ++itor)
                {
                    std::string value = *itor;
                    Ogre::StringUtil::trim(value);

                         if (value == "x-rotation") { animation.mode |= Animation::MODE_ROTATION_X; }
                    else if (value == "y-rotation") { animation.mode |= Animation::MODE_ROTATION_Y; }
                    else if (value == "z-rotation") { animation.mode |= Animation::MODE_ROTATION_Z; }
                    else if (value == "x-offset"  ) { animation.mode |= Animation::MODE_OFFSET_X;   }
                    else if (value == "y-offset"  ) { animation.mode |= Animation::MODE_OFFSET_Y;   }
                    else if (value == "z-offset"  ) { animation.mode |= Animation::MODE_OFFSET_Z;   }

                    else { snprintf(warn_msg, WARN_LEN, "Invalid 'mode': %s, ignoring...", entry[1].c_str()); }
                }
            }
            else if (entry[0] == "event")
            {
                animation.event = entry[1];
                Ogre::StringUtil::trim(animation.event);
                Ogre::StringUtil::toUpperCase(animation.event);
            }
            else if (entry[0] == "source")
            {
                for (auto itor = values.begin(); itor != values.end(); ++itor)
                {
                    std::string value = *itor;
                    Ogre::StringUtil::trim(value);

                         if (value == "airspeed")      { animation.source |= Animation::SOURCE_AIRSPEED;          }
                    else if (value == "vvi")           { animation.source |= Animation::SOURCE_VERTICAL_VELOCITY; }
                    else if (value == "altimeter100k") { animation.source |= Animation::SOURCE_ALTIMETER_100K;    }
                    else if (value == "altimeter10k")  { animation.source |= Animation::SOURCE_ALTIMETER_10K;     }
                    else if (value == "altimeter1k")   { animation.source |= Animation::SOURCE_ALTIMETER_1K;      }
                    else if (value == "aoa")           { animation.source |= Animation::SOURCE_ANGLE_OF_ATTACK;   }
                    else if (value == "flap")          { animation.source |= Animation::SOURCE_FLAP;              }
                    else if (value == "airbrake")      { animation.source |= Animation::SOURCE_AIR_BRAKE;         }
                    else if (value == "roll")          { animation.source |= Animation::SOURCE_ROLL;              }
                    else if (value == "pitch")         { animation.source |= Animation::SOURCE_PITCH;             }
                    else if (value == "brakes")        { animation.source |= Animation::SOURCE_BRAKES;            }
                    else if (value == "accel")         { animation.source |= Animation::SOURCE_ACCEL;             }
                    else if (value == "clutch")        { animation.source |= Animation::SOURCE_CLUTCH;            }
                    else if (value == "speedo")        { animation.source |= Animation::SOURCE_SPEEDO;            }
                    else if (value == "tacho")         { animation.source |= Animation::SOURCE_TACHO;             }
                    else if (value == "turbo")         { animation.source |= Animation::SOURCE_TURBO;             }
                    else if (value == "parking")       { animation.source |= Animation::SOURCE_PARKING;           }
                    else if (value == "shifterman1")   { animation.source |= Animation::SOURCE_SHIFT_LEFT_RIGHT;  }
                    else if (value == "shifterman2")   { animation.source |= Animation::SOURCE_SHIFT_BACK_FORTH;  }
                    else if (value == "sequential")    { animation.source |= Animation::SOURCE_SEQUENTIAL_SHIFT;  }
                    else if (value == "shifterlin")    { animation.source |= Animation::SOURCE_SHIFTERLIN;        }
                    else if (value == "torque")        { animation.source |= Animation::SOURCE_TORQUE;            }
                    else if (value == "heading")       { animation.source |= Animation::SOURCE_HEADING;           }
                    else if (value == "difflock")      { animation.source |= Animation::SOURCE_DIFFLOCK;          }
                    else if (value == "rudderboat")    { animation.source |= Animation::SOURCE_BOAT_RUDDER;       }
                    else if (value == "throttleboat")  { animation.source |= Animation::SOURCE_BOAT_THROTTLE;     }
                    else if (value == "steeringwheel") { animation.source |= Animation::SOURCE_STEERING_WHEEL;    }
                    else if (value == "aileron")       { animation.source |= Animation::SOURCE_AILERON;           }
                    else if (value == "elevator")      { animation.source |= Animation::SOURCE_ELEVATOR;          }
                    else if (value == "rudderair")     { animation.source |= Animation::SOURCE_AIR_RUDDER;        }
                    else if (value == "permanent")     { animation.source |= Animation::SOURCE_PERMANENT;         }
                    else if (value == "event")         { animation.source |= Animation::SOURCE_EVENT;             }

                    else
                    {
                        Animation::MotorSource motor_source;
                        if (entry[1].compare(0, 8, "throttle") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_AERO_THROTTLE;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(8));
                        }
                        else if (entry[1].compare(0, 3, "rpm") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_AERO_RPM;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(3));
                        }
                        else if (entry[1].compare(0, 8, "aerotorq") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_AERO_TORQUE;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(8));
                        }
                        else if (entry[1].compare(0, 7, "aeropit") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_AERO_PITCH;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(7));
                        }
                        else if (entry[1].compare(0, 10, "aerostatus") == 0)
                        {
                            motor_source.source = Animation::MotorSource::SOURCE_AERO_STATUS;
                            motor_source.motor = this->ParseArgUint(entry[1].substr(10));
                        }
                        else
                        {
                            snprintf(warn_msg, WARN_LEN, "Invalid 'source': %s, ignoring...", entry[1].c_str());
                            continue;
                        }
                        animation.motor_sources.push_back(motor_source);
                    }
                }
            }
            else
            {
                snprintf(warn_msg, WARN_LEN, "Invalid keyword: %s, ignoring...", entry[0].c_str());
            }
        }
        else
        {
            snprintf(warn_msg, WARN_LEN, "Invalid item: %s, ignoring...", entry[0].c_str());
        }

        if (warn_msg[0] != '\0')
        {
            char msg[WARN_LEN + 100];
            snprintf(msg, WARN_LEN + 100, "Invalid token: %s (%s) ignoring....", itor->c_str(), warn_msg);
            this->AddMessage(Message::TYPE_WARNING, msg);
        }
    }

    m_current_module->add_animation.push_back(animation);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->add_animation.size() - 1));
}

void Parser::ParseAntiLockBrakes()
{
    AntiLockBrakes alb;
    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line + 15, ","); // "AntiLockBrakes " = 15 characters
    if (tokens.size() < 2)
    {
        this->AddMessage(Message::TYPE_ERROR, "Too few arguments for `AntiLockBrakes`");
        return;
    }

    alb.regulation_force = this->ParseArgFloat(tokens[0].c_str());
    alb.min_speed        = this->ParseArgInt  (tokens[1].c_str());

    if (tokens.size() > 3) { alb.pulse_per_sec = this->ParseArgFloat(tokens[2].c_str()); }

    for (unsigned int i=3; i<tokens.size(); i++)
    {
        Ogre::StringVector args2 = Ogre::StringUtil::split(tokens[i], ":");
        Ogre::StringUtil::trim(args2[0]);
        Ogre::StringUtil::toLowerCase(args2[0]);
        if (args2[0] == "mode" && args2.size() == 2)
        {
            Ogre::StringVector attrs = Ogre::StringUtil::split(args2[1], "&");
            auto itor = attrs.begin();
            auto endi = attrs.end();
            for (; itor != endi; ++itor)
            {
                std::string attr = *itor;
                Ogre::StringUtil::trim(attr);
                Ogre::StringUtil::toLowerCase(attr);
                     if (strncmp(attr.c_str(), "nodash", 6)   == 0) { alb.attr_no_dashboard = true;  }
                else if (strncmp(attr.c_str(), "notoggle", 8) == 0) { alb.attr_no_toggle    = true;  }
                else if (strncmp(attr.c_str(), "on", 2)       == 0) { alb.attr_is_on        = true;  }
                else if (strncmp(attr.c_str(), "off", 3)      == 0) { alb.attr_is_on        = false; }
            }
        }
        else
        {
            this->AddMessage(Message::TYPE_ERROR, "Antilockbrakes Mode: missing");
            alb.attr_no_dashboard = false;
            alb.attr_no_toggle = false;
            alb.attr_is_on = true;
        }
    }

    m_current_module->anti_lock_brakes.push_back(alb);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->anti_lock_brakes.size() - 1));
}

void Parser::ParseEngoption()
{
    if (! this->CheckNumArguments(1)) { return; }

    Engoption engoption;
    engoption.inertia = this->GetArgFloat(0);

    if (m_num_args > 1)
    {
        engoption.type = Engoption::EngineType(this->GetArgChar(1));
    }

    if (m_num_args > 2) { engoption.clutch_force     = this->GetArgFloat(2); }
    if (m_num_args > 3) { engoption.shift_time       = this->GetArgFloat(3); }
    if (m_num_args > 4) { engoption.clutch_time      = this->GetArgFloat(4); }
    if (m_num_args > 5) { engoption.post_shift_time  = this->GetArgFloat(5); }
    if (m_num_args > 6) { engoption.stall_rpm        = this->GetArgFloat(6); }
    if (m_num_args > 7) { engoption.idle_rpm         = this->GetArgFloat(7); }
    if (m_num_args > 8) { engoption.max_idle_mixture = this->GetArgFloat(8); }
    if (m_num_args > 9) { engoption.min_idle_mixture = this->GetArgFloat(9); }
    if (m_num_args > 10){ engoption.braking_torque   = this->GetArgFloat(10);}

    m_current_module->engoption.push_back(engoption);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->engoption.size() - 1));
}

void Parser::ParseEngturbo()
{
    if (! this->CheckNumArguments(4)) { return; }

    Engturbo engturbo;
    engturbo.version        = this->GetArgInt  ( 0);
    engturbo.tinertiaFactor = this->GetArgFloat( 1);
    engturbo.nturbos        = this->GetArgInt  ( 2);
    engturbo.param1         = this->GetArgFloat( 3);

    if (m_num_args >  4) { engturbo.param2  = this->GetArgFloat( 4); }
    if (m_num_args >  5) { engturbo.param3  = this->GetArgFloat( 5); }
    if (m_num_args >  6) { engturbo.param4  = this->GetArgFloat( 6); }
    if (m_num_args >  7) { engturbo.param5  = this->GetArgFloat( 7); }
    if (m_num_args >  8) { engturbo.param6  = this->GetArgFloat( 8); }
    if (m_num_args >  9) { engturbo.param7  = this->GetArgFloat( 9); }
    if (m_num_args > 10) { engturbo.param8  = this->GetArgFloat(10); }
    if (m_num_args > 11) { engturbo.param9  = this->GetArgFloat(11); }
    if (m_num_args > 12) { engturbo.param10 = this->GetArgFloat(12); }
    if (m_num_args > 13) { engturbo.param11 = this->GetArgFloat(13); }

    if (engturbo.nturbos > 4)
    {
        this->AddMessage(Message::TYPE_WARNING, "You cannot have more than 4 turbos. Fallback: using 4 instead.");
        engturbo.nturbos = 4;
    }

    m_current_module->engturbo.push_back(engturbo);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->engturbo.size() - 1));
}

void Parser::ParseEngine()
{
    if (! this->CheckNumArguments(6)) { return; }

    Engine engine;
    engine.shift_down_rpm     = this->GetArgFloat(0);
    engine.shift_up_rpm       = this->GetArgFloat(1);
    engine.torque             = this->GetArgFloat(2);
    engine.global_gear_ratio  = this->GetArgFloat(3);
    engine.reverse_gear_ratio = this->GetArgFloat(4);
    engine.neutral_gear_ratio = this->GetArgFloat(5);

    // Forward gears
    for (int i = 6; i < m_num_args; i++)
    {
        float ratio = this->GetArgFloat(i);
        if (ratio < 0.f)
        {
            break; // Optional terminator argument
        }
        engine.gear_ratios.push_back(ratio);   
    }

    if (engine.gear_ratios.size() == 0)
    {
        AddMessage(Message::TYPE_ERROR, "Engine has no forward gear, ignoring...");
        return;
    }

    m_current_module->engine.push_back(engine);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->engine.size() - 1));
}

void Parser::ParseContacter()
{
    if (! this->CheckNumArguments(1)) { return; }

    m_current_module->contacters.push_back(this->GetArgNodeRef(0));
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->contacters.size() - 1));
}

void Parser::ParseCommandsUnified()
{
    const bool is_commands2 = (m_current_section == SECTION_COMMANDS_2);
    const int max_args = (is_commands2 ? 8 : 7);
    if (! this->CheckNumArguments(max_args)) { return; }

    Command2 command2;

    command2._format_version   = (is_commands2) ? 2 : 1;
    int pos = 0;
    command2.nodes[0]          = this->GetArgNodeRef(pos++);
    command2.nodes[1]          = this->GetArgNodeRef(pos++);
    command2.shorten_rate      = this->GetArgFloat  (pos++);

    if (is_commands2)
    {
        command2.lengthen_rate = this->GetArgFloat(pos++);
    }
    else
    {
        command2.lengthen_rate = command2.shorten_rate;
    }

    command2.max_contraction = this->GetArgFloat(pos++);
    command2.max_extension   = this->GetArgFloat(pos++);
    command2.contract_key    = this->GetArgInt  (pos++);
    command2.extend_key      = this->GetArgInt  (pos++);

    if (m_num_args <= max_args) // No more args?
    {
        m_current_module->commands_2.push_back(command2);
        return;
    }

    // Parse options
    const int WARN_LEN = 200;
    char warn_msg[WARN_LEN] = "";
    std::string options_str = this->GetArgStr(pos++);
    char winner = 0;
    for (auto itor = options_str.begin(); itor != options_str.end(); ++itor)
    {
        const char c = *itor;
        if ((winner == 0) && (c == 'o' || c == 'p' || c == 'c')) { winner = c; }
        
             if (c == 'n') {} // Filler, does nothing
        else if (c == 'i') { command2.option_i_invisible     = true; }
        else if (c == 'r') { command2.option_r_rope          = true; }
        else if (c == 'f') { command2.option_f_not_faster    = true; }
        else if (c == 'c') { command2.option_c_auto_center   = true; }
        else if (c == 'p') { command2.option_p_1press        = true; }
        else if (c == 'o') { command2.option_o_1press_center = true; }
        else
        {
            snprintf(warn_msg, WARN_LEN, "Ignoring unknown flag '%c'", c);
            this->AddMessage(Message::TYPE_WARNING, warn_msg);
        }
    }

    // Resolve option conflicts
    if (command2.option_c_auto_center && winner != 'c' && winner != 0)
    {
        AddMessage(Message::TYPE_WARNING, "Command cannot be one-pressed and self centering at the same time, ignoring flag 'c'");
        command2.option_c_auto_center = false;
    }
    char ignored = '\0';
    if (command2.option_o_1press_center && winner != 'o' && winner != 0)
    {
        command2.option_o_1press_center = false;
        ignored = 'o';
    }
    else if (command2.option_p_1press && winner != 'p' && winner != 0)
    {
        command2.option_p_1press = false;
        ignored = 'p';
    }

    // Report conflicts
    if (ignored != 0 && winner == 'c')
    {
        snprintf(warn_msg, WARN_LEN, "Command cannot be one-pressed and self centering at the same time, ignoring flag '%c'", ignored);
        AddMessage(Message::TYPE_WARNING, warn_msg);
    }
    else if (ignored != 0 && (winner == 'o' || winner == 'p'))
    {
        snprintf(warn_msg, WARN_LEN, "Command already has a one-pressed c.mode, ignoring flag '%c'", ignored);
        AddMessage(Message::TYPE_WARNING, warn_msg);
    }

    if (m_num_args > pos) { command2.description   = this->GetArgStr  (pos++);}

    if (m_num_args > pos) { ParseOptionalInertia(command2.inertia, pos); pos += 4; }

    if (m_num_args > pos) { command2.affect_engine = this->GetArgFloat(pos++);}
    if (m_num_args > pos) { command2.needs_engine  = this->GetArgBool (pos++);}
    if (m_num_args > pos) { command2.plays_sound   = this->GetArgBool (pos++);}

    m_current_module->commands_2.push_back(command2);
    m_current_module->sequence.push_back(SeqSection(m_current_section, (int)m_current_module->commands_2.size() - 1));
}

void Parser::ParseCollisionBox()
{
    CollisionBox collisionbox;

    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line, ",");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        collisionbox.nodes.push_back( this->_ParseNodeRef(*iter) );
    }

    m_current_module->collision_boxes.push_back(collisionbox);
    m_current_module->sequence.push_back(SeqSection(m_current_section, (int)m_current_module->collision_boxes.size() - 1));
}

void Parser::ParseCinecam()
{
    if (! this->CheckNumArguments(11)) { return; }

    Cinecam cinecam;

    // Required arguments
    cinecam.position.x = this->GetArgFloat  ( 0);
    cinecam.position.y = this->GetArgFloat  ( 1);
    cinecam.position.z = this->GetArgFloat  ( 2);
    cinecam.nodes[0]   = this->GetArgNodeRef( 3);
    cinecam.nodes[1]   = this->GetArgNodeRef( 4);
    cinecam.nodes[2]   = this->GetArgNodeRef( 5);
    cinecam.nodes[3]   = this->GetArgNodeRef( 6);
    cinecam.nodes[4]   = this->GetArgNodeRef( 7);
    cinecam.nodes[5]   = this->GetArgNodeRef( 8);
    cinecam.nodes[6]   = this->GetArgNodeRef( 9);
    cinecam.nodes[7]   = this->GetArgNodeRef(10);

    // Optional arguments
    if (m_num_args > 11) { cinecam.spring    = this->GetArgFloat(11); }
    if (m_num_args > 12) { cinecam.damping   = this->GetArgFloat(12); }

    if (m_num_args > 13)
    {
        float value = this->GetArgFloat(13);
        if (value > 0.f) // Invalid input (for example illegal trailing ";pseudo-comment") parses as 0
            cinecam.node_mass = value;
    }

    m_current_module->cinecam.push_back(cinecam);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->cinecam.size() - 1));
}

void Parser::ParseCameraRails()
{
   m_current_module->camera_rails.back().nodes.push_back( this->GetArgNodeRef(0) );
}

void Parser::ParseBrakes()
{
    if (!this->CheckNumArguments(1)) { return; }

    Brakes b;
    b.default_braking_force = this->GetArgFloat(0);
    if (m_num_args > 1) b.parking_brake_force = this->GetArgFloat(1);

    m_current_module->brakes.push_back(b);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->brakes.size() - 1));
}

void Parser::ParseAxles()
{
    Axle axle;

    Ogre::StringVector tokens = Ogre::StringUtil::split(m_current_line, ",");
    Ogre::StringVector::iterator iter = tokens.begin();
    for ( ; iter != tokens.end(); iter++)
    {
        std::smatch results;
        if (! std::regex_search(*iter, results, Regexes::SECTION_AXLES_PROPERTY))
        {
            this->AddMessage(Message::TYPE_ERROR, "Invalid property, ignoring whole line...");
            return;
        }
        // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

        if (results[1].matched)
        {
            unsigned int wheel_index = STR_PARSE_INT(results[2]) - 1;
            axle.wheels[wheel_index][0] = _ParseNodeRef(results[3]);
            axle.wheels[wheel_index][1] = _ParseNodeRef(results[4]);
        }
        else if (results[5].matched)
        {
            std::string options_str = results[6].str();
            for (unsigned int i = 0; i < options_str.length(); i++)
            {
                switch(options_str.at(i))
                {
                    case 'o':
                        axle.options.push_back(DifferentialType::DIFF_o_OPEN);
                        break;
                    case 'l':
                        axle.options.push_back(DifferentialType::DIFF_l_LOCKED);
                        break;
                    case 's':
                        axle.options.push_back(DifferentialType::DIFF_s_SPLIT);
                        break;
                    case 'v':
                        axle.options.push_back(DifferentialType::DIFF_v_VISCOUS);
                        break;

                    default: // No check needed, regex takes care of that 
                        break;
                }
            }
        }
    }

    m_current_module->axles.push_back(axle);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->axles.size() - 1));
}

void Parser::ParseInterAxles()
{
    auto args = Ogre::StringUtil::split(m_current_line, ",");
    if (args.size() < 2) { return; }

    InterAxle interaxle;

    interaxle.a1 = this->ParseArgInt(args[0].c_str()) - 1;
    interaxle.a2 = this->ParseArgInt(args[1].c_str()) - 1;

    std::smatch results;
    if (! std::regex_search(args[2], results, Regexes::SECTION_AXLES_PROPERTY))
    {
        this->AddMessage(Message::TYPE_ERROR, "Invalid property, ignoring whole line...");
        return;
    }
    // NOTE: Positions in 'results' array match E_CAPTURE*() positions (starting with 1) in the respective regex. 

    if (results[5].matched)
    {
        std::string options_str = results[6].str();
        for (unsigned int i = 0; i < options_str.length(); i++)
        {
            switch(options_str.at(i))
            {
                case 'o':
                    interaxle.options.push_back(DifferentialType::DIFF_o_OPEN);
                    break;
                case 'l':
                    interaxle.options.push_back(DifferentialType::DIFF_l_LOCKED);
                    break;
                case 's':
                    interaxle.options.push_back(DifferentialType::DIFF_s_SPLIT);
                    break;
                case 'v':
                    interaxle.options.push_back(DifferentialType::DIFF_v_VISCOUS);
                    break;

                default: // No check needed, regex takes care of that 
                    break;
            }
        }
    }

    m_current_module->interaxles.push_back(interaxle);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->interaxles.size() - 1));
}

void Parser::ParseAirbrakes()
{
    if (! this->CheckNumArguments(14)) { return; }

    Airbrake airbrake;
    airbrake.reference_node        = this->GetArgNodeRef( 0);
    airbrake.x_axis_node           = this->GetArgNodeRef( 1);
    airbrake.y_axis_node           = this->GetArgNodeRef( 2);
    airbrake.aditional_node        = this->GetArgNodeRef( 3);
    airbrake.offset.x              = this->GetArgFloat  ( 4);
    airbrake.offset.y              = this->GetArgFloat  ( 5);
    airbrake.offset.z              = this->GetArgFloat  ( 6);
    airbrake.width                 = this->GetArgFloat  ( 7);
    airbrake.height                = this->GetArgFloat  ( 8);
    airbrake.max_inclination_angle = this->GetArgFloat  ( 9);
    airbrake.texcoord_x1           = this->GetArgFloat  (10);
    airbrake.texcoord_y1           = this->GetArgFloat  (11);
    airbrake.texcoord_x2           = this->GetArgFloat  (12);
    airbrake.texcoord_y2           = this->GetArgFloat  (13);

    m_current_module->airbrakes.push_back(airbrake);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->airbrakes.size() - 1));
}

void Parser::ParseVideoCamera()
{
    if (! this->CheckNumArguments(19)) { return; }

    VideoCamera videocamera;

    videocamera.reference_node       = this->GetArgNodeRef     ( 0);
    videocamera.left_node            = this->GetArgNodeRef     ( 1);
    videocamera.bottom_node          = this->GetArgNodeRef     ( 2);
    videocamera.alt_reference_node   = this->GetArgNullableNode( 3);
    videocamera.alt_orientation_node = this->GetArgNullableNode( 4);
    videocamera.offset.x             = this->GetArgFloat       ( 5);
    videocamera.offset.y             = this->GetArgFloat       ( 6);
    videocamera.offset.z             = this->GetArgFloat       ( 7);
    videocamera.rotation.x           = this->GetArgFloat       ( 8);
    videocamera.rotation.y           = this->GetArgFloat       ( 9);
    videocamera.rotation.z           = this->GetArgFloat       (10);
    videocamera.field_of_view        = this->GetArgFloat       (11);
    videocamera.texture_width        = this->GetArgInt         (12);
    videocamera.texture_height       = this->GetArgInt         (13);
    videocamera.min_clip_distance    = this->GetArgFloat       (14);
    videocamera.max_clip_distance    = this->GetArgFloat       (15);
    videocamera.camera_role          = this->GetArgInt         (16);
    videocamera.camera_mode          = this->GetArgInt         (17);
    videocamera.material_name        = this->GetArgStr         (18);

    if (m_num_args > 19) { videocamera.camera_name = this->GetArgStr(19); }

    m_current_module->videocameras.push_back(videocamera);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->videocameras.size() - 1));
}

void Parser::ParseCameras()
{
    if (! this->CheckNumArguments(3)) { return; }

    Camera camera;
    camera.center_node = this->GetArgNodeRef(0);
    camera.back_node   = this->GetArgNodeRef(1);
    camera.left_node   = this->GetArgNodeRef(2);

    m_current_module->cameras.push_back(camera);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->cameras.size() - 1));
}

void Parser::ParseTurbopropsUnified()
{
    bool is_turboprop_2 = m_current_section == SECTION_TURBOPROPS_2;

    if (! this->CheckNumArguments(is_turboprop_2 ? 9 : 8)) { return; }

    Turboprop2 turboprop;
    
    turboprop.reference_node     = this->GetArgNodeRef(0);
    turboprop.axis_node          = this->GetArgNodeRef(1);
    turboprop.blade_tip_nodes[0] = this->GetArgNodeRef(2);
    turboprop.blade_tip_nodes[1] = this->GetArgNodeRef(3);
    turboprop.blade_tip_nodes[2] = this->GetArgNullableNode(4);
    turboprop.blade_tip_nodes[3] = this->GetArgNullableNode(5);

    int offset = 0;

    if (is_turboprop_2)
    {
        turboprop.couple_node = this->GetArgNullableNode(6);

        offset = 1;
    }

    turboprop.turbine_power_kW   = this->GetArgFloat  (6 + offset);
    turboprop.airfoil            = this->GetArgStr    (7 + offset);
    
    m_current_module->turboprops_2.push_back(turboprop);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->turboprops_2.size() - 1));
}

void Parser::ParseTurbojets()
{
    if (! this->CheckNumArguments(9)) { return; }

    Turbojet turbojet;
    turbojet.front_node     = this->GetArgNodeRef(0);
    turbojet.back_node      = this->GetArgNodeRef(1);
    turbojet.side_node      = this->GetArgNodeRef(2);
    turbojet.is_reversable  = this->GetArgInt    (3);
    turbojet.dry_thrust     = this->GetArgFloat  (4);
    turbojet.wet_thrust     = this->GetArgFloat  (5);
    turbojet.front_diameter = this->GetArgFloat  (6);
    turbojet.back_diameter  = this->GetArgFloat  (7);
    turbojet.nozzle_length  = this->GetArgFloat  (8);

    m_current_module->turbojets.push_back(turbojet);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->turbojets.size() - 1));
}

void Parser::ParseTriggers()
{
    if (! this->CheckNumArguments(6)) { return; }

    Trigger trigger;

    trigger.nodes[0]                  = this->GetArgNodeRef(0);
    trigger.nodes[1]                  = this->GetArgNodeRef(1);
    trigger.contraction_trigger_limit = this->GetArgFloat  (2);
    trigger.expansion_trigger_limit   = this->GetArgFloat  (3);
    
    int shortbound_trigger_action = this->GetArgInt(4); 
    int longbound_trigger_action  = this->GetArgInt(5); 
    if (m_num_args > 6)
    {
        std::string options_str = this->GetArgStr(6);
        for (unsigned int i = 0; i < options_str.length(); i++)
        {
            switch(options_str.at(i))
            {
                case 'i': trigger.options |= Trigger::OPTION_i_INVISIBLE;             break;
                case 'c': trigger.options |= Trigger::OPTION_c_COMMAND_STYLE;         break;
                case 'x': trigger.options |= Trigger::OPTION_x_START_OFF;             break;
                case 'b': trigger.options |= Trigger::OPTION_b_BLOCK_KEYS;            break;
                case 'B': trigger.options |= Trigger::OPTION_B_BLOCK_TRIGGERS;        break;
                case 'A': trigger.options |= Trigger::OPTION_A_INV_BLOCK_TRIGGERS;    break;
                case 's': trigger.options |= Trigger::OPTION_s_SWITCH_CMD_NUM;        break;
                case 'h': trigger.options |= Trigger::OPTION_h_UNLOCK_HOOKGROUPS_KEY; break;
                case 'H': trigger.options |= Trigger::OPTION_H_LOCK_HOOKGROUPS_KEY;   break;
                case 't': trigger.options |= Trigger::OPTION_t_CONTINUOUS;            break;
                case 'E': trigger.options |= Trigger::OPTION_E_ENGINE_TRIGGER;        break;

                default:
                    this->AddMessage(Message::TYPE_WARNING, Ogre::String("Invalid trigger option: " + options_str.at(i)));
            }
        }
    }

    if (m_num_args > 7)
    {
        float boundary_timer = this->GetArgFloat(7);
        if (boundary_timer > 0.0f)
            trigger.boundary_timer = boundary_timer;
    }

    // Handle actions
    if (trigger.IsHookToggleTrigger())
    {
        Trigger::HookToggleTrigger hook_toggle;
        hook_toggle.contraction_trigger_hookgroup_id = shortbound_trigger_action;
        hook_toggle.extension_trigger_hookgroup_id = longbound_trigger_action;
        trigger.SetHookToggleTrigger(hook_toggle);
    }
    else if (trigger.HasFlag_E_EngineTrigger())
    {
        Trigger::EngineTrigger engine_trigger;
        engine_trigger.function = Trigger::EngineTrigger::Function(shortbound_trigger_action);
        engine_trigger.motor_index = longbound_trigger_action;
        trigger.SetEngineTrigger(engine_trigger);
    }
    else
    {
        Trigger::CommandKeyTrigger command_keys;
        command_keys.contraction_trigger_key = shortbound_trigger_action;
        command_keys.extension_trigger_key   = longbound_trigger_action;
        trigger.SetCommandKeyTrigger(command_keys);
    }

    m_current_module->triggers.push_back(trigger);
    m_current_module->sequence.push_back(SeqSection(m_current_section, (int)m_current_module->triggers.size() - 1));
}

void Parser::ParseTorqueCurve()
{
    if (m_current_module->torque_curve.size() == 0)
    {
        m_current_module->torque_curve.push_back(Truck::TorqueCurve());
    }

    Ogre::StringVector args = Ogre::StringUtil::split(m_current_line, ",");
    
    if (args.size() == 1u)
    {
        m_current_module->torque_curve.back().predefined_func_name = args[0];
    }
    else if (args.size() == 2u)
    {
        TorqueCurve::Sample sample;
        sample.power          = this->ParseArgFloat(args[0].c_str());
        sample.torque_percent = this->ParseArgFloat(args[1].c_str());
        m_current_module->torque_curve.back().samples.push_back(sample);
    }
    else
    {
        // Consistent with 0.38's parser.
        this->AddMessage(Message::TYPE_ERROR, "Invalid line, too many arguments");   
    }
}

void Parser::ParseTies()
{
    if (! this->CheckNumArguments(5)) { return; }

    Tie tie;

    tie.root_node         = this->GetArgNodeRef(0);
    tie.max_reach_length  = this->GetArgFloat  (1);
    tie.auto_shorten_rate = this->GetArgFloat  (2);
    tie.min_length        = this->GetArgFloat  (3);
    tie.max_length        = this->GetArgFloat  (4);

    if (m_num_args > 5)
    {
        for (char c: this->GetArgStr(5))
        {
            switch (c)
            {
            case Tie::OPTION_n_FILLER:
            case Tie::OPTION_v_FILLER:
                break;

            case Tie::OPTION_i_INVISIBLE:
                tie.is_invisible = true;
                break;

            case Tie::OPTION_s_NO_SELF_LOCK:
                tie.disable_self_lock = true;
                break;

            default:
                this->AddMessage(Message::TYPE_WARNING, std::string("Invalid option: ") + c + ", ignoring...");
                break;
            }
        }
    }

    if (m_num_args > 6) { tie.max_stress   =  this->GetArgFloat (6); }
    if (m_num_args > 7) { tie.group        =  this->GetArgInt   (7); }

    m_current_module->ties.push_back(tie);
    m_current_module->sequence.push_back(SeqSection(m_current_section, (int)m_current_module->ties.size() - 1));
}

void Parser::ParseSoundsources()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    SoundSource soundsource;
    soundsource.node              = this->GetArgNodeRef(0);
    soundsource.sound_script_name = this->GetArgStr(1);

    m_current_module->soundsources.push_back(soundsource);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->soundsources.size() - 1));
}

void Parser::ParseSoundsources2()
{
    if (! this->CheckNumArguments(3)) { return; }
    
    SoundSource2 soundsource2;
    soundsource2.node              = this->GetArgNodeRef(0);
    soundsource2.sound_script_name = this->GetArgStr(2);

    int mode = this->GetArgInt(1);
    if (mode < 0)
    {
        if (mode < -2)
        {
            std::string msg = this->GetArgStr(1) + " is invalid soundsources2.mode, falling back to default -2";
            this->AddMessage(Message::TYPE_ERROR, msg.c_str());
            mode = -2;
        }
        soundsource2.mode = SoundSource2::Mode(mode);
    }
    else
    {
        soundsource2.mode = SoundSource2::MODE_CINECAM;
        soundsource2.cinecam_index = mode;
    }

    m_current_module->soundsources2.push_back(soundsource2);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->soundsources2.size() - 1));
}

void Parser::ParseSlidenodes()
{
    Ogre::StringVector args = Ogre::StringUtil::split(m_current_line, ", ");
    if (args.size() < 2u)
    {
        this->AddMessage(Message::TYPE_ERROR, "Too few arguments");
    }

    SlideNode slidenode;
    slidenode.slide_node = this->_ParseNodeRef(args[0]);
    
    bool in_rail_node_list = true;

    for (auto itor = args.begin() + 1; itor != args.end(); ++itor)
    {
        char c = toupper(itor->at(0));
        switch (c)
        {
        case 'S':
            slidenode.spring_rate = this->ParseArgFloat(itor->substr(1));
            slidenode._spring_rate_set = true;
            in_rail_node_list = false;
            break;
        case 'B':
            slidenode.break_force = this->ParseArgFloat(itor->substr(1));
            slidenode._break_force_set = true;
            in_rail_node_list = false;
            break;
        case 'T':
            slidenode.tolerance = this->ParseArgFloat(itor->substr(1));
            slidenode._tolerance_set = true;
            in_rail_node_list = false;
            break;
        case 'R':
            slidenode.attachment_rate = this->ParseArgFloat(itor->substr(1));
            slidenode._attachment_rate_set = true;
            in_rail_node_list = false;
            break;
        case 'G':
            slidenode.railgroup_id = this->ParseArgFloat(itor->substr(1));
            slidenode._railgroup_id_set = true;
            in_rail_node_list = false;
            break;
        case 'D':
            slidenode.max_attach_dist = this->ParseArgFloat(itor->substr(1));
            slidenode._max_attach_dist_set = true;
            in_rail_node_list = false;
            break;
        case 'C':
            switch (itor->at(1))
            {
            case 'a':
                BITMASK_SET_1(slidenode.constraint_flags, SlideNode::CONSTRAINT_ATTACH_ALL);
                break;
            case 'f':
                BITMASK_SET_1(slidenode.constraint_flags, SlideNode::CONSTRAINT_ATTACH_FOREIGN);
                break;
            case 's':
                BITMASK_SET_1(slidenode.constraint_flags, SlideNode::CONSTRAINT_ATTACH_SELF);
                break;
            case 'n':
                BITMASK_SET_1(slidenode.constraint_flags, SlideNode::CONSTRAINT_ATTACH_NONE);
                break;
            default:
                this->AddMessage(Message::TYPE_WARNING, std::string("Ignoring invalid option: ") + itor->at(1));
                break;
            }
            in_rail_node_list = false;
            break;
        default:
            if (in_rail_node_list)
                slidenode.rail_node_ranges.push_back( _ParseNodeRef(*itor));
            break;
        }
    }

    slidenode.editor_group_id = this->GetCurrentEditorGroup();
    m_current_module->slidenodes.push_back(slidenode);
}

void Parser::ParseShock3()
{
    if (! this->CheckNumArguments(15)) { return; }

    Shock3 shock_3;

    shock_3.nodes[0]       = this->GetArgNodeRef( 0);
    shock_3.nodes[1]       = this->GetArgNodeRef( 1);
    shock_3.spring_in      = this->GetArgFloat  ( 2);
    shock_3.damp_in        = this->GetArgFloat  ( 3);
    shock_3.damp_in_slow   = this->GetArgFloat  ( 4);
    shock_3.split_vel_in   = this->GetArgFloat  ( 5);
    shock_3.damp_in_fast   = this->GetArgFloat  ( 6);
    shock_3.spring_out     = this->GetArgFloat  ( 7);
    shock_3.damp_out       = this->GetArgFloat  ( 8);
    shock_3.damp_out_slow  = this->GetArgFloat  ( 9);
    shock_3.split_vel_out  = this->GetArgFloat  (10);
    shock_3.damp_out_fast  = this->GetArgFloat  (11);
    shock_3.short_bound    = this->GetArgFloat  (12);
    shock_3.long_bound     = this->GetArgFloat  (13);
    shock_3.precompression = this->GetArgFloat  (14);

    shock_3.options = 0u;
    if (m_num_args > 15)
    {
        std::string options_str = this->GetArgStr(15);
        auto itor = options_str.begin();
        auto endi = options_str.end();
        while (itor != endi)
        {
            char c = *itor++; // ++
            switch (c)
            {
                case 'n': 
                case 'v': 
                    break; // Placeholder, does nothing.
                case 'i': BITMASK_SET_1(shock_3.options, Shock3::OPTION_i_INVISIBLE);
                    break;
                case 'm': BITMASK_SET_1(shock_3.options, Shock3::OPTION_m_METRIC);
                    break;
                case 'M': BITMASK_SET_1(shock_3.options, Shock3::OPTION_M_ABSOLUTE_METRIC);
                    break;
                default: {
                        char msg[100] = "";
                        snprintf(msg, 100, "Invalid option: '%c', ignoring...", c);
                        AddMessage(Message::TYPE_WARNING, msg);
                    }
                    break;
            }
        }
    }

    m_current_module->shocks_3.push_back(shock_3);
    m_current_module->sequence.push_back(SeqSection(m_current_section, (int)m_current_module->shocks_3.size() - 1));
}

void Parser::ParseShock2()
{
    if (! this->CheckNumArguments(13)) { return; }

    Shock2 shock_2;

    shock_2.nodes[0]                   = this->GetArgNodeRef( 0);
    shock_2.nodes[1]                   = this->GetArgNodeRef( 1);
    shock_2.spring_in                  = this->GetArgFloat  ( 2);
    shock_2.damp_in                    = this->GetArgFloat  ( 3);
    shock_2.progress_factor_spring_in  = this->GetArgFloat  ( 4);
    shock_2.progress_factor_damp_in    = this->GetArgFloat  ( 5);
    shock_2.spring_out                 = this->GetArgFloat  ( 6);
    shock_2.damp_out                   = this->GetArgFloat  ( 7);
    shock_2.progress_factor_spring_out = this->GetArgFloat  ( 8);
    shock_2.progress_factor_damp_out   = this->GetArgFloat  ( 9);
    shock_2.short_bound                = this->GetArgFloat  (10);
    shock_2.long_bound                 = this->GetArgFloat  (11);
    shock_2.precompression             = this->GetArgFloat  (12);

    shock_2.options = 0u;
    if (m_num_args > 13)
    {
        std::string options_str = this->GetArgStr(13);
        auto itor = options_str.begin();
        auto endi = options_str.end();
        while (itor != endi)
        {
            char c = *itor++; // ++
            switch (c)
            {
                case 'n': 
                case 'v': 
                    break; // Placeholder, does nothing.
                case 'i': BITMASK_SET_1(shock_2.options, Shock2::OPTION_i_INVISIBLE);
                    break;
                case 'm': BITMASK_SET_1(shock_2.options, Shock2::OPTION_m_METRIC);
                    break;
                case 'M': BITMASK_SET_1(shock_2.options, Shock2::OPTION_M_ABSOLUTE_METRIC);
                    break;
                case 's': BITMASK_SET_1(shock_2.options, Shock2::OPTION_s_SOFT_BUMP_BOUNDS);
                    break;
                default: {
                        char msg[100] = "";
                        snprintf(msg, 100, "Invalid option: '%c', ignoring...", c);
                        AddMessage(Message::TYPE_WARNING, msg);
                    }
                    break;
            }
        }
    }

    m_current_module->shocks_2.push_back(shock_2);
    m_current_module->sequence.push_back(SeqSection(m_current_section, (int)m_current_module->shocks_2.size() - 1));
}

void Parser::ParseShock()
{
    if (! this->CheckNumArguments(7)) { return; }

    Shock shock;

    shock.nodes[0]       = this->GetArgNodeRef(0);
    shock.nodes[1]       = this->GetArgNodeRef(1);
    shock.spring_rate    = this->GetArgFloat  (2);
    shock.damping        = this->GetArgFloat  (3);
    shock.short_bound    = this->GetArgFloat  (4);
    shock.long_bound     = this->GetArgFloat  (5);
    shock.precompression = this->GetArgFloat  (6);

    shock.options = 0u;
    if (m_num_args > 7)
    {
        std::string options_str = this->GetArgStr(7);
        auto itor = options_str.begin();
        auto endi = options_str.end();
        while (itor != endi)
        {
            char c = *itor++;
            switch (c)
            {
                case 'n':
                case 'v':
                    break; // Placeholder, does nothing.
                case 'i': BITMASK_SET_1(shock.options, Shock::OPTION_i_INVISIBLE);
                    break;
                case 'm': BITMASK_SET_1(shock.options, Shock::OPTION_m_METRIC);
                    break;
                case 'r':
                case 'R': BITMASK_SET_1(shock.options, Shock::OPTION_R_ACTIVE_RIGHT);
                    break;
                case 'l':
                case 'L': BITMASK_SET_1(shock.options, Shock::OPTION_L_ACTIVE_LEFT);
                    break;
                default: {
                    char msg[100] = "";
                    snprintf(msg, 100, "Invalid option: '%c', ignoring...", c);
                    AddMessage(Message::TYPE_WARNING, msg);
                }
                break;
            }
        }
    }
    m_current_module->shocks.push_back(shock);
    m_current_module->sequence.push_back(SeqSection(m_current_section, (int)m_current_module->shocks.size() - 1));
}

void Parser::_CheckInvalidTrailingText(Ogre::String const & line, std::smatch const & results, unsigned int index)
{
    if (results[index].matched) // Invalid trailing text 
    {
        std::stringstream msg;
        msg << "Invalid text after parameters: '" << results[index] << "'. Please remove. Ignoring...";
        AddMessage(line, Message::TYPE_WARNING, msg.str());
    }
}

Node::Ref Parser::_ParseNodeRef(std::string const & node_id_str)
{
    int node_id_num = STR_PARSE_INT(node_id_str);
    if (node_id_num < 0)
    {
        Str<2000> msg;
        msg << "Invalid negative node number " << node_id_num << ", parsing as " << (node_id_num*-1) << " for backwards compatibility";
        AddMessage(node_id_str, Message::TYPE_WARNING, msg.ToCStr());
        node_id_num *= -1;
    }
    return Node::Ref(node_id_num, node_id_str);
}

void Parser::ParseDirectiveSetDefaultMinimass()
{
    if (! this->CheckNumArguments(2)) { return; } // Directive name + parameter

    m_current_module->default_minimass.push_back(this->GetArgFloat(1));
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->default_minimass.size() - 1));
}

void Parser::ParseDirectiveSetInertiaDefaults()
{
    if (! this->CheckNumArguments(2)) { return; }

    InertiaDefaults inertia;
    inertia._num_args = m_num_args;

    inertia.start_delay_factor = this->GetArgFloat(1);
    if (m_num_args > 3) { inertia.stop_delay_factor = this->GetArgFloat(2); }
    if (m_num_args > 3) { inertia.start_function = this->GetArgStr(3); }
    if (m_num_args > 4) { inertia.stop_function  = this->GetArgStr(4); }

    m_current_module->inertia_defaults.push_back(inertia);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->inertia_defaults.size() - 1));
}

void Parser::ParseScrewprops()
{
    if (! this->CheckNumArguments(4)) { return; }
    
    Screwprop screwprop;

    screwprop.prop_node = this->GetArgNodeRef(0);
    screwprop.back_node = this->GetArgNodeRef(1);
    screwprop.top_node  = this->GetArgNodeRef(2);
    screwprop.power     = this->GetArgFloat  (3);

    m_current_module->screwprops.push_back(screwprop);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->screwprops.size() - 1));
}

void Parser::ParseSectionconfig()
{
    if (! this->CheckNumArguments(3)) { return; }
    // arg 0: 'sectionconfig'
    // arg 1: version (unused)
    m_definition->sectionconfig.push_back(this->GetArgStr(2));
}

void Parser::ParseRotatorsUnified()
{
    if (! this->CheckNumArguments(13)) { return; }

    Rotator2 rotator;
    
    rotator.axis_nodes[0]           = this->GetArgNodeRef( 0);
    rotator.axis_nodes[1]           = this->GetArgNodeRef( 1);
    rotator.base_plate_nodes[0]     = this->GetArgNodeRef( 2);
    rotator.base_plate_nodes[1]     = this->GetArgNodeRef( 3);
    rotator.base_plate_nodes[2]     = this->GetArgNodeRef( 4);
    rotator.base_plate_nodes[3]     = this->GetArgNodeRef( 5);
    rotator.rotating_plate_nodes[0] = this->GetArgNodeRef( 6);
    rotator.rotating_plate_nodes[1] = this->GetArgNodeRef( 7);
    rotator.rotating_plate_nodes[2] = this->GetArgNodeRef( 8);
    rotator.rotating_plate_nodes[3] = this->GetArgNodeRef( 9);
    rotator.rate                    = this->GetArgFloat  (10);
    rotator.spin_left_key           = this->GetArgInt    (11);
    rotator.spin_right_key          = this->GetArgInt    (12);
    
    int offset = 0;

    if (m_current_section == SECTION_ROTATORS_2)
    {
        if (! this->CheckNumArguments(16)) { return; }
        if (m_num_args > 13) { rotator.rotating_force  = this->GetArgFloat(13); }
        if (m_num_args > 14) { rotator.tolerance       = this->GetArgFloat(14); }
        if (m_num_args > 15) { rotator.description     = this->GetArgStr  (15); }

        offset = 3;
    }

    this->ParseOptionalInertia(rotator.inertia, 13 + offset);
    if (m_num_args > 17 + offset) { rotator.engine_coupling = this->GetArgFloat(17 + offset); }
    if (m_num_args > 18 + offset) { rotator.needs_engine    = this->GetArgBool (18 + offset); }

    if (m_current_section == SECTION_ROTATORS_2)
    {
        m_current_module->rotators_2.push_back(rotator);
        m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->rotators_2.size() - 1));
    }
    else
    {
        m_current_module->rotators.push_back(rotator);
        m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->rotators.size() - 1));
    }
}

void Parser::ParseFileinfo()
{
    if (! this->CheckNumArguments(2)) { return; }

    FileInfo fileinfo;

    fileinfo.unique_id = this->GetArgStr(1);
    Ogre::StringUtil::trim(fileinfo.unique_id);

    if (m_num_args > 2) { fileinfo.category_id  = this->GetArgInt(2); }
    if (m_num_args > 3) { fileinfo.file_version = this->GetArgInt(3); }

    m_current_module->fileinfo.push_back(fileinfo);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->fileinfo.size() - 1));

    this->ChangeSection(SECTION_NONE);
}

void Parser::ParseRopes()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    Rope rope;
    // Args
    rope.root_node      = this->GetArgNodeRef(0);
    rope.end_node       = this->GetArgNodeRef(1);
    
    if (m_num_args > 2) { rope.invisible  = (this->GetArgChar(2) == 'i'); }

    m_current_module->ropes.push_back(rope);
    m_current_module->sequence.push_back(SeqSection(m_current_section, (int)m_current_module->ropes.size() - 1));
}

void Parser::ParseRopables()
{
    if (! this->CheckNumArguments(1)) { return; }

    Ropable ropable;
    ropable.node = this->GetArgNodeRef(0);
    
    if (m_num_args > 1) { ropable.group         =  this->GetArgInt(1); }
    if (m_num_args > 2) { ropable.has_multilock = (this->GetArgInt(2) == 1); }

    m_current_module->ropables.push_back(ropable);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->ropables.size() - 1));
}

void Parser::ParseRailGroups()
{
    Ogre::StringVector args = Ogre::StringUtil::split(m_current_line, ",");
    if (args.size() < 3u)
    {
        this->AddMessage(Message::TYPE_ERROR, "Not enough parameters");
        return;
    }

    RailGroup railgroup;
    railgroup.id = this->ParseArgInt(args[0].c_str());

    for (auto itor = args.begin() + 1; itor != args.end(); itor++)
    {
        railgroup.node_list.push_back( this->_ParseNodeRef(*itor));
    }

    m_current_module->railgroups.push_back(railgroup);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->railgroups.size() - 1));
}

void Parser::ParseProps()
{
    if (! this->CheckNumArguments(10)) { return; }

    Prop prop;
    prop.reference_node = this->GetArgNodeRef(0);
    prop.x_axis_node    = this->GetArgNodeRef(1);
    prop.y_axis_node    = this->GetArgNodeRef(2);
    prop.offset.x       = this->GetArgFloat  (3);
    prop.offset.y       = this->GetArgFloat  (4);
    prop.offset.z       = this->GetArgFloat  (5);
    prop.rotation.x     = this->GetArgFloat  (6);
    prop.rotation.y     = this->GetArgFloat  (7);
    prop.rotation.z     = this->GetArgFloat  (8);
    prop.mesh_name      = this->GetArgStr    (9);

    bool is_dash = false;
         if (prop.mesh_name.find("leftmirror"  ) != std::string::npos) { prop.special = Prop::SPECIAL_MIRROR_LEFT; }
    else if (prop.mesh_name.find("rightmirror" ) != std::string::npos) { prop.special = Prop::SPECIAL_MIRROR_RIGHT; }
    else if (prop.mesh_name.find("dashboard-rh") != std::string::npos) { prop.special = Prop::SPECIAL_DASHBOARD_RIGHT; is_dash = true; }
    else if (prop.mesh_name.find("dashboard"   ) != std::string::npos) { prop.special = Prop::SPECIAL_DASHBOARD_LEFT;  is_dash = true; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "spinprop", false) ) { prop.special = Prop::SPECIAL_AERO_PROP_SPIN; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "pale", false)     ) { prop.special = Prop::SPECIAL_AERO_PROP_BLADE; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "seat", false)     ) { prop.special = Prop::SPECIAL_DRIVER_SEAT; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "seat2", false)    ) { prop.special = Prop::SPECIAL_DRIVER_SEAT_2; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "beacon", false)   ) { prop.special = Prop::SPECIAL_BEACON; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "redbeacon", false)) { prop.special = Prop::SPECIAL_REDBEACON; }
    else if (Ogre::StringUtil::startsWith(prop.mesh_name, "lightb", false)   ) { prop.special = Prop::SPECIAL_LIGHTBAR; } // Previously: 'strncmp("lightbar", meshname, 6)'

    if ((prop.special == Prop::SPECIAL_BEACON) && (m_num_args >= 14))
    {
        prop.special_prop_beacon.flare_material_name = this->GetArgStr(10);
        Ogre::StringUtil::trim(prop.special_prop_beacon.flare_material_name);

        prop.special_prop_beacon.color = Ogre::ColourValue(
            this->GetArgFloat(11), this->GetArgFloat(12), this->GetArgFloat(13));
    }
    else if (is_dash)
    {
        if (m_num_args > 10) prop.special_prop_dashboard.mesh_name = this->GetArgStr(10);
        if (m_num_args > 13)
        {
            prop.special_prop_dashboard.offset = Ogre::Vector3(this->GetArgFloat(11), this->GetArgFloat(12), this->GetArgFloat(13));
            prop.special_prop_dashboard._offset_is_set = true;
        }
        if (m_num_args > 14) prop.special_prop_dashboard.rotation_angle = this->GetArgFloat(14);
    }

    m_current_module->props.push_back(prop);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->props.size() - 1));
}

void Parser::ParsePistonprops()
{
    if (!this->CheckNumArguments(10)) { return; }

    Pistonprop pistonprop;
    pistonprop.reference_node     = this->GetArgNodeRef     (0);
    pistonprop.axis_node          = this->GetArgNodeRef     (1);
    pistonprop.blade_tip_nodes[0] = this->GetArgNodeRef     (2);
    pistonprop.blade_tip_nodes[1] = this->GetArgNodeRef     (3);
    pistonprop.blade_tip_nodes[2] = this->GetArgNullableNode(4);
    pistonprop.blade_tip_nodes[3] = this->GetArgNullableNode(5);
    pistonprop.couple_node        = this->GetArgNullableNode(6);
    pistonprop.turbine_power_kW   = this->GetArgFloat       (7);
    pistonprop.pitch              = this->GetArgFloat       (8);
    pistonprop.airfoil            = this->GetArgStr         (9);

    m_current_module->pistonprops.push_back(pistonprop);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->pistonprops.size() - 1));
}

void Parser::ParseParticles()
{
    if (!this->CheckNumArguments(3)) { return; }

    Particle particle;
    particle.emitter_node         = this->GetArgNodeRef(0);
    particle.reference_node       = this->GetArgNodeRef(1);
    particle.particle_system_name = this->GetArgStr    (2);

    m_current_module->particles.push_back(particle);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->particles.size() - 1));
}

// Static
void Parser::_TrimTrailingComments(std::string const & line_in, std::string & line_out)
{
    // Trim trailing comment
    // We need to handle a case of lines as [keyword 1, 2, 3 ;;///// Comment!]
    int comment_start = static_cast<int>(line_in.find_first_of(";"));
    if (comment_start != Ogre::String::npos)
    {
        line_out = line_in.substr(0, comment_start);
        return;
    }
    // The [//Comment] is harder - the '/' character may also be present in DESCRIPTION arguments!
    comment_start = static_cast<int>(line_in.find_last_of("/"));
    if (comment_start != Ogre::String::npos)
    {
        while (comment_start >= 0)
        {
            char c = line_in[comment_start - 1];
            if (c != '/' && c != ' ' && c != '\t')
            {
                break; // Start of comment found
            }
            --comment_start;
        }
        line_out = line_in.substr(0, comment_start);
        return;
    }
    // No comment found
    line_out = line_in;
}

void Parser::ParseNodesUnified()
{
    if (! this->CheckNumArguments(4)) { return; }

    Node node;
    node._num_args = m_num_args;

    if (m_current_section == SECTION_NODES_2)
    {
        node.name = this->GetArgStr(0);
    }
    else
    {
        node.number = static_cast<NodeIdx_t>(this->GetArgUint(0));
    }

    node.position.x = this->GetArgFloat(1);
    node.position.y = this->GetArgFloat(2);
    node.position.z = this->GetArgFloat(3);
    if (m_num_args > 4)
    {
        node.options = this->GetArgStr(4);
    }
    if (m_num_args > 5)
    {
        // Only used on spawn if 'l' flag is present
        node.load_weight_override = this->GetArgFloat(5);
    }

    m_current_module->nodes.push_back(node);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->nodes.size() - 1));
}

void Parser::ParseNodeCollision()
{
    if (! this->CheckNumArguments(2)) { return; }
    
    NodeCollision node_collision;
    node_collision.node   = this->GetArgNodeRef(0);
    node_collision.radius = this->GetArgFloat  (1);

    m_current_module->node_collisions.push_back(node_collision);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->node_collisions.size() - 1));
}

void Parser::ParseMinimass()
{
    // Does not affect 'set_default_minimas' presets, only provides fallback for nodes where 'set_default_minimass' was not used (for backwards compatibility).

    Minimass def;
    def._num_args = m_num_args;
    def.min_mass = this->GetArgFloat(0);
    if (m_num_args > 1)
    {
        const std::string options_str = this->GetArgStr(1);
        for (char c: options_str)
        {
            switch (c)
            {
            case '\0': // Terminator NULL character
            case 'n': // dummy flag
                break;

            case 'l':
                def.option_skip_loaded_nodes = true;
                break;

            default:
                this->AddMessage(Message::TYPE_WARNING, std::string("Unknown option: ") + c);
                break;
            }
        }
    }

    m_current_module->minimass.push_back(def);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->minimass.size() - 1));

    this->ChangeSection(SECTION_NONE);
}

void Parser::ParseFlexBodyWheel()
{
    if (! this->CheckNumArguments(16)) { return; }

    FlexBodyWheel flexbody_wheel;

    flexbody_wheel.tyre_radius        = this->GetArgFloat        ( 0);
    flexbody_wheel.rim_radius         = this->GetArgFloat        ( 1);
    flexbody_wheel.width              = this->GetArgFloat        ( 2);
    flexbody_wheel.num_rays           = this->GetArgInt          ( 3);
    flexbody_wheel.nodes[0]           = this->GetArgNodeRef      ( 4);
    flexbody_wheel.nodes[1]           = this->GetArgNodeRef      ( 5);
    flexbody_wheel.rigidity_node      = this->GetArgRigidityNode ( 6);
    flexbody_wheel.braking            = this->GetArgBraking      ( 7);
    flexbody_wheel.propulsion         = this->GetArgPropulsion   ( 8);
    flexbody_wheel.reference_arm_node = this->GetArgNodeRef      ( 9);
    flexbody_wheel.mass               = this->GetArgFloat        (10);
    flexbody_wheel.tyre_springiness   = this->GetArgFloat        (11);
    flexbody_wheel.tyre_damping       = this->GetArgFloat        (12);
    flexbody_wheel.rim_springiness    = this->GetArgFloat        (13);
    flexbody_wheel.rim_damping        = this->GetArgFloat        (14);
    flexbody_wheel.side               = this->GetArgWheelSide    (15);

    if (m_num_args > 16) { flexbody_wheel.rim_mesh_name  = this->GetArgStr(16); }
    if (m_num_args > 17) { flexbody_wheel.tyre_mesh_name = this->GetArgStr(17); }

    m_current_module->flex_body_wheels.push_back(flexbody_wheel);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->flex_body_wheels.size() - 1));
}

void Parser::ParseMaterialFlareBindings()
{
    if (! this->CheckNumArguments(2)) { return; }

    MaterialFlareBinding binding;
    binding.flare_number  = this->GetArgInt(0);
    binding.material_name = this->GetArgStr(1);
    
    m_current_module->material_flare_bindings.push_back(binding);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->material_flare_bindings.size() - 1));
}

void Parser::ParseManagedMaterials()
{
    if (! this->CheckNumArguments(2)) { return; }

    ManagedMaterial managed_mat;
    
    managed_mat.managed_mat_options = (int)m_current_module->managed_mat_options.size() - 1;
    managed_mat.name    = this->GetArgStr(0);

    const std::string type_str = this->GetArgStr(1);
    if (type_str == "mesh_standard" || type_str == "mesh_transparent")
    {
        if (! this->CheckNumArguments(3)) { return; }

        managed_mat.type = (type_str == "mesh_standard")
            ? ManagedMaterial::TYPE_MESH_STANDARD
            : ManagedMaterial::TYPE_MESH_TRANSPARENT;
        
        managed_mat.diffuse_map = this->GetArgStr(2);
        
        if (m_num_args > 3) { managed_mat.specular_map = this->GetArgManagedTex(3); }
    }
    else if (type_str == "flexmesh_standard" || type_str == "flexmesh_transparent")
    {
        if (! this->CheckNumArguments(3)) { return; }

        managed_mat.type = (type_str == "flexmesh_standard")
            ? ManagedMaterial::TYPE_FLEXMESH_STANDARD
            : ManagedMaterial::TYPE_FLEXMESH_TRANSPARENT;
            
        managed_mat.diffuse_map = this->GetArgStr(2);
        
        if (m_num_args > 3) { managed_mat.damaged_diffuse_map = this->GetArgManagedTex(3); }
        if (m_num_args > 4) { managed_mat.specular_map        = this->GetArgManagedTex(4); }
    }
    else
    {
        this->AddMessage(Message::TYPE_WARNING, type_str + " is an unkown effect");
        return;
    }

    Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

    if (!rgm.resourceExists(m_resource_group, managed_mat.diffuse_map))
    {
        this->AddMessage(Message::TYPE_WARNING, "Missing texture file: " + managed_mat.diffuse_map);
        return;
    }
    if (managed_mat.HasDamagedDiffuseMap() && !rgm.resourceExists(m_resource_group, managed_mat.damaged_diffuse_map))
    {
        this->AddMessage(Message::TYPE_WARNING, "Missing texture file: " + managed_mat.damaged_diffuse_map);
        managed_mat.damaged_diffuse_map = "-";
    }
    if (managed_mat.HasSpecularMap() && !rgm.resourceExists(m_resource_group, managed_mat.specular_map))
    {
        this->AddMessage(Message::TYPE_WARNING, "Missing texture file: " + managed_mat.specular_map);
        managed_mat.specular_map = "-";
    }

    m_current_module->managed_materials.push_back(managed_mat);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->managed_materials.size() - 1));
}

void Parser::ParseLockgroups()
{
    if (! this->CheckNumArguments(2)) { return; } // Lockgroup num. + at least 1 node...

    Lockgroup lockgroup;
    lockgroup.number = this->GetArgInt(0);
    
    for (int i = 1; i < m_num_args; ++i)
    {
        lockgroup.nodes.push_back(this->GetArgNodeRef(i));
    }
    
    m_current_module->lockgroups.push_back(lockgroup);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->lockgroups.size() - 1));
}

void Parser::ParseHydros()
{
    if (! this->CheckNumArguments(3)) { return; }

    Hydro hydro;
    
    hydro.nodes[0]           = this->GetArgNodeRef(0);
    hydro.nodes[1]           = this->GetArgNodeRef(1);
    hydro.lenghtening_factor = this->GetArgFloat  (2);
    if (m_num_args > 3) { hydro.options = this->GetArgStr(3); }
    
    this->ParseOptionalInertia(hydro.inertia, 4);

    m_current_module->hydros.push_back(hydro);
    m_current_module->sequence.push_back(SeqSection(m_current_section, (int)m_current_module->hydros.size() - 1));
}

void Parser::ParseOptionalInertia(OptionalInertia & inertia, int index)
{
    if (m_num_args > index) { inertia.start_delay_factor = this->GetArgFloat(index++); }
    if (m_num_args > index) { inertia.stop_delay_factor  = this->GetArgFloat(index++); }
    if (m_num_args > index) { inertia.start_function     = this->GetArgStr  (index++); }
    if (m_num_args > index) { inertia.stop_function      = this->GetArgStr  (index++); }
}

void Parser::ParseBeams()
{
    if (! this->CheckNumArguments(2)) { return; }

    Beam beam;

    beam.nodes[0] = this->GetArgNodeRef(0);
    beam.nodes[1] = this->GetArgNodeRef(1);

    // Flags 
    if (m_num_args > 2)
    {
        std::string options_str = this->GetArgStr(2);
        for (char c: options_str)
        {
            switch (c)
            {
            case 'i': // Invisible
                beam.invisible = true;
                break;
            case 'v': // Visible
                beam.invisible = false;
                break;
            case 'r': // Rope
                beam.bounded = SpecialBeam::ROPE;
                break;
            case 's': // Supportbeam
                beam.bounded = SpecialBeam::SUPPORTBEAM;
                break;
            case 'n': // Dummy flag
                break;
            default:
                this->AddMessage(Message::TYPE_WARNING, fmt::format("Ignoring invalid node option '{}'", c));
            }
        }
    }
    
    if ((m_num_args > 3) && (beam.bounded == SpecialBeam::SUPPORTBEAM))
    {
        float support_break_limit = 0.0f;
        float support_break_factor = this->GetArgInt(3);
        if (support_break_factor > 0.0f)
        {
            support_break_limit = support_break_factor;
        }
        beam.extension_break_limit = support_break_limit;
        beam._has_extension_break_limit = true;
    }

    m_current_module->beams.push_back(beam);
    m_current_module->sequence.push_back(SeqSection(m_current_section, (int)m_current_module->beams.size() - 1));
}

void Parser::ParseAnimator()
{
    auto args = Ogre::StringUtil::split(m_current_line, ",");
    if (args.size() < 4) { return; }

    Animator animator;

    animator.nodes[0]           = this->_ParseNodeRef(args[0]);
    animator.nodes[1]           = this->_ParseNodeRef(args[1]);
    animator.lenghtening_factor = this->ParseArgFloat(args[2]);

    // Parse options; Just use the split/trim/compare method
    Ogre::StringVector attrs = Ogre::StringUtil::split(args[3], "|");

    auto itor = attrs.begin();
    auto endi = attrs.end();
    for (; itor != endi; ++itor)
    {
        Ogre::String token = *itor;
        Ogre::StringUtil::trim(token);
        std::smatch results;
        bool is_shortlimit = false;

        // Numbered keywords 
        if (std::regex_search(token, results, Regexes::PARSE_ANIMATORS_NUMBERED_KEYWORD))
        {
                 if (results[1] == "throttle")   animator.aero_animator.flags |= AeroAnimator::OPTION_THROTTLE;
            else if (results[1] == "rpm")        animator.aero_animator.flags |= AeroAnimator::OPTION_RPM;
            else if (results[1] == "aerotorq")   animator.aero_animator.flags |= AeroAnimator::OPTION_TORQUE;
            else if (results[1] == "aeropit")    animator.aero_animator.flags |= AeroAnimator::OPTION_PITCH;
            else if (results[1] == "aerostatus") animator.aero_animator.flags |= AeroAnimator::OPTION_STATUS;

            animator.aero_animator.engine_idx = this->ParseArgUint(results[2].str().c_str()) - 1;
        }
        else if ((is_shortlimit = (token.compare(0, 10, "shortlimit") == 0)) || (token.compare(0, 9, "longlimit") == 0))
        {
            Ogre::StringVector fields = Ogre::StringUtil::split(token, ":");
            if (fields.size() > 1)
            {
                if (is_shortlimit)
                {
                    animator.short_limit = std::strtod(fields[1].c_str(), nullptr);
                    animator.flags |= Animator::OPTION_SHORT_LIMIT;
                }
                else
                {
                    animator.long_limit = std::strtod(fields[1].c_str(), nullptr);
                    animator.flags |= Animator::OPTION_LONG_LIMIT;
                }
            }
        }
        else
        {
            // Standalone keywords 
                 if (token == "vis")           animator.flags |= Animator::OPTION_VISIBLE;
            else if (token == "inv")           animator.flags |= Animator::OPTION_INVISIBLE;
            else if (token == "airspeed")      animator.flags |= Animator::OPTION_AIRSPEED;
            else if (token == "vvi")           animator.flags |= Animator::OPTION_VERTICAL_VELOCITY;
            else if (token == "altimeter100k") animator.flags |= Animator::OPTION_ALTIMETER_100K;
            else if (token == "altimeter10k")  animator.flags |= Animator::OPTION_ALTIMETER_10K;
            else if (token == "altimeter1k")   animator.flags |= Animator::OPTION_ALTIMETER_1K;
            else if (token == "aoa")           animator.flags |= Animator::OPTION_ANGLE_OF_ATTACK;
            else if (token == "flap")          animator.flags |= Animator::OPTION_FLAP;
            else if (token == "airbrake")      animator.flags |= Animator::OPTION_AIR_BRAKE;
            else if (token == "roll")          animator.flags |= Animator::OPTION_ROLL;
            else if (token == "pitch")         animator.flags |= Animator::OPTION_PITCH;
            else if (token == "brakes")        animator.flags |= Animator::OPTION_BRAKES;
            else if (token == "accel")         animator.flags |= Animator::OPTION_ACCEL;
            else if (token == "clutch")        animator.flags |= Animator::OPTION_CLUTCH;
            else if (token == "speedo")        animator.flags |= Animator::OPTION_SPEEDO;
            else if (token == "tacho")         animator.flags |= Animator::OPTION_TACHO;
            else if (token == "turbo")         animator.flags |= Animator::OPTION_TURBO;
            else if (token == "parking")       animator.flags |= Animator::OPTION_PARKING;
            else if (token == "shifterman1")   animator.flags |= Animator::OPTION_SHIFT_LEFT_RIGHT;
            else if (token == "shifterman2")   animator.flags |= Animator::OPTION_SHIFT_BACK_FORTH;
            else if (token == "sequential")    animator.flags |= Animator::OPTION_SEQUENTIAL_SHIFT;
            else if (token == "shifterlin")    animator.flags |= Animator::OPTION_GEAR_SELECT;
            else if (token == "torque")        animator.flags |= Animator::OPTION_TORQUE;
            else if (token == "difflock")      animator.flags |= Animator::OPTION_DIFFLOCK;
            else if (token == "rudderboat")    animator.flags |= Animator::OPTION_BOAT_RUDDER;
            else if (token == "throttleboat")  animator.flags |= Animator::OPTION_BOAT_THROTTLE;
        }
    }

    m_current_module->animators.push_back(animator);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->animators.size() - 1));
}

void Parser::ParseAuthor()
{
    if (! this->CheckNumArguments(2)) { return; }

    Author author;
    if (m_num_args > 1) { author.type             = this->GetArgStr(1); }
    if (m_num_args > 2) { author.forum_account_id = this->GetArgInt(2); author._has_forum_account = true; }
    if (m_num_args > 3) { author.name             = this->GetArgStr(3); }
    if (m_num_args > 4) { author.email            = this->GetArgStr(4); }
    m_current_module->authors.push_back(author);
    m_current_module->sequence.push_back(SeqElement(m_last_keyword, (int)m_current_module->authors.size() - 1));

    this->ChangeSection(SECTION_NONE);
}

// -------------------------------------------------------------------------- 
//  Utilities
// -------------------------------------------------------------------------- 

void Parser::AddMessage(std::string const & line, Message::Type type, std::string const & message)
{
    RoR::Str<4000> txt;

    if (!m_definition->name.empty())
    {
        txt << m_definition->name;
    }
    else
    {
        txt << m_filename;
    }

    txt << " (line " << (size_t)m_current_line_number;
    if (m_current_section != Section::SECTION_INVALID)
    {
        txt << " '" << Truck::Document::SectionToString(m_current_section);
        if (m_current_subsection != Subsection::SUBSECTION_NONE)
        {
            txt << "/" << Truck::Document::SubsectionToString(m_current_subsection);
        }
        txt << "'";
    }

    txt << "): " << message;

    RoR::Console::MessageType cm_type;
    switch (type)
    {
    case Message::TYPE_FATAL_ERROR:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_ERROR;
        break;

    case Message::TYPE_ERROR:
    case Message::TYPE_WARNING:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_WARNING;
        break;

    default:
        cm_type = RoR::Console::MessageType::CONSOLE_SYSTEM_NOTICE;
        break;
    }

    RoR::App::GetConsole()->putMessage(RoR::Console::CONSOLE_MSGTYPE_ACTOR, cm_type, txt.ToCStr());
}

Keyword Parser::IdentifyKeywordInCurrentLine()
{
    // Quick check - keyword always starts with ASCII letter
    char c = tolower(m_current_line[0]); // Note: line comes in trimmed
    if (c > 'z' || c < 'a')
    {
        return KEYWORD_INVALID;
    }

    // Search (always ignore case)
    std::smatch results;
    const std::string line(m_current_line);
    std::regex_search(line, results, IDENTIFY_KEYWORD_IGNORE_CASE); // Always returns true.
    return FindKeywordMatch(results);
}

Keyword Parser::FindKeywordMatch(std::smatch& search_results)
{
    // The 'results' array contains a complete match at positon [0] and sub-matches starting with [1], 
    //    so we get exact positions in Regexes::IDENTIFY_KEYWORD, which again match Keyword enum members

    for (unsigned int i = 1; i < search_results.size(); i++)
    {
        std::ssub_match sub  = search_results[i];
        if (sub.matched)
        {
            // Build enum value directly from result offset
            return Keyword(i);
        }
    }
    return KEYWORD_INVALID;
}

void Parser::Prepare()
{
    m_last_keyword = KEYWORD_INVALID;
    m_current_section = SECTION_TRUCK_NAME;
    m_current_subsection = SUBSECTION_NONE;
    m_current_line_number = 1;
    m_definition = std::make_shared<Document>();
    m_in_block_comment = false;
    m_in_description_section = false;

    ModulePtr m(new Module());
    m_definition->modules.emplace_back(m);
    m_current_module = m;
}

void Parser::ChangeSection(Truck::Section new_section)
{
    // Exit previous section
    if (m_current_section == SECTION_FLEXBODIES || m_current_section == SECTION_SUBMESH)
    {
        m_current_subsection = SUBSECTION_NONE;
    }

    // Enter next section
    m_current_section = new_section;
    if (new_section == SECTION_SUBMESH)
    {
        m_current_module->submeshes.push_back(Submesh());
    }
    else if (new_section == SECTION_CAMERA_RAIL)
    {
        m_current_module->camera_rails.push_back(CameraRail());
    }
    else if (new_section == SECTION_FLEXBODIES)
    {
        m_current_subsection = SUBSECTION__FLEXBODIES__PROPLIKE_LINE;
    }
}

void Parser::BeginModule()
{
    if (m_current_module->defined_explicitly)
    {
        this->AddMessage(Message::TYPE_ERROR, "Nesting modules is not allowed");
        return;
    }

    // Finalize section
    this->ChangeSection(SECTION_NONE);

    // Create explicitly defined module
    ModulePtr m(new Module());
    m->defined_explicitly = true;
    m_definition->modules.emplace_back(m);
    m_current_module = m;

    // arg 0: 'section'
    // arg 1: version (unused)
    for (int i = 2; i < m_num_args; ++i)
    {
        m_current_module->sectionconfigs.push_back(this->GetArgStr(i));
    }
}

void Parser::EndModule()
{
    if (!m_current_module->defined_explicitly)
    {
        this->AddMessage(Message::TYPE_ERROR, "Misplaced 'end_section', no matching 'section'");
        return;
    }

    // Finalize section
    this->ChangeSection(SECTION_NONE);

    // Continue with an implicit module
    ModulePtr m(new Module());
    m_definition->modules.emplace_back(m);
    m_current_module = m;
}

std::string Parser::GetArgStr(int index)
{
    return std::string(m_args[index].start, m_args[index].length);
}

char Parser::GetArgChar(int index)
{
    return *(m_args[index].start);
}

MeshWheel::Side Parser::GetArgWheelSide(int index)
{
    char side_char = this->GetArgChar(index);
    if (side_char != 'r')
    {
        if (side_char != 'l')
        {
            char msg[200] = "";
            snprintf(msg, 200, "Bad arg~%d 'side' (value: %c), parsing as 'l' for backwards compatibility.", index + 1, side_char);
            this->AddMessage(Message::TYPE_WARNING, msg);
        }
        return MeshWheel::SIDE_LEFT;
    }
    return MeshWheel::SIDE_RIGHT;
}

long Parser::GetArgLong(int index)
{
    errno = 0;
    char* out_end = nullptr;
    const int MSG_LEN = 200;
    char msg[MSG_LEN];
    long res = std::strtol(m_args[index].start, &out_end, 10);
    if (errno != 0)
    {
        snprintf(msg, MSG_LEN, "Cannot parse argument [%d] as integer, errno: %d", index + 1, errno);
        this->AddMessage(Message::TYPE_ERROR, msg);
        return 0; // Compatibility
    }
    if (out_end == m_args[index].start)
    {
        snprintf(msg, MSG_LEN, "Argument [%d] is not valid integer", index + 1);
        this->AddMessage(Message::TYPE_ERROR, msg);
        return 0; // Compatibility
    }
    else if (out_end != (m_args[index].start + m_args[index].length))
    {
        snprintf(msg, MSG_LEN, "Integer argument [%d] has invalid trailing characters", index + 1);
        this->AddMessage(Message::TYPE_WARNING, msg);
    }
    return res;
}

int Parser::GetArgInt(int index)
{
    return static_cast<int>(this->GetArgLong(index));
}

Node::Ref Parser::GetArgRigidityNode(int index)
{
    std::string rigidity_node = this->GetArgStr(index);
    if (rigidity_node != "9999") // Special null value
    {
        return this->GetArgNodeRef(index);
    }
    return Node::Ref(); // Defaults to invalid ref
}

Wheels::Propulsion Parser::GetArgPropulsion(int index)
{
    int propulsion = this->GetArgInt(index);
    if (propulsion < 0 || propulsion > 2)
    {
        char msg[100] = "";
        snprintf(msg, 100, "Bad value of param ~%d (propulsion), using 0 (no propulsion)", index + 1);
        this->AddMessage(Message::TYPE_ERROR, msg);
        return Wheels::PROPULSION_NONE;
    }
    return Wheels::Propulsion(propulsion);
}

Wheels::Braking Parser::GetArgBraking(int index)
{
    int braking = this->GetArgInt(index);
    if (braking < 0 || braking > 4)
    {
        char msg[100] = "";
        snprintf(msg, 100, "Bad value of param ~%d (braking), using 0 (no braking)", index + 1);
        return Wheels::BRAKING_NO;
    }
    return Wheels::Braking(braking);
}

Node::Ref Parser::GetArgNodeRef(int index)
{
    return this->_ParseNodeRef(this->GetArgStr(index));
}

Node::Ref Parser::GetArgNullableNode(int index)
{
    if (! (Ogre::StringConverter::parseReal(this->GetArgStr(index)) == -1.f))
    {
        return this->GetArgNodeRef(index);
    }
    return Node::Ref(); // Defaults to empty ref.
}

unsigned Parser::GetArgUint(int index)
{
    return static_cast<unsigned>(this->GetArgLong(index));
}

Flare2::Type Parser::GetArgFlareType(int index)
{
    char in = this->GetArgChar(index);
    if (in != 'f' && in != 'b' && in != 'l' && in != 'r' && in != 'R' && in != 'u')
    {
        char msg[100];
        snprintf(msg, 100, "Invalid flare type '%c', falling back to type 'f' (front light)...", in);
        AddMessage(Message::TYPE_WARNING, msg);

        in = 'f';
    }
    return Flare2::Type(in);
}

float Parser::GetArgFloat(int index)
{
    errno = 0;
    char* out_end = nullptr;
    float res = std::strtod(m_args[index].start, &out_end);
    const int MSG_LEN = LINE_BUFFER_LENGTH +100;
    char msg_buf[MSG_LEN];
    if (errno != 0)
    {
        snprintf(msg_buf, MSG_LEN, "Cannot parse argument [%d] as float, errno: %d", index + 1, errno);
        this->AddMessage(Message::TYPE_ERROR, msg_buf);
        return 0.f; // Compatibility
    }
    if (out_end == m_args[index].start)
    {
        char arg[LINE_BUFFER_LENGTH] = "";
        strncpy(arg, m_args[index].start, m_args[index].length);
        snprintf(msg_buf, MSG_LEN, "Argument [%d] (\"%s\") is not valid float", index + 1, arg);
        this->AddMessage(Message::TYPE_ERROR, msg_buf);
        return 0.f; // Compatibility
    }
    else if (out_end != (m_args[index].start + m_args[index].length))
    {
        char arg[LINE_BUFFER_LENGTH] = "";
        ptrdiff_t offset = (out_end - m_args[index].start);
        strncpy(arg, out_end, m_args[index].length - offset);
        snprintf(msg_buf, MSG_LEN, "Argument [%d] (type: float) has invalid trailing characters (\"%s\")", index + 1, arg);
        this->AddMessage(Message::TYPE_WARNING, msg_buf);
    }
    return static_cast<float>(res);
}

float Parser::ParseArgFloat(const char* str)
{
    errno = 0;
    float res = std::strtod(str, nullptr);
    if (errno != 0)
    {
        char msg[100];
        snprintf(msg, 100, "Cannot parse argument '%s' as float, errno: %d", str, errno);
        this->AddMessage(Message::TYPE_ERROR, msg);
        return 0.f; // Compatibility
    }
    return static_cast<float>(res);
}

float Parser::ParseArgFloat(std::string const & str)
{
    return this->ParseArgFloat(str.c_str());
}

unsigned Parser::ParseArgUint(const char* str)
{
    errno = 0;
    long res = std::strtol(str, nullptr, 10);
    if (errno != 0)
    {
        char msg[200];
        snprintf(msg, 200, "Cannot parse argument '%s' as int, errno: %d", str, errno);
        this->AddMessage(Message::TYPE_ERROR, msg);
        return 0.f; // Compatibility
    }
    return static_cast<unsigned>(res);
}

unsigned Parser::ParseArgUint(std::string const & str)
{
    return this->ParseArgUint(str.c_str());
}

int Parser::ParseArgInt(const char* str)
{
    return static_cast<int>(this->ParseArgUint(str));
}

bool Parser::GetArgBool(int index)
{
    return Ogre::StringConverter::parseBool(this->GetArgStr(index));
}

Wing::Control Parser::GetArgWingSurface(int index)
{
    std::string str = this->GetArgStr(index);
    size_t bad_pos = str.find_first_not_of(Wing::CONTROL_LEGAL_FLAGS);
    const int MSG_LEN = 300;
    char msg_buf[MSG_LEN] = "";
    if (bad_pos == 0)
    {
        snprintf(msg_buf, MSG_LEN, "Invalid argument ~%d 'control surface' (value: %s), allowed are: <%s>, ignoring...",
            index + 1, str.c_str(), Wing::CONTROL_LEGAL_FLAGS.c_str());
        this->AddMessage(Message::TYPE_ERROR, msg_buf);
        return Wing::CONTROL_n_NONE;
    }
    if (str.size() > 1)
    {
        snprintf(msg_buf, MSG_LEN, "Argument ~%d 'control surface' (value: %s), should be only 1 letter.", index, str.c_str());
        this->AddMessage(Message::TYPE_WARNING, msg_buf);
    }
    return Wing::Control(str.at(0));
}

std::string Parser::GetArgManagedTex(int index)
{
    std::string tex_name = this->GetArgStr(index);
    return (tex_name.at(0) != '-') ? tex_name : "";
}

int Parser::TokenizeCurrentLine()
{
    int cur_arg = 0;
    const char* cur_char = m_current_line;
    int arg_len = 0;
    while ((*cur_char != '\0') && (cur_arg < Parser::LINE_MAX_ARGS))
    {
        const bool is_arg = !IsSeparator(*cur_char);
        if ((arg_len == 0) && is_arg)
        {
            m_args[cur_arg].start = cur_char;
            arg_len = 1;
        }
        else if ((arg_len > 0) && !is_arg)
        {
            m_args[cur_arg].length = arg_len;
            arg_len = 0;
            ++cur_arg;
        }
        else if (is_arg)
        {
            ++arg_len;
        }
        ++cur_char;
    }
    if (arg_len > 0)
    {
        m_args[cur_arg].length = arg_len;
        ++cur_arg;
    }

    m_num_args = cur_arg;
    return cur_arg;
}

void Parser::ProcessOgreStream(Ogre::DataStream* stream, Ogre::String resource_group)
{
    m_resource_group = resource_group;
    m_filename = stream->getName();

    char raw_line_buf[LINE_BUFFER_LENGTH];
    while (!stream->eof())
    {
        try
        {
            stream->readLine(raw_line_buf, LINE_BUFFER_LENGTH);
        }
        catch (Ogre::Exception &ex)
        {
            std::string msg = "Error reading truckfile! Message:\n";
            msg += ex.getFullDescription();
            this->AddMessage(Message::TYPE_FATAL_ERROR, msg.c_str());
            break;
        }

        this->ProcessRawLine(raw_line_buf);
    }

    this->ChangeSection(SECTION_NONE);
}

void Parser::ProcessRawLine(const char* raw_line_buf)
{
    const char* raw_start = raw_line_buf;
    const char* raw_end = raw_line_buf + strnlen(raw_line_buf, LINE_BUFFER_LENGTH);

    // Trim leading whitespace
    while (IsWhitespace(*raw_start) && (raw_start != raw_end))
    {
        ++raw_start;
    }

    // Skip empty lines
    if (raw_start == raw_end)
    {
        ++m_current_line_number;
        return;
    }

    // Sanitize UTF-8
    memset(m_current_line, 0, LINE_BUFFER_LENGTH);
    char* out_start = m_current_line;
    utf8::replace_invalid(raw_start, raw_end, out_start, '?');

    // Process
    this->ProcessCurrentLine();
    ++m_current_line_number;
}

void Parser::ProcessCommentLine()
{
    if (m_current_line[0] != ';')
    {
        return;
    }

    Str<100> name;
    if ((strlen(m_current_line) >= 5) &&
        (m_current_line[1] == 'g')    &&
        (m_current_line[2] == 'r')    &&
        (m_current_line[3] == 'p')    &&
        (m_current_line[4] == ':'))
    {
        name = &m_current_line[5];
    }
    else if (App::diag_import_grp_loose->GetBool())
    {
        name = &m_current_line[1];
    }
    else
    {
        return;
    }

    m_current_module->editor_groups.push_back(EditorGroup(name.ToCStr(), m_current_section));
}

int Parser::GetCurrentEditorGroup()
{
    if (!m_current_module->editor_groups.empty() &&
        m_current_module->editor_groups.rbegin()->section == m_current_section)
    {
        return (int)m_current_module->editor_groups.size() - 1;
    }
    else
    {
        return -1; // no group
    }
}

