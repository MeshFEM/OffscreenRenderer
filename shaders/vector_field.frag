// Shader for instanced vector field rendering.
// Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
#version 140
varying vec4 v2f_color;
void main() {
    // Gouraud shading
    gl_FragColor = v2f_color;
}
