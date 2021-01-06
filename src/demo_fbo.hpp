#pragma once

#include "glad/glad.h"

#include "mesh_builder.hpp"

#include "demo.hpp"

class DemoFBO : public Demo
{
public:
    DemoFBO(const DemoInputs& inputs);
    ~DemoFBO() override;
    void UpdateAndRender(const DemoInputs& inputs) override;
    const char* Name() const override { return "FBO"; }

private:
    struct Framebuffer
    {
        void Generate(int width, int height);
        void Resize(int width, int height);
        void Delete();

        int width = 0;
        int height = 0;

        GLuint id = 0;
        GLuint baseTexture = 0;
        GLuint finalTexture = 0;
        GLuint depthRenderbuffer = 0;
    };

    Camera mainCamera = {};

    GLuint vertexBuffer = 0;
    GLuint vertexArrayObject = 0;

    // First pass data (render offscreen)
    Framebuffer framebuffer = {};
    GLuint mainProgram = 0;
    GLuint diffuseTexture = 0;
    GLuint specularTexture = 0;
    MeshSlice fullscreenQuad = {};

    // Second pass data (postprocess)
    GLuint postProcessProgram = 0;
    MeshSlice obj = {};
};