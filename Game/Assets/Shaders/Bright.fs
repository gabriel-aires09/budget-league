#version 330

// Bright pass of the bloom chain: keeps only what is brighter than the
// threshold and rescales it, so a dim scene blooms nothing at all.
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform float threshold; // luma above which a pixel starts to bloom

out vec4 finalColor;

void main()
{
    vec4 source = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    float luma = dot(source.rgb, vec3(0.299, 0.587, 0.114));
    // Scaled by how far past the threshold it is rather than passed straight
    // through, which keeps the boost pads from blowing out the whole floor.
    float weight = max(luma - threshold, 0.0) / max(1.0 - threshold, 0.0001);
    finalColor = vec4(source.rgb * weight, 1.0);
}
