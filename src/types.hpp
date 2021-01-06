#pragma once

#ifndef ARRAYSIZE
#define ARRAYSIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#endif

union float2
{
    struct { float x; float y; };
    struct { float u; float v; };
    float e[2];
};

union float3
{
    struct { float x; float y; float z; };
    struct { float r; float g; float b; };
    float e[3];
};

union float4
{
    struct { float x; float y; float z; float w; };
    struct { float r; float g; float b; float a; };
    float e[4];
};

union mat4
{
    float4 c[4];
    float e[16];
};
