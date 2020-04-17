#version 410

// Inputs from OGRE
uniform mat4 worldViewProj;

// Inputs from vertex data
attribute vec3 position;
attribute vec2 uv0; // Nodes (packed)
attribute vec2 uv1; // Texcoords

// Outputs to fragment shader
varying vec2 varTexcoord;
 
void main()
{
    vec3 pos = (position*1.5); // DEBUG TEST
    gl_Position = worldViewProj * vec4(pos, 1.0);

    varTexcoord = uv1;
}
