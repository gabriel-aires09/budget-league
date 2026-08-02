#version 330

// Attribute and uniform names are raylib's defaults, which is what lets raylib
// bind them automatically for both DrawModel and the rlgl batch.
in vec3 vertexPosition;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragPosition;
out vec4 fragColor;

void main()
{
    // World space position. The fragment shader differentiates it to recover the
    // face normal, so no vertex normal is needed here.
    fragPosition = vec3(matModel*vec4(vertexPosition, 1.0));
    fragColor = vertexColor;

    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
