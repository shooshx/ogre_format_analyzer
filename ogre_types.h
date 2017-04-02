#pragma once
#include "Except.h"
#include <map>
#include <set>

typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned char  ubyte;

// See OgreHardwareVertexBuffer.h

enum VertexElementType
{
    VET_FLOAT1 = 0,
    VET_FLOAT2 = 1,
    VET_FLOAT3 = 2,
    VET_FLOAT4 = 3,
    /// alias to more specific colour type - use the current rendersystem's colour packing
            VET_COLOUR = 4,
    VET_SHORT1 = 5,
    VET_SHORT2 = 6,
    VET_SHORT3 = 7,
    VET_SHORT4 = 8,
    VET_UBYTE4 = 9,
    /// D3D style compact colour
            VET_COLOUR_ARGB = 10,
    /// GL style compact colour
            VET_COLOUR_ABGR = 11
};

inline const char* typeName(ushort type) {
    switch(type) {
    case VET_FLOAT1: return "VET_FLOAT1";
    case VET_FLOAT2: return "VET_FLOAT2";
    case VET_FLOAT3: return "VET_FLOAT3";
    case VET_FLOAT4: return "VET_FLOAT4";
    case VET_COLOUR: return "VET_COLOUR";
    case VET_SHORT1: return "VET_SHORT1";
    case VET_SHORT2: return "VET_SHORT2";
    case VET_SHORT3: return "VET_SHORT3";
    case VET_SHORT4: return "VET_SHORT4";
    case VET_UBYTE4: return "VET_UBYTE4";
    case VET_COLOUR_ARGB: return "VET_COLOUR_ARGB";
    case VET_COLOUR_ABGR: return "VET_COLOUR_ABGR";
    }
    return "[unknown-type]";
}

inline int typeSize(ushort type) { // in bytes
    switch(type) {
    case VET_FLOAT1:
    case VET_FLOAT2:
    case VET_FLOAT3:
    case VET_FLOAT4:
        return (int)sizeof(float) * (type - VET_FLOAT1 + 1);
    case VET_COLOUR:
        throw Exception("Unknow type size");
    case VET_SHORT1:
    case VET_SHORT2:
    case VET_SHORT3:
    case VET_SHORT4:
    case VET_UBYTE4:
        return (int)sizeof(short) * (type - VET_SHORT1 + 1);
    case VET_COLOUR_ARGB:
    case VET_COLOUR_ABGR:
        return 4;
    }
    throw Exception("unknown type with unknown size");
}

/// Vertex element semantics, used to identify the meaning of vertex buffer contents
enum VertexElementSemantic {
/// Position, 3 reals per vertex
            VES_POSITION = 1,
/// Blending weights
            VES_BLEND_WEIGHTS = 2,
/// Blending indices
            VES_BLEND_INDICES = 3,
/// Normal, 3 reals per vertex
            VES_NORMAL = 4,
/// Diffuse colours
            VES_DIFFUSE = 5,
/// Specular colours
            VES_SPECULAR = 6,
/// Texture coordinates
            VES_TEXTURE_COORDINATES = 7,
/// Binormal (Y axis if normal is Z)
            VES_BINORMAL = 8,
/// Tangent (X axis if normal is Z)
            VES_TANGENT = 9,
/// The  number of VertexElementSemantic elements (note - the first value VES_POSITION is 1)
            VES_COUNT = 9
};


inline const char* semanticName(ushort s) {
    switch(s) {
    case VES_POSITION: return "VES_POSITION";
    case VES_BLEND_WEIGHTS: return "VES_BLEND_WEIGHTS";
    case VES_BLEND_INDICES: return "VES_BLEND_INDICES";
    case VES_NORMAL: return "VES_NORMAL";
    case VES_DIFFUSE: return "VES_DIFFUSE";
    case VES_SPECULAR: return "VES_SPECULAR";
    case VES_TEXTURE_COORDINATES: return "VES_TEXTURE_COORDINATES";
    case VES_BINORMAL: return "VES_BINORMAL";
    case VES_TANGENT: return "VES_TANGENT";
    }
    return "[unknown-semantic]";
}

