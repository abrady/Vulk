#include "Vulk/VulkPCH.h"

// TODO: get subdivideTris working with our geo creation functions and maybe we can accumulate the vertices and indices in the meshData
#define CHECK_MESH_DATA(meshData) assert(meshData.vertices.size() == 0 && meshData.indices.size() == 0)

using namespace glm;

// Calculate the tangent of a triangle - in this case convention is to use:
// Imagine a triangle on a plane with a tangent and bitangent vector on the plane.
//
// 2. E1=ΔU1T+ΔV1B
// see also https://learnopengl.com/Advanced-Lighting/Normal-Mapping
static vec3 calcTangent(vec3 pos1, vec3 pos2, vec3 pos3, vec2 uv1, vec2 uv2, vec2 uv3) {
    // positions
    // pos1 = vec3(-1.0, 1.0, 0.0);
    // pos2 = vec3(-1.0, -1.0, 0.0);
    // pos3 = vec3(1.0, -1.0, 0.0);
    // glm::vec3 pos4( 1.0,  1.0, 0.0);
    // // texture coordinates
    // uv1 = vec2(0.0, 1.0);
    // uv2 = vec2(0.0, 0.0);
    // uv3 = vec2(1.0, 0.0);
    // glm::vec2 uv4(1.0, 1.0);
    // normal vector
    // glm::vec3 nm(0.0, 0.0, 1.0);

    glm::vec3 edge1 = pos2 - pos1;
    glm::vec3 edge2 = pos3 - pos1;
    glm::vec2 deltaUV1 = uv2 - uv1;
    glm::vec2 deltaUV2 = uv3 - uv1;

    float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    glm::vec3 tangent1;
    tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    // bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    // bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    // bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);
    return tangent1;
}

static void calcMeshTangents(VulkMesh& meshData) {
    for (auto& vert : meshData.vertices) {
        vert.tangent = vec3(0.f); // normalize to get the 'average' tangent
    }
    // tangent space needs special handling
    for (uint32_t i = 0; i < meshData.indices.size(); i += 3) {
        uint32_t i0 = meshData.indices[i + 0];
        uint32_t i1 = meshData.indices[i + 1];
        uint32_t i2 = meshData.indices[i + 2];
        vec3 P0 = meshData.vertices[i0].pos;
        vec3 P1 = meshData.vertices[i1].pos;
        vec3 P2 = meshData.vertices[i2].pos;
        vec2 UV0 = meshData.vertices[i0].uv;
        vec2 UV1 = meshData.vertices[i1].uv;
        vec2 UV2 = meshData.vertices[i2].uv;
        vec3 tangent = calcTangent(P0, P1, P2, UV0, UV1, UV2);
        meshData.vertices[i0].tangent += tangent;
        meshData.vertices[i1].tangent += tangent;
        meshData.vertices[i2].tangent += tangent;
    }

    for (auto& vert : meshData.vertices) {
        vert.tangent = normalize(vert.tangent); // normalize to get the 'average' tangent
    }
}

