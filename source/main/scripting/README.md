Rigs of Rods scripting subsystem
================================

Scripting allows you to attach your own programmed actions to terrains,
or to create stand-alone addons.

There are 2 distinct interfaces:
 * Synchronous (classic) interface:
      Used by all terrain scripts and callbacks.
      Lets you directly read and make all sorts of changes to gameplay or graphics.
      Invoked per-frame from main thread while physics are stopped. This includes callbacks.
      
 * Frame step interface:
      Invoked once per frame, but running in parallel with the game simulation.
      You can only read buffered gameplay state, but you can modify graphics or update/draw GUI.

Coding
------

When extending the script interface, prefer to register the C++ classes/functions as-is.
The completed interface should look like this:
```
// Global functions:
RoR::GetGfxScene() // C++ App::GetGfxScene()
RoR::GetConsole()  // C++ App::GetConsole()
// ... etc...

// Classes:
Console + CVar
GfxActor + GfxActorSimBuffer
GfxScene + GfxSceneSimBuffer
// .. etc..
```
The goal is to gradually transfer feature implementations from C++ code to scripts,
keeping the existing logic only for backwards compatibility.

When registering 3rd party APIs (like Ogre/CURL), employ the same practice
- make the AngelScript classes/functions resemble the C++ API as close as possible.

There are some legacy proxy-classes