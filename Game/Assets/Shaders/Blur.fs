#version 330

// One axis of a separable gaussian blur. The bloom chain runs it twice, once
// horizontally and once vertically, which is far cheaper than a 2D kernel.
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

uniform vec2 direction; // one texel along the axis being blurred

out vec4 finalColor;

void main()
{
    // Five taps at the standard 1 4 6 4 1 weights, spaced two texels apart so
    // the same tap count covers twice the radius.
    vec3 sum = texture(texture0, fragTexCoord).rgb * 6.0;
    sum += texture(texture0, fragTexCoord + direction * 2.0).rgb * 4.0;
    sum += texture(texture0, fragTexCoord - direction * 2.0).rgb * 4.0;
    sum += texture(texture0, fragTexCoord + direction * 4.0).rgb;
    sum += texture(texture0, fragTexCoord - direction * 4.0).rgb;

    finalColor = vec4(sum / 16.0, 1.0);
}