// Turn a single triangle into 4 triangles by adding 3 new vertices at the midpoints of each edge
// Original Triangle:
//      /\
//     /  \
//    /    \
//   /______\
//
// Subdivision Step:
//      /\
//     /__\
//    /\  /\
//   /__\/__\
//
static void subdivideTris(VulkMesh& meshData) {
    // save a copy of the input geometry
    std::vector<Vertex> verticesCopy = meshData.vertices;
    std::vector<uint32_t> indicesCopy = meshData.indices;
    meshData.vertices.clear();
    meshData.indices.clear();
    meshData.vertices.reserve(verticesCopy.size() * 2);
    meshData.indices.reserve(indicesCopy.size() * 4);

    std::unordered_map<glm::vec3, uint32_t> indexFromPoint;
    auto addVertex = [&](Vertex const& v) -> uint32_t {
        auto it = indexFromPoint.find(v.pos);
        if (it != indexFromPoint.end()) {
            return it->second;
        } else {
            uint32_t index = (uint32_t)meshData.vertices.size();
            meshData.vertices.push_back(v);
            indexFromPoint[v.pos] = index;
            return index;
        }
    };

    for (uint32_t i = 0; i < indicesCopy.size(); i += 3) {
        Vertex v0 = verticesCopy[indicesCopy[i + 0]];
        Vertex v1 = verticesCopy[indicesCopy[i + 1]];
        Vertex v2 = verticesCopy[indicesCopy[i + 2]];

        // generate 3 new vertices at the midpoints of each edge
        Vertex m0, m1, m2;
        m0.pos = 0.5f * (v0.pos + v1.pos);
        m0.uv = 0.5f * (v0.uv + v1.uv);
        m0.normal = 0.5f * (v0.normal + v1.normal);
        m1.pos = 0.5f * (v1.pos + v2.pos);
        m1.uv = 0.5f * (v1.uv + v2.uv);
        m1.normal = 0.5f * (v1.normal + v2.normal);
        m2.pos = 0.5f * (v0.pos + v2.pos);
        m2.uv = 0.5f * (v0.uv + v2.uv);
        m2.normal = 0.5f * (v0.normal + v2.normal);

        // add new geometry
        // meshData.vertices.push_back(v0);
        // meshData.vertices.push_back(m0);
        // meshData.vertices.push_back(v1);
        // meshData.vertices.push_back(m1);
        // meshData.vertices.push_back(v2);
        // meshData.vertices.push_back(m2);
        uint32_t i0 = addVertex(v0);
        uint32_t i1 = addVertex(m0);
        uint32_t i2 = addVertex(v1);
        uint32_t i3 = addVertex(m1);
        uint32_t i4 = addVertex(v2);
        uint32_t i5 = addVertex(m2);

        // add new indices
        meshData.indices.push_back(i0);
        meshData.indices.push_back(i1);
        meshData.indices.push_back(i5);

        meshData.indices.push_back(i1);
        meshData.indices.push_back(i2);
        meshData.indices.push_back(i3);

        meshData.indices.push_back(i1);
        meshData.indices.push_back(i3);
        meshData.indices.push_back(i5);

        meshData.indices.push_back(i3);
        meshData.indices.push_back(i4);
        meshData.indices.push_back(i5);
    }
}

void makeEquilateralTri(float side, uint32_t numSubdivisions, VulkMesh& meshData) {
    CHECK_MESH_DATA(meshData);
    meshData.name = "EquilateralTriangle";
    Vertex v0;
    v0.pos = vec3(0.0f, 0.0f, 0.0f);
    v0.normal = vec3(0.0f, 0.0f, 1.0f);
    v0.uv = vec2(0.5f, 0.0f);

    Vertex v1;
    v1.pos = vec3(side, 0.0f, 0.0f);
    v1.normal = vec3(0.0f, 0.0f, 1.0f);
    v1.uv = vec2(0.5f, 1.0f);

    Vertex v2;
    v2.pos = vec3(side / 2.0f, side * sqrtf(3.0f) / 2.0f, 0.0f);
    v2.normal = vec3(0.0f, 0.0f, 1.0f);
    v2.uv = vec2(0.0f, 1.0f);

    uint32_t baseIndex = (uint32_t)meshData.vertices.size();
    meshData.vertices.push_back(v0);
    meshData.vertices.push_back(v1);
    meshData.vertices.push_back(v2);

    meshData.indices.push_back(baseIndex + 0);
    meshData.indices.push_back(baseIndex + 1);
    meshData.indices.push_back(baseIndex + 2);

    assert(numSubdivisions <= 6u);
    numSubdivisions = glm::min(numSubdivisions, 6u);
    for (uint32_t i = 0; i < numSubdivisions; ++i) {
        subdivideTris(meshData);
    }

    calcMeshTangents(meshData);
}

