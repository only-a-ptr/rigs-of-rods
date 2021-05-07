Addons
======

Addons are AngelScript-powered feature packages for Rigs of Rods.

Addons are mods (ZIPs or directories under 
  'userprofile/Documents/My Games/Rigs of Rods/mods')
  with an INI-syntax config file '*.addon'.

All addons found during cache update are assembled under
  menu "Addons" in the console window (hotkey ~)
  with actions "Load" and "AutoLoad". 

An <.addon> file lists script file names (.as) 
  and entry points (script function names).
  
Addon scripts are initialized once using builtin 'setup()'
  function, the other builtin 'loop()' function is then
  invoked on every rendered frame (in both menu and sim).
  
------------------- example.addon -------------------
[General]
Name = Example

[Scripts]
example.as

------------------- example.as -------------------

int setup(string arg)
{
    string msg;
    msg = "setup() works! arg: " + arg;
    log(msg);
    return 0;
}

int frame_num = 0;

int loop()
{
    string msg;
    msg = "Frame: " + frame_num;
    // Opens window "Debug" (DearIMGUI feat)
    ImGui::Text(msg);
    frame_num++;
    return 0;
}

