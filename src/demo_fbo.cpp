
#include <cstddef>
#include <vector>
#include <imgui.h>

#include "types.hpp"
#include "calc.hpp"
#include "gl_helpers.hpp"
#include "demo_fbo.hpp"

// Vertex format
struct Vertex
{
    float3 position;
    float2 uv;
};

DemoFBO::DemoFBO(const DemoInputs& inputs)
{
    // Upload vertex buffer
    {
        // In memory
        Vertex* vertices = nullptr;
        int vertexCount = 0;

        {
            VertexDescriptor descriptor = {};
            descriptor.size             = sizeof(Vertex);
            descriptor.positionOffset   = offsetof(Vertex, position);
            descriptor.hasUV            = true;
            descriptor.uvOffset         = offsetof(Vertex, uv);

            MeshBuilder meshBuilder(descriptor, (void**)&vertices, &vertexCount);

            fullscreenQuad = meshBuilder.GenQuad(nullptr, 1.0f, 1.0f);
            obj            = meshBuilder.LoadObj(nullptr, "media/fantasy_game_inn.obj", "media", 1.f);
        }

        // In VRAM
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(Vertex), vertices, GL_STATIC_DRAW);

        free(vertices);
    }

    // Vertex layout
    {
        glGenVertexArrays(1, &vertexArrayObject);
        glBindVertexArray(vertexArrayObject);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const GLvoid*)offsetof(Vertex, uv));
    }

    // Main program
    mainProgram = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec2 aUV;

        out vec4 vColor;
        out vec2 vUV;

        uniform mat4 projection;
        uniform mat4 view;
        uniform mat4 model;

        void main()
        {
            gl_Position = projection * view * model * vec4(aPosition, 1.0);
            vUV = aUV;
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        in vec2 vUV;
        layout(location = 0) out vec4 baseColor;
        layout(location = 1) out vec4 finalColor;

        uniform sampler2D diffuseTexture;  // Texture channel 0
        uniform sampler2D specularTexture; // Texture channel 1

        void main()
        {
            vec3 diffuse  = texture(diffuseTexture, vUV).rgb;
            vec3 specular = texture(specularTexture, vUV).rgb;
            baseColor     = vec4(diffuse + specular * 4, 1.0);
            finalColor    = vec4(specular, 1.0);
        }
        )GLSL"
    );

    // Post process program
    postProcessProgram = gl::CreateBasicProgram(
        // Vertex shader
        R"GLSL(
        layout(location = 0) in vec3 aPosition;
        layout(location = 1) in vec2 aUV;
        out vec2 vUV;

        void main()
        {
            gl_Position = vec4(aPosition, 1.0);
            vUV = aUV;
        }
        )GLSL",

        // Fragment shader
        R"GLSL(
        in vec2 vUV;
        layout(location = 0) out vec4 fragColor;

        uniform sampler2D colorTexture;
        uniform mat4 colorTransform;

        void main()
        {
            fragColor = texture(colorTexture, vUV);
        }
        )GLSL"
    );

    // Load diffuse/specular texture
    {
        glGenTextures(1, &diffuseTexture);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);
        gl::UploadImage("media/fantasy_game_inn_diffuse.png");
        gl::SetTextureDefaultParams();

        glGenTextures(1, &specularTexture);
        glBindTexture(GL_TEXTURE_2D, specularTexture);
        gl::UploadImage("media/fantasy_game_inn_emissive.png");
        gl::SetTextureDefaultParams();
    }

    // Create framebuffer (for post process pass)
    framebuffer.Generate((int)inputs.windowSize.x, (int)inputs.windowSize.y);
}

