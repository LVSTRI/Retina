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

struct SRayPayload {
    bool Miss;
    float RayT;
    vec3 Normal;
    vec3 Color;
};

layout (location = 0) rayPayloadInEXT SRayPayload p_rayPayload;

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
    const vec3 vertexNormal = normalize(
        vertices[0].Normal * barycentrics.x +
        vertices[1].Normal * barycentrics.y +
        vertices[2].Normal * barycentrics.z
    );
    const vec2 vertexUv =
        vertices[0].Uv * barycentrics.x +
        vertices[1].Uv * barycentrics.y +
        vertices[2].Uv * barycentrics.z;
    const vec4 vertexTangent =
        vertices[0].Tangent * barycentrics.x +
        vertices[1].Tangent * barycentrics.y +
        vertices[2].Tangent * barycentrics.z;
    vec3 color = vec3(1.0);
    vec3 normal = vertexNormal;
    if (objectInfo.MaterialIndex != uint(-1)) {
        const SMaterial material = g_materialBuffer[objectInfo.MaterialIndex];
        color *= material.BaseColor;
        if (material.BaseColorTexture != uint(-1)) {
            color *= textureLod(GetTexture(material.BaseColorTexture), vertexUv, 0).rgb;
        }
        if (material.NormalTexture != uint(-1)) {
            vec3 sampledNormal = vec3(textureLod(GetTexture(material.NormalTexture), vertexUv, 0).rg * 2.0 - 1.0, 1.0);
            // reconstruct z
            sampledNormal.z = sqrt(max(1.0 - dot(sampledNormal.xy, sampledNormal.xy), 0.0));
            const vec3 bitangent = cross(vertexNormal, vertexTangent.xyz) * vertexTangent.w;
            const vec3 T = normalize(mat3(gl_ObjectToWorldEXT) * vertexTangent.xyz);
            const vec3 B = normalize(mat3(gl_ObjectToWorldEXT) * bitangent);
            const vec3 N = normalize(mat3(gl_ObjectToWorldEXT) * vertexNormal);
            const mat3 TBN = mat3(T, B, N);
            normal = normalize(TBN * sampledNormal);
        }
    }
    p_rayPayload.Miss = false;
    p_rayPayload.RayT = gl_HitTEXT;
    p_rayPayload.Normal = normal;
    p_rayPayload.Color = color;
}
