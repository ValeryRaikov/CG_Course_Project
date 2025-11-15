#version 460 core

in vec3 v_normal;
in vec3 v_frag_pos;
in vec2 v_tex_coord;

out vec4 frag_color;

uniform vec3 u_view_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_color;

// Robot part colors
uniform vec3 u_body_color = vec3(0.3, 0.5, 0.8);    // Blue body
uniform vec3 u_head_color = vec3(0.4, 0.6, 0.9);    // Lighter blue head
uniform vec3 u_arm_color = vec3(0.2, 0.4, 0.7);     // Darker blue arms
uniform vec3 u_leg_color = vec3(0.2, 0.4, 0.7);     // Darker blue legs
uniform vec3 u_eye_color = vec3(1.0, 0.0, 0.0);     // Bright red eyes
uniform vec3 u_antenna_color = vec3(1.0, 1.0, 0.0); // Yellow antenna
uniform vec3 u_shoulder_color = vec3(0.3, 0.5, 0.8); // Shoulder color
uniform vec3 u_hip_color = vec3(0.3, 0.5, 0.8);     // Hip color

// Which robot part are we drawing? (passed from C++ code)
uniform int u_robot_part;

vec3 get_part_color_based_on_id()
{
    // Use the uniform to determine color - much more reliable
    switch(u_robot_part) {
        case 0: return u_body_color;      // Body
        case 1: return u_head_color;      // Head
        case 2: return u_arm_color;       // Arms
        case 3: return u_leg_color;       // Legs
        case 4: return u_eye_color;       // Eyes
        case 5: return u_antenna_color;   // Antenna
        case 6: return u_shoulder_color;  // Shoulders
        case 7: return u_hip_color;       // Hips
        case 8: return u_arm_color;       // Forearms
        case 9: return u_leg_color;       // Shins
        default: return vec3(0.5, 0.5, 0.5); // Fallback
    }
}

void main()
{
    // Get the color based on robot part ID
    vec3 object_color = get_part_color_based_on_id();
    
    // Enhanced lighting for eyes to make them stand out
    if(u_robot_part == 4) { // Eyes
        // Make eyes self-illuminating (no lighting calculations)
        frag_color = vec4(u_eye_color * 1.2, 1.0);
        return;
    }
    
    // Standard lighting for other parts
    
    // Ambient lighting - increased for better visibility
    float ambient_strength = 0.5; // Increased from 0.3
    vec3 ambient = ambient_strength * u_light_color;
    
    // Diffuse lighting
    vec3 norm = normalize(v_normal);
    vec3 light_dir = normalize(u_light_pos - v_frag_pos);
    float diff = max(dot(norm, light_dir), 0.0);
    vec3 diffuse = diff * u_light_color;
    
    // Specular lighting - reduced to prevent harsh highlights
    float specular_strength = 0.3; // Reduced from 0.5
    vec3 view_dir = normalize(u_view_pos - v_frag_pos);
    vec3 reflect_dir = reflect(-light_dir, norm);
    float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 16.0); // Reduced shininess
    vec3 specular = specular_strength * spec * u_light_color;
    
    // Combine results with better balance
    vec3 result = (ambient + diffuse + specular) * object_color;
    
    // Add a tiny bit of gamma correction for better appearance
    result = pow(result, vec3(1.0/1.2));
    
    frag_color = vec4(result, 1.0);
}