// from OgreRenderOperation.h
enum OperationType {
    /// A list of points, 1 vertex per point
            OT_POINT_LIST = 1,
    /// A list of lines, 2 vertices per line
            OT_LINE_LIST = 2,
    /// A strip of connected lines, 1 vertex per line plus 1 start vertex
            OT_LINE_STRIP = 3,
    /// A list of triangles, 3 vertices per triangle
            OT_TRIANGLE_LIST = 4,
    /// A strip of triangles, 3 vertices for the first triangle, and 1 per triangle after that
            OT_TRIANGLE_STRIP = 5,
    /// A fan of triangles, 3 vertices for the first triangle, and 1 per triangle after that
            OT_TRIANGLE_FAN = 6
};


// from OgreMeshSerializerImpl.cpp
//      OgreMeshFileFormat.h
enum MeshChunkID {
    M_HEADER                = 0x1000,
        // char*          version           : Version number check
    M_MESH                = 0x3000,
        // bool skeletallyAnimated   // important flag which affects h/w buffer policies
        // Optional M_GEOMETRY chunk
        M_SUBMESH             = 0x4000, 
            // char* materialName
            // bool useSharedVertices
            // unsigned int indexCount
            // bool indexes32Bit
            // unsigned int* faceVertexIndices (indexCount)
            // OR
            // unsigned short* faceVertexIndices (indexCount)
            // M_GEOMETRY chunk (Optional: present only if useSharedVertices = false)
            M_SUBMESH_OPERATION = 0x4010, // optional, trilist assumed if missing
                // unsigned short operationType
            M_SUBMESH_BONE_ASSIGNMENT = 0x4100,
                // Optional bone weights (repeating section)
                // unsigned int vertexIndex;
                // unsigned short boneIndex;
                // float weight;
            // Optional chunk that matches a texture name to an alias
            // a texture alias is sent to the submesh m_material to use this texture name
            // instead of the one in the texture unit with a matching alias name
            M_SUBMESH_TEXTURE_ALIAS = 0x4200, // Repeating section
                // char* aliasName;
                // char* textureName;

        M_GEOMETRY          = 0x5000, // NB this chunk is embedded within M_MESH and M_SUBMESH
            // unsigned int vertexCount
            M_GEOMETRY_VERTEX_DECLARATION = 0x5100,
                M_GEOMETRY_VERTEX_ELEMENT = 0x5110, // Repeating section
                    // unsigned short source;  	// buffer bind source
                    // unsigned short type;    	// VertexElementType
                    // unsigned short semantic; // VertexElementSemantic
                    // unsigned short offset;	// start offset in buffer in bytes
                    // unsigned short index;	// index of the semantic (for colours and texture coords)
            M_GEOMETRY_VERTEX_BUFFER = 0x5200, // Repeating section
                // unsigned short bindIndex;	// Index to bind this buffer to
                // unsigned short m_vertexSize;	// Per-vertex size, must agree with declaration at this index
                M_GEOMETRY_VERTEX_BUFFER_DATA = 0x5210,
                    // raw buffer data
        M_MESH_SKELETON_LINK = 0x6000,
            // Optional link to skeleton
            // char* skeletonName           : name of .skeleton to use
        M_MESH_BONE_ASSIGNMENT = 0x7000,
            // Optional bone weights (repeating section)
            // unsigned int vertexIndex;
            // unsigned short boneIndex;
            // float weight;
        M_MESH_LOD = 0x8000,
            // Optional LOD information
            // string strategyName;
            // unsigned short numLevels;
            // bool manual;  (true for manual alternate meshes, false for generated)
            M_MESH_LOD_USAGE = 0x8100,
            // Repeating section, ordered in increasing depth
            // NB LOD 0 (full detail from 0 depth) is omitted
            // LOD value - this is a distance, a pixel count etc, based on strategy
            // float lodValue;
                M_MESH_LOD_MANUAL = 0x8110,
                // Required if M_MESH_LOD section manual = true
                // String manualMeshName;
                M_MESH_LOD_GENERATED = 0x8120,
                // Required if M_MESH_LOD section manual = false
                // Repeating section (1 per submesh)
                // unsigned int indexCount;
                // bool indexes32Bit
                // unsigned short* faceIndexes;  (indexCount)
                // OR
                // unsigned int* faceIndexes;  (indexCount)
        M_MESH_BOUNDS = 0x9000,
            // float minx, miny, minz
            // float maxx, maxy, maxz
            // float radius
                
