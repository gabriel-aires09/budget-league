#version 330

in vec3 fragPosition;
in vec4 fragColor;

uniform vec4 colDiffuse;

uniform vec3 sunDirection; // from the surface towards the light, normalised
uniform vec3 sunColor;
uniform vec3 skyColor;    // hemisphere fill received by upward facing surfaces
uniform vec3 groundColor; // bounce received by downward facing surfaces

out vec4 finalColor;

void main()
{
    // Flat shading, derived rather than interpolated: the screen space
    // derivatives of the world position give the true face normal, constant
    // across the triangle, whatever normals the mesh was authored with.
    //
    // This is what keeps the whole scene flat shaded with one shader. The cooked
    // car models already carry per-face normals, but raylib's generated spheres
    // and cylinders carry smooth ones, and reading those would leave the ball
    // looking like a beach ball next to hard edged cars.
    vec3 geometric = cross(dFdx(fragPosition), dFdy(fragPosition));
    // Wireframe and line drawing produce degenerate derivatives; fall back to up.
    vec3 normal = (dot(geometric, geometric) > 1e-12)? normalize(geometric) : vec3(0.0, 1.0, 0.0);
    if (!gl_FrontFacing) normal = -normal;

    float key = max(dot(normal, sunDirection), 0.0);
    vec3 fill = mix(groundColor, skyColor, normal.y*0.5 + 0.5);

    vec4 base = colDiffuse*fragColor;
    finalColor = vec4(base.rgb*(fill + sunColor*key), base.a);
}
