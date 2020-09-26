// Improved implementation of "Single-Pass Wireframe Rendering" technique.
// Author:  Julian Panetta (jpanetta), julian.panetta@gmail.com
#version 150

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

// Light and material parameters
uniform vec3 lightEyePos;
uniform vec3 diffuseIntensity;
uniform vec3 ambientIntensity;
uniform vec3 specularIntensity;
uniform float shininess;
uniform float alpha;    // Transparency

uniform float lineWidth;

in vec3 v2f_eyePos;
in vec3 v2f_eyeNormal;
in vec4 v2f_color; // Used for ambient, specular, and diffuse reflection constants

// For drawing wireframe
noperspective in vec3 v2f_barycentric; // Barycentric coordinate functions.

out vec4 result;

void main() {
    vec3 L = normalize(lightEyePos - v2f_eyePos);
    vec3 V = normalize(-v2f_eyePos);
    // Use unit normal oriented to always point towards the camera
    vec3 N = sign(dot(v2f_eyeNormal, V)) * normalize(v2f_eyeNormal);
    vec3 R = reflect(-L, N);
    float d = max(dot(L, N), 0.0);

    vec4 color = v2f_color * vec4(ambientIntensity + d * diffuseIntensity
               + (float(d != 0.0) * pow(max(dot(R, V), 0.0), shininess)) * specularIntensity, 1.0);

    // Draw a black wireframe if requested
    if (lineWidth > 0.0) {
        // Trick for implementing "Single-pass Wireframe Rendering" method without a geometry shader:
        // infer height above each triangle edge using (norms of) barycentrentric coordinate gradients.
        vec3 h = v2f_barycentric / sqrt(pow(dFdx(v2f_barycentric), vec3(2.0)) + pow(dFdy(v2f_barycentric), vec3(2.0)));
        float dist = min(min(h.x, h.y), h.z) - 0.5 * lineWidth; // Distance to line border (at lineWidth/2 above h=0)
        color = mix(color, vec4(0.0, 0.0, 0.0, 1.0),
                exp(-pow(max(dist + 0.9124443057840285280, 0.0), 4))); // dist + log(2)^(1/4) centers transition from 1 to 0 around the edge.
    }

    result = color;
    result.a *= alpha;
}
