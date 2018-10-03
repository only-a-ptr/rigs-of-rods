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

    // Inspired by *.soundscript handler - See "SoundScriptManager.h"
    //   ~ only_a_ptr, 10/2018
    class TruckfileScriptLoader: public Ogre::ScriptLoader
    {
    public:
        TruckfileScriptLoader()
        {
            // for OGRE to recognize
            m_filename_patterns.push_back("*.machine");
            m_filename_patterns.push_back("*.fixed");
            m_filename_patterns.push_back("*.truck");
            m_filename_patterns.push_back("*.car");
            m_filename_patterns.push_back("*.boat");
            m_filename_patterns.push_back("*.airplane");
            m_filename_patterns.push_back("*.trailer");
            m_filename_patterns.push_back("*.load");
            m_filename_patterns.push_back("*.train");
        }

        virtual ~TruckfileScriptLoader()
        {
            Ogre::ResourceGroupManager::getSingleton()._unregisterScriptLoader(this);
        }

    // ===== Ogre::ScriptLoader interface =====

        const Ogre::StringVector& getScriptPatterns(void) const override
        {
            return m_filename_patterns;
        }

        void parseScript(Ogre::DataStreamPtr& stream, const Ogre::String& groupName) override
        {
            RoR::LogFormat("[RoR| -- content DB -- ] Ogre::ScriptLoader::parseScript() >> filename '%s', group name '%s'", stream->getName().c_str(), groupName.c_str());
        }

        Ogre::Real getLoadingOrder(void) const override { return 0.f;  } // whatever!

    private:
         Ogre::StringVector m_filename_patterns;
    };

class UserContentRGListener: public Ogre::ResourceGroupListener
{
public:
    // experimental prototype, DO NOT MERGE!  ~ only_a_ptr, 10/2018
    UserContentRGListener() {}

    virtual void scriptParseStarted(const Ogre::String& scriptName, bool& skipThisScript) override
    {
        // NOTE: yes, this works the way you'd expect, when 'initializeResourceGroup()' is running.
        RoR::LogFormat("[RoR| -- content DB -- ] ScriptParseStarted: %s", scriptName.c_str());


        skipThisScript = 
            Ogre::StringUtil::match(scriptName, "*.material", false) ||
            Ogre::StringUtil::match(scriptName, "*.program", false);
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
    void RecurseArchives(Ogre::String const & rg, Ogre::String const & pattern);

    // implementation for resource loading listener
    Ogre::DataStreamPtr resourceLoading(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource);
    void resourceStreamOpened(const Ogre::String& name, const Ogre::String& group, Ogre::Resource* resource, Ogre::DataStreamPtr& dataStream);
    bool resourceCollision(Ogre::Resource* resource, Ogre::ResourceManager* resourceManager);

    RoR::SkinManager* m_skin_manager;
    bool              m_base_resource_loaded;
};

} // namespace RoR