        // Added By DrEvil
        // optional chunk that contains a table of submesh indexes and the names of
        // the sub-meshes.
        M_SUBMESH_NAME_TABLE = 0xA000,
            // Subchunks of the name table. Each chunk contains an index & string
            M_SUBMESH_NAME_TABLE_ELEMENT = 0xA100,
                // short index
                // char* name
        
        // Optional chunk which stores precomputed edge data					 
        M_EDGE_LISTS = 0xB000,
            // Each LOD has a separate edge list
            M_EDGE_LIST_LOD = 0xB100,
                // unsigned short lodIndex
                // bool isManual			// If manual, no edge data here, loaded from manual mesh
                    // bool isClosed
                    // unsigned long numTriangles
                    // unsigned long numEdgeGroups
                    // Triangle* triangleList
                        // unsigned long indexSet
                        // unsigned long vertexSet
                        // unsigned long vertIndex[3]
                        // unsigned long sharedVertIndex[3] 
                        // float normal[4] 

                    M_EDGE_GROUP = 0xB110,
                        // unsigned long vertexSet
                        // unsigned long triStart
                        // unsigned long triCount
                        // unsigned long numEdges
                        // Edge* edgeList
                            // unsigned long  triIndex[2]
                            // unsigned long  vertIndex[2]
                            // unsigned long  sharedVertIndex[2]
                            // bool degenerate

        // Optional poses section, referred to by pose keyframes
        M_POSES = 0xC000,
            M_POSE = 0xC100,
                // char* name (may be blank)
                // unsigned short target	// 0 for shared geometry, 
                                            // 1+ for submesh index + 1
                // bool includesNormals [1.8+]
                M_POSE_VERTEX = 0xC111,
                    // unsigned long vertexIndex
                    // float xoffset, yoffset, zoffset
                    // float xnormal, ynormal, znormal (optional, 1.8+)
        // Optional vertex animation chunk
        M_ANIMATIONS = 0xD000, 
            M_ANIMATION = 0xD100,
            // char* name
            // float length
            M_ANIMATION_BASEINFO = 0xD105,
            // [Optional] base keyframe information (pose animation only)
            // char* baseAnimationName (blank for self)
            // float baseKeyFrameTime
    
            M_ANIMATION_TRACK = 0xD110,
                // unsigned short type			// 1 == morph, 2 == pose
                // unsigned short target		// 0 for shared geometry, 
                                                // 1+ for submesh index + 1
                M_ANIMATION_MORPH_KEYFRAME = 0xD111,
                    // float time
                    // bool includesNormals [1.8+]
                    // float x,y,z			// repeat by number of vertices in original geometry
                M_ANIMATION_POSE_KEYFRAME = 0xD112,
                    // float time
                    M_ANIMATION_POSE_REF = 0xD113, // repeat for number of referenced poses
                        // unsigned short poseIndex 
                        // float influence

