/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2015 Petr Ohlidal

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

#pragma once

#include "Application.h" // Logging
#include "BitFlags.h"
#include "RoRPrerequisites.h"
#include "Singleton.h"

#include <OgreResourceGroupManager.h>

namespace RoR {

class TestRGListener: public Ogre::ResourceGroupListener
{
public:
    // experimental prototype, DO NOT MERGE!  ~ only_a_ptr, 10/2018
    TestRGListener(): m_loading_vehicles(false) {}

    virtual void resourceGroupPrepareStarted(const Ogre::String& groupName, size_t resourceCount) override
    {
        // --------
        // TO BE DISCOVERED: what does this callback mean? I thought it was when 'initializeResourceGroup()' is called, but apparently not.   ~ only_a_ptr, 10/2018
        // --------
        RoR::LogFormat("[RoR| -- content DB -- ] resourceGroupPrepareStarted: %s", groupName.c_str());
        if (groupName == "VehicleFolders")
        {
            RoR::Log("[RoR| -- content DB -- ] Loading vehicles!");
            m_loading_vehicles = true; // TO BE DISCOVERED: is it sequential like this?   ~ only_a_ptr, 10/2018
        }
    }

    virtual void resourceGroupPrepareEnded(const Ogre::String& groupName) override
    {
        // --------
        // TO BE DISCOVERED: what does this callback mean? I thought it was when 'initializeResourceGroup()' returns, but apparently not.   ~ only_a_ptr, 10/2018
        // --------
        RoR::LogFormat("[RoR| -- content DB -- ] resourceGroupPrepareEnded: %s", groupName.c_str());
        if (groupName == "VehicleFolders")
        {
            RoR::Log("[RoR| -- content DB -- ] Done loading vehicles!");
            m_loading_vehicles = false; // TO BE DISCOVERED: is it sequential like this?  ~ only_a_ptr, 10/2018
        }
    }

    virtual void scriptParseStarted(const Ogre::String& scriptName, bool& skipThisScript) override
    {
        // NOTE: yes, this works the way you'd expect, when 'initializeResourceGroup()' is running.
        RoR::LogFormat("[RoR| -- content DB -- ] ScriptParseStarted: %s", scriptName.c_str());
        skipThisScript = true; // skip all!
    }

    virtual void resourceGroupScriptingStarted(const Ogre::String& groupName, size_t scriptCount) override {}

    virtual void resourceGroupScriptingEnded(const Ogre::String& groupName) override {}

	virtual void resourceGroupLoadStarted(const Ogre::String& groupName, size_t resourceCount) override {}

	virtual void resourceLoadStarted(const Ogre::ResourcePtr& resource) override {}

    virtual void resourceLoadEnded(void) override {}

    virtual void worldGeometryStageStarted(const Ogre::String& description) override {}

    virtual void worldGeometryStageEnded(void) override {}
        
	virtual void resourceGroupLoadEnded(const Ogre::String& groupName) override {}

    virtual void scriptParseEnded(const Ogre::String& scriptName, bool skipped) override {}

private:

    bool m_loading_vehicles; // TO BE DISCOVERED: is it sequential? ~ only_a_ptr, 10/2018
};

class ContentManager : public Ogre::ResourceLoadingListener, public ZeroedMemoryAllocator
{
public:

    ContentManager();
    ~ContentManager();

    struct ResourcePack
    {
        ResourcePack(const char* name, const char* resource_group_name):
            name(name), resource_group_name(resource_group_name)
        {}

        static const ResourcePack OGRE_CORE;
        static const ResourcePack WALLPAPERS;
        static const ResourcePack AIRFOILS;
        static const ResourcePack BEAM_OBJECTS;
        static const ResourcePack BLUR;
        static const ResourcePack CAELUM;
        static const ResourcePack CUBEMAPS;
        static const ResourcePack DASHBOARDS;
        static const ResourcePack DEPTH_OF_FIELD;
        static const ResourcePack FAMICONS;
        static const ResourcePack FLAGS;
        static const ResourcePack GLOW;
        static const ResourcePack HDR;
        static const ResourcePack HEATHAZE;
        static const ResourcePack HYDRAX;
        static const ResourcePack ICONS;
        static const ResourcePack MATERIALS;
        static const ResourcePack MESHES;
        static const ResourcePack MYGUI;
        static const ResourcePack OVERLAYS;
        static const ResourcePack PAGED;
        static const ResourcePack PARTICLES;
        static const ResourcePack PSSM;
        static const ResourcePack SKYX;
        static const ResourcePack RTSHADER;
        static const ResourcePack SCRIPTS;
        static const ResourcePack SOUNDS;
        static const ResourcePack SUNBURN;
        static const ResourcePack TEXTURES;

        const char* name;
        const char* resource_group_name;
    };

    void               AddResourcePack(ResourcePack const& resource_pack); //!< Loads resources if not already loaded (currently resources never unload until shutdown)
    bool               OnApplicationStartup(void);
    void               InitManagedMaterials();
    void               LoadGameplayResources();  //!< Checks GVar settings and loads required resources.
    void               RegenCache();
    RoR::SkinManager*  GetSkinManager()  { return m_skin_manager; }

protected:

    void exploreFolders(Ogre::String rg);
    void exploreZipFolders(Ogre::String rg);

    // implementation for resource loading listener
    Ogre::DataStreamPtr resourceLoading(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource);
    void resourceStreamOpened(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource, Ogre::DataStreamPtr& dataStream);
    bool resourceCollision(Ogre::Resource* resource, Ogre::ResourceManager* resourceManager);

    RoR::SkinManager* m_skin_manager;
    bool              m_base_resource_loaded;
};

} // namespace RoR
