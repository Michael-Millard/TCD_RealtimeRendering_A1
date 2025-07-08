#version 330 core

in vec3 FragPos;    // Fragment position in world space
in vec3 Normal;     // Normal vector at the fragment
in vec3 LightDir;   // Direction vector from fragment to light source
in vec3 ViewDir;    // Direction vector from fragment to the camera

out vec4 FragColor; // Output colour

uniform vec3 lightColour;    // Light colour
uniform vec3 objectColour;   // Object base colour
uniform int lightingID;      // ID for selecting the lighting model
uniform vec3 ambient;        // Ambient light

// Uniforms specific to models
uniform float specularExponent;  // For Phong and Blinn models
uniform float roughness;         // For Cook-Torrance
uniform float fresnelReflectance; // Fresnel for Cook-Torrance

void main() 
{
    // Normalize input vectors
    vec3 N = normalize(Normal);
    vec3 L = normalize(LightDir);
    vec3 V = normalize(ViewDir);

    vec3 colour = vec3(0.0);
    vec3 ambientLight = ambient * objectColour;

    // Lambertian Lighting
    if (lightingID == 0) 
    {
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diff * lightColour * objectColour;
        colour = ambient + diffuse;
    } 
    // Phong Lighting
    else if (lightingID == 1) 
    {
        vec3 R = reflect(-L, N); // Reflection vector
        float spec = pow(max(dot(V, R), 0.0), specularExponent);
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diff * lightColour * objectColour;
        vec3 specular = spec * lightColour;
        colour = ambient + diffuse + specular;
    } 

    // Cook-Torrance Lighting (* Equations with help from ChatGPT)
    else if (lightingID == 2) 
    {
        vec3 H = normalize(L + V); // Halfway vector
        float NdotL = max(dot(N, L), 0.0);
        float NdotV = max(dot(N, V), 0.0);
        float NdotH = max(dot(N, H), 0.0);
        float VdotH = max(dot(V, H), 0.0);

        // D = Beckmann distribution
        float m2 = roughness * roughness;
        float D = exp((NdotH * NdotH - 1.0) / (m2 * NdotH * NdotH)) / (3.1415 * m2 * pow(NdotH, 4.0));

        // F = Fresnel term
        float F = fresnelReflectance + (1.0 - fresnelReflectance) * pow(1.0 - VdotH, 5.0);

        // G = Geometry attenuation
        float G = min(1.0, min(2.0 * NdotH * NdotV / VdotH, 2.0 * NdotH * NdotL / VdotH));

        // Specular component
        vec3 specular = lightColour * (D * F * G) / (4.0 * NdotL * NdotV + 0.001);

        // Diffuse component
        vec3 diffuse = NdotL * lightColour * objectColour;

        // Final colour
        colour = ambient + diffuse + specular;
    } 
    // Blinn Lighting
    else if (lightingID == 3) 
    {
        vec3 H = normalize(L + V); // Halfway vector
        float spec = pow(max(dot(N, H), 0.0), specularExponent);
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = diff * lightColour * objectColour;
        vec3 specular = spec * lightColour;
        colour = ambient + diffuse + specular;
    }

    FragColor = vec4(colour, 1.0);
}

