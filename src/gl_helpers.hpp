#pragma once


#include <glad/glad.h>

namespace gl
{
    GLuint CreateShader(GLenum type, int sourceCount, const char** sources);
    GLuint CreateBasicProgram(const char* vsStr, const char* fsStr);
    void UploadPerlinNoise(int width, int height, float z, float lacunarity = 2.f, float gain = 0.5f, float offset = 1.f, int octaves = 6);
    void UploadImage(const char* file);
    void SetTextureDefaultParams(bool genMipmap = true);
}