        // Optional submesh extreme vertex list chink
        M_TABLE_EXTREMES = 0xE000,
        // unsigned short submesh_index;
        // float extremes [n_extremes][3];

/* Version 1.2 of the .mesh format (deprecated)
enum MeshChunkID {
    M_HEADER                = 0x1000,
        // char*          version           : Version number check
    M_MESH                = 0x3000,
        // bool skeletallyAnimated   // important flag which affects h/w buffer policies
        // Optional M_GEOMETRY chunk
        M_SUBMESH             = 0x4000, 
            // char* materialName
            // bool useSharedVertices
            // unsigned int indexCount
            // bool indexes32Bit
            // unsigned int* faceVertexIndices (indexCount)
            // OR
            // unsigned short* faceVertexIndices (indexCount)
            // M_GEOMETRY chunk (Optional: present only if useSharedVertices = false)
            M_SUBMESH_OPERATION = 0x4010, // optional, trilist assumed if missing
                // unsigned short operationType
            M_SUBMESH_BONE_ASSIGNMENT = 0x4100,
                // Optional bone weights (repeating section)
                // unsigned int vertexIndex;
                // unsigned short boneIndex;
                // float weight;
        M_GEOMETRY          = 0x5000, // NB this chunk is embedded within M_MESH and M_SUBMESH
        */
            // unsigned int vertexCount
            // float* pVertices (x, y, z order x numVertices)
            M_GEOMETRY_NORMALS = 0x5100,    //(Optional)
                // float* pNormals (x, y, z order x numVertices)
            M_GEOMETRY_COLOURS = 0x5200,    //(Optional)
                // unsigned long* pColours (RGBA 8888 format x numVertices)
            M_GEOMETRY_TEXCOORDS = 0x5300    //(Optional, REPEATABLE, each one adds an extra set)
                // unsigned short dimensions    (1 for 1D, 2 for 2D, 3 for 3D)
                // float* pTexCoords  (u [v] [w] order, dimensions x numVertices)
        /*
        M_MESH_SKELETON_LINK = 0x6000,
            // Optional link to skeleton
            // char* skeletonName           : name of .skeleton to use
        M_MESH_BONE_ASSIGNMENT = 0x7000,
            // Optional bone weights (repeating section)
            // unsigned int vertexIndex;
            // unsigned short boneIndex;
            // float weight;
        M_MESH_LOD = 0x8000,
            // Optional LOD information
            // unsigned short numLevels;
            // bool manual;  (true for manual alternate meshes, false for generated)
            M_MESH_LOD_USAGE = 0x8100,
            // Repeating section, ordered in increasing depth
            // NB LOD 0 (full detail from 0 depth) is omitted
            // float fromSquaredDepth;
                M_MESH_LOD_MANUAL = 0x8110,
                // Required if M_MESH_LOD section manual = true
                // String manualMeshName;
                M_MESH_LOD_GENERATED = 0x8120,
                // Required if M_MESH_LOD section manual = false
                // Repeating section (1 per submesh)
                // unsigned int indexCount;
                // bool indexes32Bit
                // unsigned short* faceIndexes;  (indexCount)
                // OR
                // unsigned int* faceIndexes;  (indexCount)
        M_MESH_BOUNDS = 0x9000
            // float minx, miny, minz
            // float maxx, maxy, maxz
            // float radius

        // Added By DrEvil
        // optional chunk that contains a table of submesh indexes and the names of
        // the sub-meshes.
        M_SUBMESH_NAME_TABLE,
            // Subchunks of the name table. Each chunk contains an index & string
            M_SUBMESH_NAME_TABLE_ELEMENT,
                // short index
                // char* name

*/
};

enum SkeletonChunkID {
    SKELETON_HEADER            = 0x1000,
    // char* version           : Version number check
            SKELETON_BLENDMODE		   = 0x1010, // optional
    // unsigned short blendmode		: SkeletonAnimationBlendMode

    SKELETON_BONE              = 0x2000,
    // Repeating section defining each bone in the system.
    // Bones are assigned indexes automatically based on their order of declaration
    // starting with 0.

    // char* name                       : name of the bone
    // unsigned short handle            : handle of the bone, should be contiguous & start at 0
    // Vector3 position                 : position of this bone relative to parent
    // Quaternion orientation           : orientation of this bone relative to parent
    // Vector3 scale (optional)         : scale of this bone relative to parent

    SKELETON_BONE_PARENT       = 0x3000,
    // Record of the parent of a single bone, used to build the node tree
    // Repeating section, listed in Bone Index order, one per Bone

    // unsigned short handle             : child bone
    // unsigned short parentHandle   : parent bone

    SKELETON_ANIMATION         = 0x4000,
    // A single animation for this skeleton

    // char* name                       : Name of the animation
    // float length                      : Length of the animation in seconds

    SKELETON_ANIMATION_BASEINFO = 0x4010,
    // [Optional] base keyframe information
    // char* baseAnimationName (blank for self)
    // float baseKeyFrameTime

