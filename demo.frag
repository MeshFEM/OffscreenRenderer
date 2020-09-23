#version 150

in  vec4 v4f_color; // Interpolated input read from vertex shader
out vec4 f_color;   // Final color output produced by fragment shader.
                    // (Can name this anything you want...)

void main() {
    // pass through interpolated fragment color
    f_color = v4f_color;
}
