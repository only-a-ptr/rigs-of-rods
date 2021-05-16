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

#include "ScriptEngine.h"

#include "GfxActor.h"
#include "GfxScene.h"

using namespace RoR;
using namespace AngelScript;

void RoR::RegisterFrameStepInterface(asIScriptEngine* engine)
{
    const char* obj = "ActorSimBuffer";
    engine->RegisterObjectType(obj, 0, asOBJ_REF | asOBJ_NOCOUNT);

    engine->RegisterObjectProperty(obj, "vector3        pos                    ", asOFFSET(GfxActor::SimBuffer, simbuf_pos                    )); //Ogre::Vector3               
    engine->RegisterObjectProperty(obj, "vector3        node0_velo             ", asOFFSET(GfxActor::SimBuffer, simbuf_node0_velo             )); //Ogre::Vector3               
    engine->RegisterObjectProperty(obj, "bool           live_local             ", asOFFSET(GfxActor::SimBuffer, simbuf_live_local             )); //bool                        
    engine->RegisterObjectProperty(obj, "bool           physics_paused         ", asOFFSET(GfxActor::SimBuffer, simbuf_physics_paused         )); //bool                        
    engine->RegisterObjectProperty(obj, "float          rotation               ", asOFFSET(GfxActor::SimBuffer, simbuf_rotation               )); //float                       
    engine->RegisterObjectProperty(obj, "float          tyre_pressure          ", asOFFSET(GfxActor::SimBuffer, simbuf_tyre_pressure          )); //float                       
   // engine->RegisterObjectProperty(obj, "AxisAlignedBox aabb                   ", asOFFSET(GfxActor::SimBuffer, simbuf_aabb                   )); //Ogre::AxisAlignedBox        
    engine->RegisterObjectProperty(obj, "string         net_username           ", asOFFSET(GfxActor::SimBuffer, simbuf_net_username           )); //std::string                 
    engine->RegisterObjectProperty(obj, "bool           is_remote              ", asOFFSET(GfxActor::SimBuffer, simbuf_is_remote              )); //bool                        
    engine->RegisterObjectProperty(obj, "int            gear                   ", asOFFSET(GfxActor::SimBuffer, simbuf_gear                   )); //int                         
    engine->RegisterObjectProperty(obj, "int            autoshift              ", asOFFSET(GfxActor::SimBuffer, simbuf_autoshift              )); //int                         
    engine->RegisterObjectProperty(obj, "float          wheel_speed            ", asOFFSET(GfxActor::SimBuffer, simbuf_wheel_speed            )); //float                       
    engine->RegisterObjectProperty(obj, "float          engine_rpm             ", asOFFSET(GfxActor::SimBuffer, simbuf_engine_rpm             )); //float                       
    engine->RegisterObjectProperty(obj, "float          engine_crankfactor     ", asOFFSET(GfxActor::SimBuffer, simbuf_engine_crankfactor     )); //float                       
    engine->RegisterObjectProperty(obj, "float          engine_turbo_psi       ", asOFFSET(GfxActor::SimBuffer, simbuf_engine_turbo_psi       )); //float                       
    engine->RegisterObjectProperty(obj, "float          engine_accel           ", asOFFSET(GfxActor::SimBuffer, simbuf_engine_accel           )); //float                       
    engine->RegisterObjectProperty(obj, "float          engine_torque          ", asOFFSET(GfxActor::SimBuffer, simbuf_engine_torque          )); //float                       
    engine->RegisterObjectProperty(obj, "float          inputshaft_rpm         ", asOFFSET(GfxActor::SimBuffer, simbuf_inputshaft_rpm         )); //float                       
    engine->RegisterObjectProperty(obj, "float          drive_ratio            ", asOFFSET(GfxActor::SimBuffer, simbuf_drive_ratio            )); //float                       
    engine->RegisterObjectProperty(obj, "bool           beaconlight_active     ", asOFFSET(GfxActor::SimBuffer, simbuf_beaconlight_active     )); //bool                        
    engine->RegisterObjectProperty(obj, "float          hydro_dir_state        ", asOFFSET(GfxActor::SimBuffer, simbuf_hydro_dir_state        )); //float                       
    engine->RegisterObjectProperty(obj, "float          hydro_aileron_state    ", asOFFSET(GfxActor::SimBuffer, simbuf_hydro_aileron_state    )); //float                       
    engine->RegisterObjectProperty(obj, "float          hydro_elevator_state   ", asOFFSET(GfxActor::SimBuffer, simbuf_hydro_elevator_state   )); //float                       
    engine->RegisterObjectProperty(obj, "float          hydro_aero_rudder_state", asOFFSET(GfxActor::SimBuffer, simbuf_hydro_aero_rudder_state)); //float                       
    engine->RegisterObjectProperty(obj, "int            cur_cinecam            ", asOFFSET(GfxActor::SimBuffer, simbuf_cur_cinecam            )); //int                         
    //engine->RegisterObjectProperty(obj, "std::vector<S  screwprops             ", asOFFSET(GfxActor::SimBuffer, simbuf_screwprops             )); //std::vector<ScrewPropSB>    
    //engine->RegisterObjectProperty(obj, "std::vector<C  commandkey             ", asOFFSET(GfxActor::SimBuffer, simbuf_commandkey             )); //std::vector<CommandKeySB>   
    //engine->RegisterObjectProperty(obj, "std::vector<A  aeroengines            ", asOFFSET(GfxActor::SimBuffer, simbuf_aeroengines            )); //std::vector<AeroEngineSB>   
    //engine->RegisterObjectProperty(obj, "std::vector<A  airbrakes              ", asOFFSET(GfxActor::SimBuffer, simbuf_airbrakes              )); //std::vector<AirbrakeSB>     
  //  engine->RegisterObjectProperty(obj, "DiffType       diff_type              ", asOFFSET(GfxActor::SimBuffer, simbuf_diff_type              )); //DiffType                    
    engine->RegisterObjectProperty(obj, "bool           parking_brake          ", asOFFSET(GfxActor::SimBuffer, simbuf_parking_brake          )); //bool                        
    engine->RegisterObjectProperty(obj, "float          brake                  ", asOFFSET(GfxActor::SimBuffer, simbuf_brake                  )); //float                       
    engine->RegisterObjectProperty(obj, "float          clutch                 ", asOFFSET(GfxActor::SimBuffer, simbuf_clutch                 )); //float                       
    engine->RegisterObjectProperty(obj, "int            aero_flap_state        ", asOFFSET(GfxActor::SimBuffer, simbuf_aero_flap_state        )); //int                         
    engine->RegisterObjectProperty(obj, "int            airbrake_state         ", asOFFSET(GfxActor::SimBuffer, simbuf_airbrake_state         )); //int                         
    engine->RegisterObjectProperty(obj, "float          wing4_aoa              ", asOFFSET(GfxActor::SimBuffer, simbuf_wing4_aoa              )); //float                       
    engine->RegisterObjectProperty(obj, "bool           headlight_on           ", asOFFSET(GfxActor::SimBuffer, simbuf_headlight_on           )); //bool                        
    engine->RegisterObjectProperty(obj, "vector3        direction              ", asOFFSET(GfxActor::SimBuffer, simbuf_direction              )); //Ogre::Vector3               
    engine->RegisterObjectProperty(obj, "float          top_speed              ", asOFFSET(GfxActor::SimBuffer, simbuf_top_speed              )); //float                       
      // Autopilot
    engine->RegisterObjectProperty(obj, "int            ap_heading_mode        ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_heading_mode        )); //int                         
    engine->RegisterObjectProperty(obj, "int            ap_heading_value       ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_heading_value       )); //int                         
    engine->RegisterObjectProperty(obj, "int            ap_alt_mode            ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_alt_mode            )); //int                         
    engine->RegisterObjectProperty(obj, "int            ap_alt_value           ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_alt_value           )); //int                         
    engine->RegisterObjectProperty(obj, "bool           ap_ias_mode            ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_ias_mode            )); //bool                        
    engine->RegisterObjectProperty(obj, "int            ap_ias_value           ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_ias_value           )); //int                         
    engine->RegisterObjectProperty(obj, "bool           ap_gpws_mode           ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_gpws_mode           )); //bool                        
    engine->RegisterObjectProperty(obj, "bool           ap_ils_available       ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_ils_available       )); //bool                        
    engine->RegisterObjectProperty(obj, "float          ap_ils_vdev            ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_ils_vdev            )); //float                       
    engine->RegisterObjectProperty(obj, "float          ap_ils_hdev            ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_ils_hdev            )); //float                       
    engine->RegisterObjectProperty(obj, "int            ap_vs_value            ", asOFFSET(GfxActor::SimBuffer, simbuf_ap_vs_value            )); //int  

    obj = "GfxActor";
    engine->RegisterObjectType(obj, 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectMethod(obj, "ActorSimBuffer& GetSimDataBuffer()", asMETHODPR(GfxActor, GetSimDataBuffer, (void), GfxActor::SimBuffer&), asCALL_THISCALL);

    obj = "GfxScene";
    engine->RegisterObjectType(obj, 0, asOBJ_REF | asOBJ_NOCOUNT);
    engine->RegisterObjectMethod(obj, "GfxActor& GetPlayerGfxActor()", asMETHODPR(GfxScene, GetPlayerGfxActor, (void), GfxActor*), asCALL_THISCALL);

    engine->SetDefaultNamespace("App");

    engine->RegisterGlobalFunction("GfxScene* GetGfxScene()", asFUNCTION(App::GetGfxScene), asCALL_CDECL);

    engine->SetDefaultNamespace(nullptr);
}