    SKELETON_ANIMATION_TRACK = 0x4100,
    // A single animation track (relates to a single bone)
    // Repeating section (within SKELETON_ANIMATION)

    // unsigned short boneIndex     : Index of bone to apply to

    SKELETON_ANIMATION_TRACK_KEYFRAME = 0x4110,
    // A single keyframe within the track
    // Repeating section

    // float time                    : The time position (seconds)
    // Quaternion rotate            : Rotation to apply at this keyframe
    // Vector3 translate            : Translation to apply at this keyframe
    // Vector3 scale (optional)               : Scale to apply at this keyframe
            SKELETON_ANIMATION_LINK         = 0x5000
    // Link to another skeleton, to re-use its animations

    // char* skeletonName					: name of skeleton to get animations from
    // float scale							: scale to apply to trans/scale keys

};

typedef bool (*TIsSubCheck)(ushort, ushort);

// defines the hierarchical struction of the mesh file
// which sub chunk is allowed to be in which
// parsing relies on this information to correctly parse the tree structure
inline bool isMeshSubChunk(ushort parent, ushort sub)
{
    static std::map<ushort, std::set<ushort>> p;
    if (p.empty()) {
        p[0] = { M_MESH };
        p[M_MESH] = { M_SUBMESH, M_GEOMETRY, M_MESH_SKELETON_LINK, M_MESH_BONE_ASSIGNMENT, M_MESH_LOD, M_MESH_BOUNDS, M_SUBMESH_NAME_TABLE, M_EDGE_LISTS, M_POSES, M_ANIMATIONS, M_TABLE_EXTREMES };
        p[M_SUBMESH] = { M_GEOMETRY, M_SUBMESH_OPERATION, M_SUBMESH_BONE_ASSIGNMENT, M_SUBMESH_TEXTURE_ALIAS };
        p[M_GEOMETRY] = { M_GEOMETRY_VERTEX_DECLARATION, M_GEOMETRY_VERTEX_BUFFER };
        p[M_GEOMETRY_VERTEX_DECLARATION] = { M_GEOMETRY_VERTEX_ELEMENT };
        p[M_GEOMETRY_VERTEX_BUFFER] = { M_GEOMETRY_VERTEX_BUFFER_DATA };
        p[M_MESH_LOD] = { M_MESH_LOD_USAGE };
        p[M_MESH_LOD_USAGE] = { M_MESH_LOD_MANUAL, M_MESH_LOD_GENERATED };
        p[M_SUBMESH_NAME_TABLE] = { M_SUBMESH_NAME_TABLE_ELEMENT };
        p[M_EDGE_LISTS] = { M_EDGE_LIST_LOD };
        p[M_EDGE_LIST_LOD] = { M_EDGE_GROUP };
        p[M_POSES] = { M_POSE };
        p[M_POSE] = { M_POSE_VERTEX };
        p[M_ANIMATIONS] = { M_ANIMATION };
        p[M_ANIMATION] = { M_ANIMATION_BASEINFO, M_ANIMATION_TRACK };
        p[M_ANIMATION_TRACK] = { M_ANIMATION_MORPH_KEYFRAME, M_ANIMATION_POSE_KEYFRAME };
        p[M_ANIMATION_POSE_KEYFRAME] = { M_ANIMATION_POSE_REF };
    }
    return p[parent].count(sub) != 0;
};



