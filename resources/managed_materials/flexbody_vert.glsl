#version 410

uniform mat4 worldViewProj;
 
attribute vec3 position;
 
void main()
{
    vec3 pos = (position*1.5); // DEBUG TEST
    gl_Position = worldViewProj * vec4(pos, 1.0);    
}