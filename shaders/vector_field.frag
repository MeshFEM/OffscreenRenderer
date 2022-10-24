// Shader for instanced vector field rendering.
// Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
#version 140
in vec4 v2f_color;
out vec4 result;

void main() {
    // Gouraud shading
    result = v2f_color;
}
