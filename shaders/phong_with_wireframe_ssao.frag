// Phong shader with wireframe and SSAO support
// Based on phong_with_wireframe.frag with added SSAO
#version 140

// Light and material parameters
uniform vec3 lightEyePos;
uniform vec3 diffuseIntensity;
uniform vec3 ambientIntensity;
uniform vec3 specularIntensity;
uniform float shininess;
uniform float alpha;    // Transparency

uniform float lineWidth;

// SSAO parameters
uniform bool ssaoEnabled;
uniform float ssaoRadius;
uniform float ssaoBias;
uniform int ssaoSamples;

// Fragment shader inputs
in vec3 v2f_eyePos;
in vec3 v2f_eyeNormal;
in vec4 v2f_color; // Used for ambient, specular, and diffuse reflection constants
in vec4 v2f_wireframe_color;

// For drawing wireframe
noperspective in vec3 v2f_barycentric; // Barycentric coordinate functions.

// Fragment shader output (pixel color)
out vec4 result;

// Simple pseudo-random function for SSAO sampling
float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

// Simple SSAO approximation using screen-space derivatives
float computeSSAO() {
    if (!ssaoEnabled) return 1.0;
    
    vec3 N = normalize(v2f_eyeNormal);
    vec3 V = normalize(-v2f_eyePos);
    
    // Use depth-based occlusion approximation
    // This is a simplified SSAO that works without additional render passes
    float depth = length(v2f_eyePos);
    vec3 ddxPos = dFdx(v2f_eyePos);
    vec3 ddyPos = dFdy(v2f_eyePos);
    
    // Sample surrounding geometry using derivatives
    float occlusion = 0.0;
    int samples = min(ssaoSamples, 16); // Limit samples for performance
    
    for (int i = 0; i < samples; i++) {
        float angle = float(i) * 3.14159265 * 2.0 / float(samples);
        vec2 offset = vec2(cos(angle), sin(angle)) * ssaoRadius;
        
        vec3 samplePos = v2f_eyePos + ddxPos * offset.x + ddyPos * offset.y;
        float sampleDepth = length(samplePos);
        
        // Simple depth comparison
        if (sampleDepth < depth - ssaoBias) {
            occlusion += 1.0;
        }
    }
    
    return 1.0 - (occlusion / float(samples));
}

void main() {
    vec3 L = normalize(lightEyePos - v2f_eyePos);
    vec3 V = normalize(-v2f_eyePos);
    // Use unit normal oriented to always point towards the camera
    vec3 N = sign(dot(v2f_eyeNormal, V)) * normalize(v2f_eyeNormal);
    vec3 R = reflect(-L, N);
    float d = max(dot(L, N), 0.0);

    // Compute SSAO factor
    float aoFactor = computeSSAO();
    
    // Apply SSAO to ambient lighting
    vec3 ambientWithAO = ambientIntensity * aoFactor;
    
    vec4 color = v2f_color * vec4(ambientWithAO + d * diffuseIntensity, alpha);
    if (d != 0.0) color.rgb += pow(max(dot(R, V), 0.0), shininess) * specularIntensity; // Use white specular highlights regardless of material's color

    // Draw a black wireframe if requested
    if (lineWidth > 0.0) {
        // Clamp now so that the wireframe antialiasing doesn't get blown out.
        color = clamp(color, 0.0, 1.0);

        // Trick for implementing "Single-pass Wireframe Rendering" method without a geometry shader:
        // infer height above each triangle edge using (norms of) barycentrentric coordinate gradients.
        vec3 h = v2f_barycentric / sqrt(pow(dFdx(v2f_barycentric), vec3(2.0)) + pow(dFdy(v2f_barycentric), vec3(2.0)));
        float dist = min(min(h.x, h.y), h.z) - 0.5 * lineWidth; // Distance to line border (at lineWidth/2 above h=0)
        color = mix(color, v2f_wireframe_color,
                    exp(-pow(max(dist + 0.9124443057840285280, 0.0), 4))); // dist + log(2)^(1/4) centers transition from 1 to 0 around the edge.
    }

    result = color;
}
