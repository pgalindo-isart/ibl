
#include <tiny_obj_loader.h>

#include "mesh_builder.hpp"

struct FullVertex
{
    float3 position;
    float3 normal;
    float2 uv;
    float4 color;
};

static void ConvertVertices(void* dst, const FullVertex* src, int count, const VertexDescriptor& descriptor)
{
    unsigned char* dstBuffer = (unsigned char*)dst;
    for (int i = 0; i < count; ++i)
    {
        const FullVertex* srcVertex = src + i;
        unsigned char* vertexStart = dstBuffer + (i * descriptor.size);

        float3* dstPos = (float3*)(vertexStart + descriptor.positionOffset);
        *dstPos = srcVertex->position;

        if (descriptor.hasNormal)
        {
            float3* dstNormal = (float3*)(vertexStart + descriptor.normalOffset);
            *dstNormal = srcVertex->normal;
        }

        if (descriptor.hasUV)
        {
            float2* dstUV = (float2*)(vertexStart + descriptor.uvOffset);
            *dstUV = srcVertex->uv;
        }

        if (descriptor.hasColor)
        {
            float4* dstColor = (float4*)(vertexStart + descriptor.colorOffset);
            *dstColor = srcVertex->color;
        }
    }
}

MeshBuilder::MeshBuilder(const VertexDescriptor& descriptor, void** verticesPtr, int* vertexCount)
    : descriptor(descriptor)
    , verticesPtr(verticesPtr)
    , vertexCount(vertexCount)
{
}

void* MeshBuilder::GetDst(int* startIndex, int count)
{
    void* dst;

    // TODO: Check space is sufficient if startIndex is set
    if (startIndex)
        dst = (unsigned char*)(*verticesPtr) + *startIndex * descriptor.size;
    else
        dst = Grow(count);

    return dst;
}

void* MeshBuilder::Grow(int count)
{
    int oldCount = *vertexCount;
    *vertexCount += count;
    void* newPtr = realloc(*verticesPtr, *vertexCount * descriptor.size);
    *verticesPtr = newPtr;

    return (unsigned char*)*verticesPtr + (oldCount * descriptor.size);
}

MeshSlice MeshBuilder::GenTriangle(int* startIndex)
{
    FullVertex vertices[] =
    {
        // position             normal             uv              color
        { { 0.5f,-0.5f, 0.f }, { 0.f, 0.f, 1.f }, { 1.0f, 0.0f }, { 1.f, 0.f, 0.f, 1.f } },
        { {-0.5f,-0.5f, 0.f }, { 0.f, 0.f, 1.f }, { 0.0f, 0.0f }, { 0.f, 1.f, 0.f, 1.f } },
        { { 0.0f, 0.5f, 0.f }, { 0.f, 0.f, 1.f }, { 0.5f, 1.0f }, { 0.f, 0.f, 1.f, 1.f } },
    };

    int count = ARRAYSIZE(vertices);

    ConvertVertices(GetDst(startIndex, count), vertices, count, descriptor);

    return { *vertexCount - count, count };
}

MeshSlice MeshBuilder::GenQuad(int* startIndex, float halfWidth, float halfHeight)
{
    FullVertex vertices[] =
    {
        // position                        normal             uv            color
        { { halfWidth,-halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
        { {-halfWidth,-halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
        { { halfWidth, halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f } },

        { { halfWidth, halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 1.f }, { 1.f, 1.f, 1.f, 1.f } },
        { {-halfWidth, halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 1.f }, { 1.f, 1.f, 1.f, 1.f } },
        { {-halfWidth,-halfHeight, 0.f }, { 0.f, 0.f, 1.f }, { 0.f, 0.f }, { 1.f, 1.f, 1.f, 1.f } },
    };
    int count = ARRAYSIZE(vertices);

    ConvertVertices(GetDst(startIndex, count), vertices, count, descriptor);

    return { *vertexCount - count, count };
}

// Implement dumb caching to avoid parsing .obj again and again
static bool LoadObjFromCache(std::vector<FullVertex>& mesh, const char* filename)
{
    std::string cachedFile = filename;
    cachedFile += ".cache";

    FILE* file = fopen(cachedFile.c_str(), "rb");
    if (file == nullptr)
        return false;

    size_t vertexCount = 0;
    fread(&vertexCount, sizeof(size_t), 1, file);
    mesh.resize(vertexCount);
    fread(mesh.data(), sizeof(FullVertex), vertexCount, file);
    fclose(file);

    printf("Loaded from cache: %s (%d vertices)\n", filename, (int)vertexCount);

    return true;
}

static void SaveObjToCache(const std::vector<FullVertex>& mesh, const char* filename)
{
    std::string cachedFile = filename;
    cachedFile += ".cache";

    FILE* file = fopen(cachedFile.c_str(), "wb");
    size_t vertexCount = mesh.size();
    fwrite(&vertexCount, sizeof(size_t), 1, file);
    fwrite(&mesh[0], sizeof(FullVertex), vertexCount, file);
    fclose(file);

    printf("Saved to cache: %s (%d vertices)\n", filename, (int)vertexCount);
}

MeshSlice MeshBuilder::LoadObj(int* startIndex, const char* objFile, const char* mtlDir, float scale)
{
    std::vector<FullVertex> vertices;
    if (!LoadObjFromCache(vertices, objFile))
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warning;
        std::string error;

        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warning, &error, objFile, mtlDir, true);
        if (!warning.empty())
            printf("%s\n", warning.c_str());

        if (!error.empty())
            fprintf(stderr, "%s\n", error.c_str());

        if (!ret)
            return { 0, 0 };

        // TODO: Precompute total vertex count to prealloc

        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++)
        {
            // Loop over faces(polygon)
            size_t index_offset = 0;
            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
            {
                int fv = shapes[s].mesh.num_face_vertices[f];

                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++)
                {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                    FullVertex vert = {};

                    vert.position.x = attrib.vertices[3 * idx.vertex_index + 0] * scale;
                    vert.position.y = attrib.vertices[3 * idx.vertex_index + 1] * scale;
                    vert.position.z = attrib.vertices[3 * idx.vertex_index + 2] * scale;

                    if (!attrib.normals.empty())
                    {
                        vert.normal.x = attrib.normals[3 * idx.normal_index + 0];
                        vert.normal.y = attrib.normals[3 * idx.normal_index + 1];
                        vert.normal.z = attrib.normals[3 * idx.normal_index + 2];
                    }

                    if (!attrib.texcoords.empty())
                    {
                        vert.uv.u = attrib.texcoords[2 * idx.texcoord_index + 0];
                        vert.uv.v = attrib.texcoords[2 * idx.texcoord_index + 1];
                    }

                    // Optional: vertex colors
                    if (!attrib.colors.empty())
                    {
                        vert.color.r = attrib.colors[3 * idx.vertex_index + 0];
                        vert.color.g = attrib.colors[3 * idx.vertex_index + 1];
                        vert.color.b = attrib.colors[3 * idx.vertex_index + 2];
                    }

                    vertices.push_back(vert);
                }
                index_offset += fv;
            }
        }

        SaveObjToCache(vertices, objFile);
    }

    int count = (int)vertices.size();
    ConvertVertices(GetDst(startIndex, count), vertices.data(), count, descriptor);

    return { *vertexCount - count, count };
}
