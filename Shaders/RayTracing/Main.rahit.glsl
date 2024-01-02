#version 460
#extension GL_EXT_ray_tracing_position_fetch : require
#include <RayTracing/Common.glsl>

struct SVertex {
    vec3 Normal;
    vec2 Uv;
    vec4 Tangent;
};

struct SObjectInfo {
    uint VertexOffset;
    uint IndexOffset;
    uint MaterialIndex;
};

struct SMaterial {
    vec3 BaseColor;
    vec3 EmissionColor;
    float EmissionStrength;

    uint BaseColorTexture;
    uint NormalTexture;
};

layout (push_constant) uniform UPushConstant {
    uint u_frameIndex;
    uint u_imageId;
    uint u_viewInfoBufferId;
    uint u_vertexBufferId;
    uint u_indexBufferId;
    uint u_objectInfoBufferId;
    uint u_materialBufferId;
    uint u_samplerId;
    uint u_accelerationStructureId;
};

RetinaDeclareSampledImage(texture2D, UTexture2DBlock);

RetinaDeclareScalarStorageBuffer(restrict readonly, BVertexBuffer, {
    SVertex[] Vertices;
});
RetinaDeclareStorageBuffer(restrict readonly, BIndexBuffer, {
    uint[] Indices;
});
RetinaDeclareStorageBuffer(restrict readonly, BObjectInfoBuffer, {
    SObjectInfo[] ObjectInfos;
});
RetinaDeclareScalarStorageBuffer(restrict readonly, BMaterialBuffer, {
    SMaterial[] Materials;
});

#define g_vertexBuffer RetinaGetStorageBufferMember(BVertexBuffer, Vertices, u_vertexBufferId)
#define g_indexBuffer RetinaGetStorageBufferMember(BIndexBuffer, Indices, u_indexBufferId)
#define g_objectInfoBuffer RetinaGetStorageBufferMember(BObjectInfoBuffer, ObjectInfos, u_objectInfoBufferId)
#define g_materialBuffer RetinaGetStorageBufferMember(BMaterialBuffer, Materials, u_materialBufferId)
#define g_mainSampler RetinaGetSampler(u_samplerId)

#define GetTexture(id) sampler2D(RetinaGetSampledImage(UTexture2DBlock, id), g_mainSampler)

hitAttributeEXT vec2 g_barycentrics;

void main() {
    const vec3 barycentrics = vec3(1.0 - g_barycentrics.x - g_barycentrics.y, g_barycentrics.x, g_barycentrics.y);
    const SObjectInfo objectInfo = g_objectInfoBuffer[gl_InstanceCustomIndexEXT];
    const uvec3 indices = uvec3(
        g_indexBuffer[objectInfo.IndexOffset + gl_PrimitiveID * 3 + 0],
        g_indexBuffer[objectInfo.IndexOffset + gl_PrimitiveID * 3 + 1],
        g_indexBuffer[objectInfo.IndexOffset + gl_PrimitiveID * 3 + 2]
    );
    const SVertex[] vertices = SVertex[](
        g_vertexBuffer[objectInfo.VertexOffset + indices.x],
        g_vertexBuffer[objectInfo.VertexOffset + indices.y],
        g_vertexBuffer[objectInfo.VertexOffset + indices.z]
    );
    const vec2 vertexUv =
        vertices[0].Uv * barycentrics.x +
        vertices[1].Uv * barycentrics.y +
        vertices[2].Uv * barycentrics.z;
    if (objectInfo.MaterialIndex != uint(-1)) {
        const SMaterial material = g_materialBuffer[objectInfo.MaterialIndex];
        if (material.BaseColorTexture != uint(-1)) {
            const float alpha = textureLod(GetTexture(material.BaseColorTexture), vertexUv, 0.0).a;
            if (alpha < 0.8) {
                ignoreIntersectionEXT;
            }
        }
    }
}
