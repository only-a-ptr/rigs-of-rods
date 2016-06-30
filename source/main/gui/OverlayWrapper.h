/*
	This source file is part of Rigs of Rods
	Copyright 2005-2012 Pierre-Michel Ricordel
	Copyright 2007-2012 Thomas Fischer
	Copyright 2013-2014 Petr Ohlidal

	For more information, see http://www.rigsofrods.com/

	Rigs of Rods is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 3, as
	published by the Free Software Foundation.

	Rigs of Rods is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/

/** 
	@file   OverlayWrapper.h
	@author Thomas Fischer
	@date   6th of May 2010
*/

#pragma once

#include "RoRPrerequisites.h"

#include <OgreTextAreaOverlayElement.h>
#include <OIS.h>

class OverlayWrapper : public ZeroedMemoryAllocator
{
	friend class RoRFrameListener;
	friend class RoR::Application;
	friend class RoR::MainThread;

public:

	struct LoadedOverlay
	{
		float orgScaleX;
		float orgScaleY;
		Ogre::v1::Overlay *o;
	};

	void showDashboardOverlays(bool show, Beam *truck);
	void showDebugOverlay(int mode);
	void showPressureOverlay(bool show);

	void windowResized();
	void resizeOverlay(LoadedOverlay & overlay);

	int getDashBoardHeight();

