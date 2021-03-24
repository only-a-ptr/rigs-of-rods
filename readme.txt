Rigs of Rods - v0.4.0.7 retro edition
=====================================

INTRO
-----

This branch is based on v0.4.0.7 source code from year 2013
and updated to use recent components (OGRE 1.11.6, MyGUI 3.4.0).

The purpose is to have a working instance of the original
truck parser/spawner logic, for diagnosing defects
in the current upstream parser.

EXTRAS
------

RoR.cfg setting "vehicleOutputFile" now recognizes "txt" extension
and dumps a detailed diagnostic output. In this case, only filename
in RoR.cfg is respected, path is overriden to
Documents\Rigs of Rods 0.4\logs
 
BUILDING
--------

Only tested on Windows10 + MSVC2019, needs a lot of manual adjustments.

1. run cmake; build outside the source tree

2. set ROR_DEPENDENCIES dir; you'll need to manually assemble the includes/libs to the expected layout:
    includes/x64:
        AngelScript\
        moFileReader\
        MyGUI\
        OGRE\
        OIS\
        OpenAL\
        PagedGeometry\
    libs/x64:
        moFileReader\
        MyGUI\ ~ MyGUIEngine.lib + MyGUI.OgrePlatform.lib (from conan), freetype.lib (from latest conan OGREDeps)
        OGRE\
        OIS\
        OpenAL\
        PagedGeometry\
        pthread\ ~ pthreadVC3.lib (from vcpkg)
    
   
3. generate visual studio project

4. select build config: RelWithDebInfo
    
    

LICENSE
-------

Copyright 2005-2013 Pierre-Michel Ricordel
Copyright 2007-2013 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as 
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.

