#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include <set>
#include "ogre_types.h"
#include "helper_types.h"

class Chunk;

struct VtxEntry {
    int source;  // index of buffer this is in
    int offset;
    ushort type;
    const char* name;
    int sem;
    int index; // support multiple m_entries of the same type only for texture coordinates

    shared_ptr<Chunk> entryChunk; // the 0x5110 chunk for this entry
};

// represends the data the is expected in a buffer
struct VtxBind {
    vector<VtxEntry> e;
    int entriesSize = 0; // total size of all the entries in this bind
    shared_ptr<Chunk> bufferChunk; // the 0x5200 chunk that contains this buffer
};

// bit mask constants, each identifying a property of a vertex
#define VF_POSITION 0x01
#define VF_NORMAL 0x02
#define VF_DIFFUSE 0x04
#define VF_TEXCOORD0 0x08
#define VF_TEXCOORD1 0x10
#define VF_TEXCOORD2 0x20
#define VF_TEXCOORD3 0x40
#define VF_TANGENT 0x100
#define VF_BINORMAL 0x200
#define ALL_BUT(n) (~n)

uint vtxFlagFromSem(int sem, int index);


// everything we know about a certain vertex
struct VtxInfo
{
    VtxInfo() {
    }

    int index = -1;
    Vec3 pos;
    Vec3 normal;
    Vec2 tex[4];
    uint diffuse;
    Vec3 tangent;
    Vec3 binormal;

    vector<string> selfBufs; // data buffer from the file indexed by the buffer it was in
    VtxInfo * isDupOf = NULL;
    bool isUsed = true; // for terrain culling
};

// saved for vertex index translation
struct BoneAssign {
    uint vertexIndex;
    ushort boneIndex;
    float weight;
};


struct QuadIndex
{
    int d1, d2, dex1, dex2;
    int t1, t2; // triangles first index where this quad came from

    void init() {
        d1 = -1; d2 = -1; dex1 = -1; dex2 = -1;
        t1 = -1; t2 = -1;
    }
};

struct Quad2D
{
    float x1, z1, x2, z2;
    QuadIndex tinf;
};


// a chunk in the recursive chunk struction of the file
class Chunk
{
public:
    Chunk(ushort _id = 0, int _size = 0, Chunk* _parent = nullptr)
            :id(_id), size(_size), origSize(_size), consumedSize(0), parent(_parent)
    {}
    ~Chunk()
    {}

    // remove this chunk and all its children from the tree (invalidates iterators)
    void detach() {
        parent->detachChild(this);
    }
    void detachChild(Chunk* child) {
        for(auto it = sub.begin(); it != sub.end(); ++it) {
            if (it->get() == child) {
                sub.erase(it);
                return;
            }
        }
        CHECK(false, "Could not find deleted child");
    }

    ushort id;
    int origSize;      // size from chunk header
    int size;          // fixed size according to consumed data
    vector<shared_ptr<Chunk>> sub;
    int consumedSize;  // during parse - how many bytes of this chunk were consumed
    //int remainSize;    // during parse - how many bytes are left
    string selfBuf;
    Chunk* parent;
};

class SubMesh
{
public:
    void dedup(vector<int>* outOldToNew = nullptr);
    void removeField(int sem, int index);
    int dupsExact(int sem = -1, int index = 0);
    int dupsByVecEpsilon(float epsilon, int vtxFlag); // VF_TANGENT or VF_BINORMAL
    void unifyBuffers();
    bool buffersNeedUnify();
    void cullFaces(const vector<Vec3>& possibleEyes, SubMesh* sharedGeom);

    void clearIsDupOf();
    void clearUsed();
    void fixIndices(const vector<int>& oldToNew);

    int m_vertexSize = 0;
    int m_vertexCount = 0;
    int m_vertexBind = -1;

    // for every bound buffer, a list of m_entries which describe the fields
    vector<VtxBind> m_entries;
    uint m_hasEntries = 0; // or-ed VF_XXX
    vector<VtxInfo> m_vtx;

    void extractQuads(float height, SubMesh* sharedGeom, vector<Quad2D>* outQuads);

public:
    bool m_hasVtxAnimation = false;
    bool m_hasEdges = false;
    int m_texCoordCount = 0;
    string m_material;
    uint m_indicesCount = 0;
    bool m_indices32bit = false;
    vector<uint> m_indices;
    vector<BoneAssign> m_boneAssign;
    bool m_isSharedGeom = false;
};

class Deserializer;

class QuadGrid;
class Mesh
{
public:
	static void initStaticVariables();
    // out is for the standard output of the mesh dump
    void parse(const string& filename, ostream* out);
    void parse(istream& infile, ostream* out);

    void printChunkTree(ostream& out);

    int dupsByTanEpsilon(float epsilon);
    int dupsExact(int sem = -1, int index = 0);

    void dedup();
    void removeField(int sem, int index);
    void unifyBuffers();
    bool buffersNeedUnify();

    void cullFaces(const vector<Vec3>& possibleEyes);

    void save(const string& filename);
    void save(ostream& outfile);

    void statline(ostream& out);
    void clear();
    uint gatheredEntries();

    bool hasBinormal();
    bool hasEdgeList();
    bool hasVertexAnim();

    void setEpsilon(float e) {
        m_defaultEpsilon = e;
    }

    void exportObj(const string& filename);

    int countVtx();
    int countTri();

    bool extractQuads(float height, QuadGrid* grid);
    void replaceQuads(const QuadGrid& grid);

    void removeDupTri();
    vector<float> getPlaneHeights() const;
    void clearUsed();
    void markUsedVertices();

private:
    void parseMesh(Deserializer& s, ostream* out, int fileVer);
    void parseSkeleton(Deserializer& s, ostream* out, int fileVer);

public:
    vector<SubMesh> m_sub;
    SubMesh* m_cursub = nullptr;
    shared_ptr<Chunk> m_rootChunk;
    string m_headerBuf;
    set<string> m_materials; // keep track if there is more than one material
    shared_ptr<SubMesh> m_sharedGeom; // if shared geometry exists, this holds it (not all fields of SubMesh used)

    bool m_outAllVertices = false; // should parsing output a live for each vertex with its info? (lots of data)
    float m_defaultEpsilon = 0.2f;

    void cullFaces();
};