	bool mouseMoved(const OIS::MouseEvent& _arg);
	bool mousePressed(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
	bool mouseReleased(const OIS::MouseEvent& _arg, OIS::MouseButtonID _id);
	float mTimeUntilNextToggle;

	void SetupDirectionArrow();

	void UpdateDirectionArrow(Beam* vehicle, Ogre::Vector3 const & point_to);

	void HideDirectionOverlay();

	void ShowDirectionOverlay(Ogre::String const & caption);

	void UpdatePressureTexture(float pressure);

	void UpdateLandVehicleHUD(Beam * vehicle);

	void UpdateAerialHUD(Beam * vehicle);

	void UpdateMarineHUD(Beam * vehicle);

	void ShowRacingOverlay();

	void HideRacingOverlay();

	/** Hides all overlays, but doesn't change visibility flags (for further restoring).
	*/
	void TemporarilyHideAllOverlays(Beam *current_vehicle);

	/** Shows all overlays flagged as "visible".
	*/
	void RestoreOverlaysVisibility(Beam *current_vehicle);

protected:

	/**
	* RoR needs to temporarily hide all overlays when player enters editor. 
	* However, OGRE only provides per-overlay show() and hide() functionality.
	* Thus, an external state must be kept to restore overlays after exiting the editor.
	*/
	struct VisibleOverlays
	{
		static const int DIRECTION_ARROW              = BITMASK(1);
		static const int DEBUG_FPS_MEMORY             = BITMASK(2);
		static const int DEBUG_BEAM_TIMING            = BITMASK(3);
		static const int RACING                       = BITMASK(4);
		static const int TRUCK_TIRE_PRESSURE_OVERLAY  = BITMASK(5);
	};

	OverlayWrapper();
	~OverlayWrapper();

	int init();
	void update(float dt);
	void resizePanel(Ogre::v1::OverlayElement *oe);
	void reposPanel(Ogre::v1::OverlayElement *oe);
	void placeNeedle(Ogre::SceneNode *node, float x, float y, float len);
	void updateStats(bool detailed=false);

	Ogre::v1::Overlay *loadOverlay(Ogre::String name, bool autoResizeRation=true);
	Ogre::v1::OverlayElement *loadOverlayElement(Ogre::String name);

	Ogre::RenderWindow* win;
	TruckHUD *truckhud;

	// -------------------------------------------------------------
	// Overlays
	// -------------------------------------------------------------

	unsigned int  m_visible_overlays;

	Ogre::v1::Overlay *m_truck_dashboard_overlay;
	Ogre::v1::Overlay *m_truck_dashboard_needles_overlay;
	Ogre::v1::Overlay *m_truck_dashboard_needles_mask_overlay;
	Ogre::v1::Overlay *m_truck_pressure_overlay;
	Ogre::v1::Overlay *m_truck_pressure_needle_overlay;

	Ogre::v1::Overlay *m_aerial_dashboard_overlay;
	Ogre::v1::Overlay *m_aerial_dashboard_needles_overlay;

	Ogre::v1::Overlay *m_marine_dashboard_overlay;
	Ogre::v1::Overlay *m_marine_dashboard_needles_overlay;

	Ogre::v1::Overlay *m_machine_dashboard_overlay;

	// Misc
	Ogre::v1::Overlay *m_direction_arrow_overlay;
	Ogre::v1::Overlay *m_debug_fps_memory_overlay;
	Ogre::v1::Overlay *m_debug_beam_timing_overlay;	
	Ogre::v1::Overlay *m_racing_overlay;

	// -------------------------------------------------------------
	// Overlay elements
	// -------------------------------------------------------------

	// Truck
	Ogre::v1::OverlayElement* guiGear;      //!< truck
	Ogre::v1::OverlayElement* guiGear3D;    //!< truck
	Ogre::v1::OverlayElement* guiRoll;      //!< truck
	Ogre::v1::OverlayElement* guipedclutch; //!< truck
	Ogre::v1::OverlayElement* guipedbrake;  //!< truck
	Ogre::v1::OverlayElement* guipedacc;    //!< truck
	Ogre::v1::OverlayElement *pbrakeo;      //!< truck
	Ogre::v1::OverlayElement *tcontrolo;    //!< truck
	Ogre::v1::OverlayElement *antilocko;    //!< truck
	Ogre::v1::OverlayElement *lockedo;      //!< truck
	Ogre::v1::OverlayElement *securedo;     //!< truck
	Ogre::v1::OverlayElement *lopresso;     //!< truck
	Ogre::v1::OverlayElement *clutcho;      //!< truck
	Ogre::v1::OverlayElement *lightso;      //!< truck
	Ogre::v1::OverlayElement *batto;        //!< truck
	Ogre::v1::OverlayElement *igno;         //!< truck

	// Aerial overlay elements
	Ogre::v1::OverlayElement *thro1;
	Ogre::v1::OverlayElement *thro2;
	Ogre::v1::OverlayElement *thro3;
	Ogre::v1::OverlayElement *thro4;
	Ogre::v1::OverlayElement *engfireo1;
	Ogre::v1::OverlayElement *engfireo2;
	Ogre::v1::OverlayElement *engfireo3;
	Ogre::v1::OverlayElement *engfireo4;
	Ogre::v1::OverlayElement *engstarto1;
	Ogre::v1::OverlayElement *engstarto2;
	Ogre::v1::OverlayElement *engstarto3;
	Ogre::v1::OverlayElement *engstarto4;

	// Marine overlay elements
	Ogre::v1::OverlayElement *bthro1;
	Ogre::v1::OverlayElement *bthro2;

	// Truck
	Ogre::v1::TextAreaOverlayElement* guiAuto[5];
	Ogre::v1::TextAreaOverlayElement* guiAuto3D[5];

	// Truck (m_racing_overlay)
	Ogre::v1::TextAreaOverlayElement* laptimemin;
	Ogre::v1::TextAreaOverlayElement* laptimes;
	Ogre::v1::TextAreaOverlayElement* laptimems;
	Ogre::v1::TextAreaOverlayElement* lasttime;
	Ogre::v1::TextAreaOverlayElement* directionArrowText;
	Ogre::v1::TextAreaOverlayElement* directionArrowDistance;

	Ogre::v1::TextAreaOverlayElement* alt_value_taoe; //!!< Aerial

	Ogre::v1::TextAreaOverlayElement* boat_depth_value_taoe; //!< Marine

	// Aerial
	Ogre::TextureUnitState *adibugstexture;
	Ogre::TextureUnitState *aditapetexture;
	Ogre::TextureUnitState *hsirosetexture;
	Ogre::TextureUnitState *hsibugtexture;
	Ogre::TextureUnitState *hsivtexture;
	Ogre::TextureUnitState *hsihtexture;

	// truck
	Ogre::TextureUnitState *speedotexture;
	Ogre::TextureUnitState *tachotexture;
	Ogre::TextureUnitState *rolltexture;
	Ogre::TextureUnitState *pitchtexture;
	Ogre::TextureUnitState *rollcortexture;
	Ogre::TextureUnitState *turbotexture;

	// Aerial
	Ogre::TextureUnitState *airspeedtexture;
	Ogre::TextureUnitState *altimetertexture;
	Ogre::TextureUnitState *vvitexture;
	Ogre::TextureUnitState *aoatexture;

	// Marine
	Ogre::TextureUnitState *boatspeedtexture;
	Ogre::TextureUnitState *boatsteertexture;

	// Truck
	Ogre::TextureUnitState *pressuretexture;
	
	// Aerial
	Ogre::TextureUnitState *airrpm1texture;
	Ogre::TextureUnitState *airrpm2texture;
	Ogre::TextureUnitState *airrpm3texture;
	Ogre::TextureUnitState *airrpm4texture;
	Ogre::TextureUnitState *airpitch1texture;
	Ogre::TextureUnitState *airpitch2texture;
	Ogre::TextureUnitState *airpitch3texture;
	Ogre::TextureUnitState *airpitch4texture;
	Ogre::TextureUnitState *airtorque1texture;
	Ogre::TextureUnitState *airtorque2texture;
	Ogre::TextureUnitState *airtorque3texture;
	Ogre::TextureUnitState *airtorque4texture;

	// Aerial + Marine: Written in init(), read-only in simulation.
	float thrtop;
	float thrheight;
	float throffset;

	bool m_flipflop;

	// Truck m_racing_overlay overlay
	Ogre::SceneNode* m_direction_arrow_node;

	std::vector<LoadedOverlay> m_loaded_overlays;
};