void makeQuad(float x, float y, float w, float h, float depth, uint32_t numSubdivisions, VulkMesh& meshData) {
    CHECK_MESH_DATA(meshData);
    meshData.name = "Quad";

    Vertex v0;
    v0.pos = vec3(x, y, depth);
    v0.normal = vec3(0.0f, 0.0f, 1.0f);
    v0.uv = vec2(0.0f, 0.0f);

    Vertex v1;
    v1.pos = vec3(x + w, y, depth);
    v1.normal = vec3(0.0f, 0.0f, 1.0f);
    v1.uv = vec2(1.0f, 0.0f);

    Vertex v2;
    v2.pos = vec3(x + w, y + h, depth);
    v2.normal = vec3(0.0f, 0.0f, 1.0f);
    v2.uv = vec2(1.0f, 1.0f);

    Vertex v3;
    v3.pos = vec3(x, y + h, depth);
    v3.normal = vec3(0.0f, 0.0f, 1.0f);
    v3.uv = vec2(0.0f, 1.0f);

    uint32_t baseIndex = (uint32_t)meshData.vertices.size();
    meshData.vertices.push_back(v0);
    meshData.vertices.push_back(v1);
    meshData.vertices.push_back(v2);
    meshData.vertices.push_back(v3);

    meshData.indices.push_back(baseIndex + 0);
    meshData.indices.push_back(baseIndex + 1);
    meshData.indices.push_back(baseIndex + 2);
    meshData.indices.push_back(baseIndex + 0);
    meshData.indices.push_back(baseIndex + 2);
    meshData.indices.push_back(baseIndex + 3);

    assert(numSubdivisions <= 6u);
    numSubdivisions = glm::min(numSubdivisions, 6u);
    for (uint32_t i = 0; i < numSubdivisions; ++i) {
        subdivideTris(meshData);
    }
    calcMeshTangents(meshData);
}

void makeQuad(float w, float h, uint32_t numSubdivisions, VulkMesh& meshData) {
    makeQuad(-w / 2.0f, -h / 2.0f, w, h, 0.0f, numSubdivisions, meshData);
}

// make a cylinder of height `height` with radius `radius`, `numStacks` and `numSlices` slices (stacks are vertical, slices are horizontal - think pizza slices)
// cenetered at the origin
void makeCylinder(float height, float bottomRadius, float topRadius, uint32_t numStacksIn, uint32_t numSlices, VulkMesh& meshData) {
    CHECK_MESH_DATA(meshData);
    meshData.name = "Cylinder";

    uint32_t baseIndex;
    float numStacks = (float)numStacksIn;

    float stackHeight = height / numStacks;
    float radiusStep = (topRadius - bottomRadius) / numStacks;
    uint32_t ringCount = numStacksIn + 1;
    baseIndex = (uint32_t)meshData.vertices.size();
    for (uint32_t i = 0; i < ringCount; ++i) {
        float y = -0.5f * height + (float)i * stackHeight;
        float r = bottomRadius + (float)i * radiusStep;
        float dTheta = 2.0f * pi<float>() / (float)numSlices;
        for (uint32_t j = 0; j <= numSlices; ++j) {
            Vertex vertex;
            float c = cosf((float)j * dTheta);
            float s = sinf((float)j * dTheta);
            vertex.pos = vec3(r * c, y, r * s);
            vertex.uv = vec2((float)j / (float)numSlices, 1.0f - (float)i / (float)numStacks);
            vertex.normal = vec3(c, 0.0f, s);
            meshData.vertices.push_back(vertex);
        }
    }
    uint32_t ringVertexCount = numSlices + 1;
    for (uint32_t i = 0; i < numStacksIn; ++i) {
        for (uint32_t j = 0; j < numSlices; ++j) {
            meshData.indices.push_back(baseIndex + i * ringVertexCount + j);
            meshData.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j);
            meshData.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
            meshData.indices.push_back(baseIndex + i * ringVertexCount + j);
            meshData.indices.push_back(baseIndex + (i + 1) * ringVertexCount + j + 1);
            meshData.indices.push_back(baseIndex + i * ringVertexCount + j + 1);
        }
    }

    float const dTheta = 2.0f * pi<float>() / (float)numSlices;
    uint32_t centerIndex;

    // // make top
    baseIndex = (uint32_t)meshData.vertices.size();
    for (uint32_t i = 0; i <= numSlices; ++i) {
        Vertex vertex;
        float c = cosf((float)i * dTheta);
        float s = sinf((float)i * dTheta);
        float x = topRadius * c;
        float z = topRadius * s;
        float u = x / height + 0.5f;
        float v = z / height + 0.5f;
        vertex.pos = vec3(x, 0.5f * height, z);
        vertex.normal = vec3(0.0f, 1.0f, 0.0f);
        // vertex.tangent = vec3(-s, 0.0f, c);
        vertex.uv = vec2(u, v);
        meshData.vertices.push_back(vertex);
    }
    Vertex topCenter;
    topCenter.pos = vec3(0.0f, 0.5f * height, 0.0f);
    topCenter.uv = vec2(0.5f, 0.5f);
    topCenter.normal = vec3(0.0f, -1.0f, 0.0f);
    // topCenter.tangent = vec3(1.0f, 0.0f, 0.0f); // no idea on tangent for the topCenter point
    meshData.vertices.push_back(topCenter);
    centerIndex = (uint32_t)meshData.vertices.size() - 1;

    for (uint32_t i = 0; i < numSlices; ++i) {
        meshData.indices.push_back(baseIndex + i + 1);
        meshData.indices.push_back(baseIndex + i);
        meshData.indices.push_back(centerIndex);
    }

    // make bottom
    baseIndex = (uint32_t)meshData.vertices.size();
    for (uint32_t i = 0; i <= numSlices; ++i) {
        Vertex vertex;
        float c = cosf((float)i * dTheta);
        float s = sinf((float)i * dTheta);
        float x = bottomRadius * c;
        float z = bottomRadius * s;
        float u = x / height + 0.5f;
        float v = z / height + 0.5f;

        vertex.pos = vec3(x, -0.5f * height, z);
        vertex.normal = vec3(0.0f, -1.0f, 0.0f);
        // vertex.tangent = vec3(-s, 0.0f, c);
        vertex.uv = vec2(u, v);
        meshData.vertices.push_back(vertex);
    }
    Vertex bottomCenter;
    bottomCenter.pos = vec3(0.0f, -0.5f * height, 0.0f);
    bottomCenter.uv = vec2(0.5f, 0.5f);
    bottomCenter.normal = vec3(0.0f, -1.0f, 0.0f);
    // bottomCenter.tangent = vec3(1.0f, 0.0f, 0.0f); // no idea on tangent for the bottomCenter point
    meshData.vertices.push_back(bottomCenter);
    centerIndex = (uint32_t)meshData.vertices.size() - 1;

    for (uint32_t i = 0; i < numSlices; ++i) {
        meshData.indices.push_back(baseIndex + i);
        meshData.indices.push_back(baseIndex + i + 1);
        meshData.indices.push_back(centerIndex);
    }
    calcMeshTangents(meshData);
}

