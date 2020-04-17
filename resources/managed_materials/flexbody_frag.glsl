#version 410

// Inputs from OGRE
uniform sampler2D Diffuse_Map;

// Inputs from vertex shader
varying vec2 varTexcoord;

void main()
{
    gl_FragColor = vec4(1,1,0,1);   // DEBUG - BRIGHT YELLOW
        //texture2D(Diffuse_Map, varTexcoord);
}
