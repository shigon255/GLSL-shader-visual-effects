#version 330 core
//out vec4 FragColor;
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;

in VS_OUT {
    vec3 Pos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D meshTexture;
uniform samplerCube shadowMap;
uniform sampler2D scene;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform bool meshOrModel;
uniform bool drawShadow;

uniform float far_plane;
uniform float invisible;

vec3 sampleOffsetDirections[20] = vec3[]
(
   vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1), 
   vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
   vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
   vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
   vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

void main()
{           
    // implementing discretized Blinn-Phong model (toon shading)

    // strength of lights
    vec3 light = vec3(1.0f, 1.0f, 1.0f);

    // distance from light
    vec3 lightDir = lightPos - fs_in.Pos;

    // normalize direction vector
    lightDir = normalize(lightDir);

    // colors
    vec3 diffuseColor = meshOrModel? texture(meshTexture, fs_in.TexCoords).xyz : texture(texture_diffuse1, fs_in.TexCoords).xyz;
    vec3 specularColor = meshOrModel? texture(meshTexture, fs_in.TexCoords).xyz : texture(texture_specular1, fs_in.TexCoords).xyz;

    // ambient
    float ka = 0.2;
    vec3 ambient = ka * light;
    vec3 total_ambient = ambient * diffuseColor;

    //diffuse
    float kd = 0.7;
    vec3 diffuse = kd * light; 

    vec3 total_diffuse = diffuse * diffuseColor;

    //specular
    float ks = 0.9;
    float alpha = 10;
    vec3 viewDir = normalize(viewPos - fs_in.Pos);

    vec3 halfway = normalize(lightDir + viewDir);
    vec3 specular = ks * light;
    vec3 total_specular =  specular * specularColor;

    // Toon
    float NdotL = max(dot(fs_in.Normal,lightDir),0);

    vec3 total_color = vec3(0.0, 0.0, 0.0);

    // discretized color into 3 part (Ambient, diffuse, diffuse + specular)
    vec3 total_point_light = vec3(0.0, 0.0, 0.0);
    if( NdotL<0.3)
        total_point_light = total_point_light+total_ambient;
    else if(NdotL>=0.3 && NdotL<0.85)    
        total_point_light = total_point_light+total_diffuse + total_ambient;
    else if(NdotL>=0.85)    
        total_point_light = total_point_light+total_specular+total_diffuse + total_ambient;

    total_color = total_point_light;

    // -----Shadow Test-----
    float shadow = 0.0;
    if(drawShadow)
    {
        // use vector between fragment position(in camera-view) and light position to sample the cubemap
        vec3 fragToLight = fs_in.Pos - lightPos;

        // Compare the distance between light and fragment position under camera-view with that under light-view
        float cameraViewDepth = length(fragToLight);

        // Add the bias preventing from stripe
        // Note that the bias should be large enough since the depth is in [0, far_plane] rather than [0, 1]
        float bias = 0.35;
        int samples = 20;
        float viewDistance = length(viewPos - fs_in.Pos);
        float diskRadius = 0.05;
        for(int i = 0; i < samples; ++i)
        {
            // use vector between fragment position(in camera-view) and light position to sample the cubemap
            float lightViewDepth = texture(shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;

            // Since we stored a normalized depth in cubemap, we use far_plane to transform it back to the original length
            lightViewDepth *= far_plane;

            // If the dis under camera-view > dis under light-view, then this fragment is in the shadow
            if(cameraViewDepth - bias > lightViewDepth)
                shadow += 1.0;
        }
        shadow /= float(samples);
    }

    // if in shadow -> ambient
    // else -> direct
    if(shadow > 0.0)
        total_color = total_ambient;
    else //in shadow
        total_color =  total_point_light;


    //FragColor = vec4(vec3(closestDepth / far_plane), 1.0);

    // invisible
    // trick: if invisible, bring part of the color into brightcolor part to blur it
    if(invisible > 0.1f)
    {
        vec2 screen_coord = vec2(gl_FragCoord) / textureSize(scene, 0);
        vec3 scene_color = texture(scene, screen_coord).xyz;
        total_color = total_ambient;
        total_color = scene_color * invisible + total_color * (1.0-invisible);
        float blur_weight = 0.7f;
        FragColor = vec4(total_color * blur_weight, 1.0);
        BrightColor = vec4(total_color * (1.0 - blur_weight), 1.0);
    }
    else
    {
        // Assign the black color to bright color, telling it that this part is not bright
        FragColor = vec4(total_color, 1.0);
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}