void makeGeoSphere(float radius, uint32_t numSubdivisions, VulkMesh& meshData) {
    CHECK_MESH_DATA(meshData);
    meshData.name = "GeoSphere";

    // put a cap on the number of subdivisions
    numSubdivisions = glm::min(numSubdivisions, 6u);

    // put a cap on the number of subdivisions
    const float x = 0.525731f;
    const float z = 0.850651f;

    vec3 pos[12] = {vec3(-x, 0.0f, z), vec3(x, 0.0f, z),   vec3(-x, 0.0f, -z), vec3(x, 0.0f, -z), vec3(0.0f, z, x),  vec3(0.0f, z, -x),
                    vec3(0.0f, -z, x), vec3(0.0f, -z, -x), vec3(z, x, 0.0f),   vec3(-z, x, 0.0f), vec3(z, -x, 0.0f), vec3(-z, -x, 0.0f)};

    uint32_t k[60] = {1, 4,  0, 4,  9, 0, 4, 5,  9, 8, 5, 4,  1, 8, 4, 1,  10, 8, 10, 3, 8, 8, 3,  5, 3, 2, 5, 3,  7, 2,
                      3, 10, 7, 10, 6, 7, 6, 11, 7, 6, 0, 11, 6, 1, 0, 10, 1,  6, 11, 0, 9, 2, 11, 9, 5, 2, 9, 11, 2, 7};

    uint32_t baseIndex = (uint32_t)meshData.vertices.size();
    meshData.vertices.resize(meshData.vertices.size() + 12);
    meshData.indices.resize(meshData.indices.size() + 60);
    for (uint32_t i = 0; i < 60; ++i) {
        meshData.indices[i] = baseIndex + k[i];
    }

    for (uint32_t i = 0; i < 12; ++i) {
        meshData.vertices[i].pos = pos[i];
    }

    for (uint32_t i = 0; i < numSubdivisions; ++i) {
        subdivideTris(meshData);
    }

    // project vertices onto sphere and scale
    for (uint32_t i = 0; i < meshData.vertices.size(); ++i) {
        vec3 n = normalize(meshData.vertices[i].pos);
        vec3 p = radius * n;
        meshData.vertices[i].pos = p;
        meshData.vertices[i].normal = n;
        meshData.vertices[i].uv.x = atan2f(n.z, n.x) / (2.0f * pi<float>()) + 0.5f;
        meshData.vertices[i].uv.y = asinf(n.y) / pi<float>() + 0.5f;
    }
    calcMeshTangents(meshData);
}