void DemoFBO::Framebuffer::Generate(int width, int height)
{
    // Create base buffer
    {
        glGenTextures(1, &baseTexture);
        glBindTexture(GL_TEXTURE_2D, baseTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Create final buffer
    {
        glGenTextures(1, &finalTexture);
        glBindTexture(GL_TEXTURE_2D, finalTexture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    // Create depth buffer
    {
        glGenRenderbuffers(1, &depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    glGenFramebuffers(1, &id);
    glBindFramebuffer(GL_FRAMEBUFFER, id);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, baseTexture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, finalTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

    GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBuffers);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(status == GL_FRAMEBUFFER_COMPLETE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DemoFBO::Framebuffer::Resize(int width, int height)
{
    this->width = width;
    this->height = height;
    glBindTexture(GL_TEXTURE_2D, baseTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, finalTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
}

void DemoFBO::Framebuffer::Delete()
{
    glDeleteFramebuffers(1, &id);
    glDeleteTextures(1, &baseTexture);
    glDeleteTextures(1, &finalTexture);
    glDeleteRenderbuffers(1, &depthRenderbuffer);
}

DemoFBO::~DemoFBO()
{
    // Delete OpenGL objects
    framebuffer.Delete();
    glDeleteTextures(1, &diffuseTexture);
    glDeleteTextures(1, &specularTexture);
    glDeleteProgram(mainProgram);
    glDeleteProgram(postProcessProgram);
    glDeleteVertexArrays(1, &vertexArrayObject);
    glDeleteBuffers(1, &vertexBuffer);
}

void DemoFBO::UpdateAndRender(const DemoInputs& inputs)
{
    // Resize framebuffer if needed
    if ((int)inputs.windowSize.x != framebuffer.width || (int)inputs.windowSize.y != framebuffer.height)
        framebuffer.Resize((int)inputs.windowSize.x, (int)inputs.windowSize.y);

    // Update camera
    mainCamera.UpdateFreeFly(inputs.cameraInputs);

    // Show debug info
    static bool finalImage = false;
    ImGui::Checkbox("final render", &finalImage);
    ImVec2 imageSize = { 256, 256 };
    ImGui::Image((ImTextureID)(size_t)framebuffer.baseTexture, imageSize,  ImVec2(0, 1), ImVec2(1, 0));
    ImGui::Image((ImTextureID)(size_t)framebuffer.finalTexture, imageSize, ImVec2(0, 1), ImVec2(1, 0));

    // Setup main program uniforms
    {
        mat4 projection = mat4Perspective(calc::ToRadians(60.f), inputs.windowSize.x / inputs.windowSize.y, 0.01f, 50.f);
        mat4 view       = mainCamera.GetViewMatrix();
        mat4 model      = mat4Scale(2.f);

        glUseProgram(mainProgram);
        glUniformMatrix4fv(glGetUniformLocation(mainProgram, "projection"), 1, GL_FALSE, projection.e);
        glUniformMatrix4fv(glGetUniformLocation(mainProgram, "view"), 1, GL_FALSE, view.e);
        glUniformMatrix4fv(glGetUniformLocation(mainProgram, "model"), 1, GL_FALSE, model.e);

        glUniform1i(glGetUniformLocation(mainProgram, "diffuseTexture"), 0);
        glUniform1i(glGetUniformLocation(mainProgram, "specularTexture"), 1);
    }

    // Setup post process program uniforms
    {
        mat4 colorTransform = //mat4Identity();
        {
            0.299f, 0.299f, 0.299f, 0.0f,
            0.587f, 0.587f, 0.587f, 0.0f,
            0.114f, 0.114f, 0.114f, 0.0f,
            0.000f, 0.000f, 0.000f, 1.0f,
        };

        glUseProgram(postProcessProgram);
        glUniformMatrix4fv(glGetUniformLocation(postProcessProgram, "colorTransform"), 1, GL_FALSE, colorTransform.e);
    }

    // Keep track of previous framebuffer to rebind it after offscreen rendering
    GLint previousFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);

    // =============================================
    // Start rendering
    // =============================================
    glEnable(GL_DEPTH_TEST);

    // We use the same vao for both passes
    glBindVertexArray(vertexArrayObject);

    // Render to framebuffer
    {
        glViewport(0, 0, framebuffer.width, framebuffer.height);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.id);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(mainProgram);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseTexture);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularTexture);

        glDrawArrays(GL_TRIANGLES, obj.start, obj.count);

        glActiveTexture(GL_TEXTURE0);
    }

    // Render to screen
    {
        glViewport(0, 0, (int)inputs.windowSize.x, (int)inputs.windowSize.y);
        glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(postProcessProgram);
        glBindTexture(GL_TEXTURE_2D, finalImage ? framebuffer.finalTexture : framebuffer.baseTexture);
        glDrawArrays(GL_TRIANGLES, fullscreenQuad.start, fullscreenQuad.count);
    }
}