inline const char* meshChunkName(ushort id)
{
    switch(id) {
    case 0: return "[root]";
    case M_HEADER: return "M_HEADER";
    case M_MESH: return "M_MESH";
    case M_SUBMESH: return "M_SUBMESH";
    case M_SUBMESH_OPERATION: return "M_SUBMESH_OPERATION";
    case M_SUBMESH_BONE_ASSIGNMENT: return "M_SUBMESH_BONE_ASSIGNMENT";
    case M_SUBMESH_TEXTURE_ALIAS: return "M_SUBMESH_TEXTURE_ALIAS";
    case M_GEOMETRY: return "M_GEOMETRY";
    case M_GEOMETRY_VERTEX_DECLARATION: return "M_GEOMETRY_VERTEX_DECLARATION";
    case M_GEOMETRY_VERTEX_ELEMENT: return "M_GEOMETRY_VERTEX_ELEMENT";
    case M_GEOMETRY_VERTEX_BUFFER: return "M_GEOMETRY_VERTEX_BUFFER";
    case M_GEOMETRY_VERTEX_BUFFER_DATA: return "M_GEOMETRY_VERTEX_BUFFER_DATA";
    case M_MESH_SKELETON_LINK: return "M_MESH_SKELETON_LINK";
    case M_MESH_BONE_ASSIGNMENT: return "M_MESH_BONE_ASSIGNMENT";
    case M_MESH_LOD: return "M_MESH_LOD";
    case M_MESH_LOD_USAGE: return "M_MESH_LOD_USAGE";
    case M_MESH_LOD_MANUAL: return "M_MESH_LOD_MANUAL";
    case M_MESH_LOD_GENERATED: return "M_MESH_LOD_GENERATED";
    case M_MESH_BOUNDS: return "M_MESH_BOUNDS";
    case M_SUBMESH_NAME_TABLE: return "M_SUBMESH_NAME_TABLE";
    case M_SUBMESH_NAME_TABLE_ELEMENT: return "M_SUBMESH_NAME_TABLE_ELEMENT";
    case M_EDGE_LISTS: return "M_EDGE_LISTS";
    case M_EDGE_LIST_LOD: return "M_EDGE_LIST_LOD";
    case M_EDGE_GROUP: return "M_EDGE_GROUP";
    case M_POSES: return "M_POSES";
    case M_POSE: return "M_POSE";
    case M_POSE_VERTEX: return "M_POSE_VERTEX";
    case M_ANIMATIONS: return "M_ANIMATIONS";
    case M_ANIMATION: return "M_ANIMATION";
    case M_ANIMATION_BASEINFO: return "M_ANIMATION_BASEINFO";
    case M_ANIMATION_TRACK: return "M_ANIMATION_TRACK";
    case M_ANIMATION_MORPH_KEYFRAME: return "M_ANIMATION_MORPH_KEYFRAME";
    case M_ANIMATION_POSE_KEYFRAME: return "M_ANIMATION_POSE_KEYFRAME";
    case M_ANIMATION_POSE_REF: return "M_ANIMATION_POSE_REF";
    case M_TABLE_EXTREMES: return "M_TABLE_EXTREMES";
    }
    return "[unknown-id]";
}


inline bool isSkelSubChunk(ushort parent, ushort sub)
{
    static std::map<ushort, std::set<ushort>> p;
    if (p.empty()) {
        p[0] = { SKELETON_BLENDMODE,
                 SKELETON_BONE,
                 SKELETON_BONE_PARENT,
                 SKELETON_ANIMATION,
                 SKELETON_ANIMATION_LINK };
        p[SKELETON_ANIMATION] = {
                SKELETON_ANIMATION_TRACK,
                SKELETON_ANIMATION_BASEINFO,
                SKELETON_ANIMATION_TRACK_KEYFRAME };
    }
    return p[parent].count(sub) != 0;
};


inline const char* skelChunkName(ushort id)
{
    switch(id) {
    case SKELETON_HEADER: return "SKELETON_HEADER";
    case SKELETON_BLENDMODE: return "SKELETON_BLENDMODE";
    case SKELETON_BONE: return "SKELETON_BONE";
    case SKELETON_BONE_PARENT: return "SKELETON_BONE_PARENT";
    case SKELETON_ANIMATION: return "SKELETON_ANIMATION";
    case SKELETON_ANIMATION_BASEINFO: return "SKELETON_ANIMATION_BASEINFO";
    case SKELETON_ANIMATION_TRACK: return "SKELETON_ANIMATION_TRACK";
    case SKELETON_ANIMATION_TRACK_KEYFRAME: return "SKELETON_ANIMATION_TRACK_KEYFRAME";
    case SKELETON_ANIMATION_LINK: return "SKELETON_ANIMATION_LINK";
    }
    return "[unknown-id]";
}