void makeAxes(float length, VulkMesh& meshData) {
    CHECK_MESH_DATA(meshData);
    meshData.name = "Axes";
    VulkMesh x, y, z;
    makeCylinder(length, 0.01f, 0.01f, 10, 10, x);
    makeCylinder(length, 0.01f, 0.01f, 10, 10, y);
    makeCylinder(length, 0.01f, 0.01f, 10, 10, z);

    mat4 rotX = rotate(mat4(1.0f), pi<float>() / 2.0f, vec3(0.0f, 0.0f, 1.0f));
    mat4 rotAndTranslateX = translate(mat4(1.0f), vec3(length / 2.0f, 0.0f, 0.0f)) * rotX;
    x.xform(rotAndTranslateX);

    mat4 transY = translate(mat4(1.0f), vec3(0.0f, length / 2.0f, 0.0f));
    y.xform(transY);

    mat4 rotZ = rotate(mat4(1.0f), pi<float>() / 2.0f, vec3(1.0f, 0.0f, 0.0f));
    mat4 rotAndTranslateZ = translate(mat4(1.0f), vec3(0.0f, 0.0f, length / 2.0f)) * rotZ;
    z.xform(rotAndTranslateZ);

    meshData.appendMesh(x);
    meshData.appendMesh(y);
    meshData.appendMesh(z);
    calcMeshTangents(meshData);
}

#ifdef __clang__
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#endif

void makeGrid(float width, float depth, uint32_t m, uint32_t n, VulkMesh& meshData, float repeatU, float repeatV) {
    uint32_t vertexCount = m * n;
    uint32_t faceCount = (m - 1) * (n - 1) * 2;

    // Create the vertices
    float halfWidth = 0.5f * width;
    float halfDepth = 0.5f * depth;

    float dx = width / (n - 1);
    float dz = depth / (m - 1);

    float du = repeatU / (n - 1);
    float dv = repeatV / (m - 1);

    meshData.vertices.resize(vertexCount);
    for (uint32_t i = 0; i < m; ++i) {
        float z = halfDepth - i * dz;
        for (uint32_t j = 0; j < n; ++j) {
            float x = -halfWidth + j * dx;

            meshData.vertices[i * n + j].pos = vec3(x, 0.0f, z);
            meshData.vertices[i * n + j].normal = vec3(0.0f, 1.0f, 0.0f);
            // meshData.vertices[i * n + j].tangent = vec3(1.0f, 0.0f, 0.0f);
            meshData.vertices[i * n + j].uv = vec2(j * du, i * dv);
        }
    }

    // Create the indices.
    meshData.indices.resize(faceCount * 3); // 3 indices per face

    // Iterate over each quad and compute indices.
    uint32_t k = 0;
    for (uint32_t i = 0; i < m - 1; ++i) {
        for (uint32_t j = 0; j < n - 1; ++j) {
            meshData.indices[k] = i * n + j;
            meshData.indices[k + 1] = i * n + j + 1;
            meshData.indices[k + 2] = (i + 1) * n + j;

            meshData.indices[k + 3] = (i + 1) * n + j;
            meshData.indices[k + 4] = i * n + j + 1;
            meshData.indices[k + 5] = (i + 1) * n + j + 1;

            k += 6; // next quad
        }
    }

    calcMeshTangents(meshData);
}