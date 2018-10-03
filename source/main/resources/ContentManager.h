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

    // Inspired by *.skin handler in "gameplay/SkinManager.h|cpp" 
    // TODO: maybe Ogre:::ScriptLoader would be enough? See "SoundScriptManager.h"
    //   ~ only_a_ptr, 10/2018
    class TruckfileResourceManager: public Ogre::ResourceManager
    {
    public:
        TruckfileResourceManager()
        {
            // This makes the magic come true
            mScriptPatterns.push_back("*.machine");
            mScriptPatterns.push_back("*.fixed");
            mScriptPatterns.push_back("*.truck");
            mScriptPatterns.push_back("*.car");
            mScriptPatterns.push_back("*.boat");
            mScriptPatterns.push_back("*.airplane");
            mScriptPatterns.push_back("*.trailer");
            mScriptPatterns.push_back("*.load");
            mScriptPatterns.push_back("*.train");

            mResourceType = "RoR Softbody actors";
            Ogre::ResourceGroupManager::getSingleton()._registerScriptLoader(this); // TODO: maybe Ogre:::ScriptLoader would be enough? See "SoundScriptManager.h"   ~ only_a_ptr, 10/2018
            Ogre::ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
        }

        virtual ~TruckfileResourceManager()
        {
            Ogre::ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
            Ogre::ResourceGroupManager::getSingleton()._unregisterScriptLoader(this);
        }

        // == Ogre::ResourceManager interface functions ==

        Ogre::Resource* createImpl(const Ogre::String& name, Ogre::ResourceHandle handle,
            const Ogre::String& group, bool isManual, Ogre::ManualResourceLoader* loader,
            const Ogre::NameValuePairList* params) override
        {
            return nullptr; // Not used
        }
        void parseScript(Ogre::DataStreamPtr& stream, const Ogre::String& groupName) override
        {
            RoR::LogFormat("[RoR| -- content DB -- ] Ogre::ResourceManager::parseScript() >> filename '%s', group name '%s'", stream->getName().c_str(), groupName.c_str());
        }
        void reloadUnreferencedResources(bool reloadableOnly = true) override {}
        void unloadUnreferencedResources(bool reloadableOnly = true) override {}
        void unload     (const Ogre::String& name) override {}
        void unload     (Ogre::ResourceHandle handle) override {}
        void unloadAll  (bool reloadableOnly = true) override {}
    
        void remove     (Ogre::ResourcePtr& r) override {}
        void remove     (const Ogre::String& name) override{}
        void remove     (Ogre::ResourceHandle handle) override{}
        void removeAll  () override{}
        void reloadAll  (bool reloadableOnly = true) override{}  
    };

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
        skipThisScript = false; // TODO: Skip OGRE materials+programs, but not RoR content
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
