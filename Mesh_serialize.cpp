#include "Mesh.h"

#include "Except.h"
#include "InputOutput.h"

#include <functional>
#include <fstream>

using namespace std;

/*
*	static function to initialize static vars, so we can run thread-safe
*/
void Mesh::initStaticVariables()
{
	isMeshSubChunk(0, 0);
	isSkelSubChunk(0, 0);
}

void Mesh::clear()
{
    m_sub.clear();
    m_cursub = nullptr;
    m_rootChunk.reset();
    m_headerBuf.clear();
    m_materials.clear();
}


void Mesh::printChunkTree(ostream& out) {
    out << "\n";
    function<void(const shared_ptr<Chunk>&, int)> rec = [&](const shared_ptr<Chunk>& c, int lvl) {
        out << string(lvl*4, ' ') << meshChunkName(c->id) << "(" << hex << c->id << dec << ")  " << c->origSize << " bytes";
        if (c->origSize != c->size)
            out << " (**WRONG, fixed to " << c->size << "  diff=" << c->size - c->origSize << ")";
        out << endl;
        for(auto s: c->sub)
            rec(s, lvl+1);
    };
    rec(m_rootChunk, 0);
}

template<TIsSubCheck FuncIsSubChunk>
class ChunkStack
{
public:
    ChunkStack(const shared_ptr<Chunk> rootChunk) {
        m_stack.push_back(rootChunk);
    }
    shared_ptr<Chunk> push(ushort id, int size) {
        ushort topid = 0;
        while (!m_stack.empty()) {
            const auto& tos = m_stack.back();
            topid = tos->id;
            if (FuncIsSubChunk(topid, id))
                break;
            tos->size = tos->consumedSize;
            m_stack.pop_back();
        }
        CHECK(!m_stack.empty(), "Unexpected chunk id in tree " << topid);

        shared_ptr<Chunk> nc(new Chunk(id, size, m_stack.back().get() ));
        m_stack.back()->sub.push_back(nc);
        m_stack.push_back(nc);
        return nc;
    }
    void consumeSize(int size) {
        for(const auto& chunk: m_stack) {
            chunk->consumedSize += size;
        }

    }
    void consumeBuf(const string& buf) {
        m_stack.back()->selfBuf = buf;
        consumeSize((int)buf.size());
    }

    void checkDone() {

    }
    vector<shared_ptr<Chunk>> m_stack;
};



void Mesh::parse(const string& filename, ostream* out) {
    ifstream inf(filename.c_str(), ios::binary);
    CHECK(inf.good(), "Failed reading file `" << filename << "`");
    parse(inf, out);
}


inline void tostream(std::ostream& os) {
}
template<typename T, typename... Args>
inline void tostream(std::ostream& os, const T& v, const Args&... args) {
    os << v;
    tostream(os, args...);
}

template<typename... Args>
void log(ostream* os, const Args&... args)
{
    if (os == nullptr)
        return;
    tostream(*os, args...);
    *os << endl;
}
template<typename... Args>
void logn(ostream* os, const Args&... args)
{
    if (os == nullptr)
        return;
    tostream(*os, args...);
}
#define LOG(...) log(out, __VA_ARGS__)
#define LOGN(...) logn(out, __VA_ARGS__)


void Mesh::parse(istream& inf, ostream* out)
{
    Deserializer s(inf);

    // read header
    ushort headerid = s.read16();
    CHECK(headerid == 0x1000, "Wrong header id");
    string verstr = s.readStr();
    int fileVer = 0;

    //  "[Serializer_v1.80]" for skeleton
    if (verstr.substr(0,17) == "[MeshSerializer_v")
    { // mesh

        fileVer = atoi(verstr.substr(17).erase(1,1).c_str()); // erase point

        LOG("MESH HEADER ", hex, headerid, dec, " ", verstr, " VER=", fileVer);
        if (fileVer < 100)
            fileVer *= 10; // it had only one decimal number
        CHECK(fileVer > 120, "Unsupported mesh file version " << fileVer);

        m_headerBuf = s.consumedBuf();

        parseMesh(s, out, fileVer);
    }
    else if (verstr.substr(0,13) == "[Serializer_v")
    { // skeleton
        fileVer = atoi(verstr.substr(13).erase(1,1).c_str()); // erase point
        LOG("SKELETON HEADER ", hex, headerid, dec, " ", verstr, " VER=", fileVer);
        parseSkeleton(s, out, fileVer);
    }
    else {
        CHECK(false, "Unexpected version string got:" << verstr);
    }

}

// OgreSkeletonSerializer.cpp
void Mesh::parseSkeleton(Deserializer& s, ostream* out, int fileVer)
{
    m_rootChunk.reset(new Chunk(0, s.remainSize()));
    ChunkStack<isSkelSubChunk> chunkStack(m_rootChunk);


    while (!s.eof())
    {
        int chunkLen = 0;
        auto id = s.chunkHeader(&chunkLen);

        LOG("CHUNK ", hex, id, " ", skelChunkName(id), " (", dec, chunkLen, " bytes)");

        shared_ptr<Chunk> curChunk = chunkStack.push(id, chunkLen);

        int boneIndex = 0;
        switch (id)
        {
        case 0x1000: // header
            LOG("  ", s.readStr());
            break;
        case 0x1010: // blend mode
            LOG("  blendmode=", s.read16());
            break;
        case 0x2000: {// bone
            LOG("  index=", boneIndex++);
            auto name = s.readStr();
            LOG("  name=", name);
            LOG("  handle=", s.read16());
            Vec3 pos, scale;
            pos.x = s.read32f();
            pos.y = s.read32f();
            pos.z = s.read32f();
            LOG("  pos=", pos);
            LOG("  orient=[", s.read32f(), ", ", s.read32f(), ", ", s.read32f(), ", ", s.read32f(), "]");
            if (chunkLen > 2 + 12 + 16 + 6) { // check from Ogre::SkeletonSerializer::readBone
                scale.x = s.read32f();
                scale.y = s.read32f();
                scale.z = s.read32f();
                LOG("  scale=", scale);
            }
            break;
        }
        case 0x3000:  // bone parent
            LOG("  handle=", s.read16());
            LOG("  parentHandle=", s.read16());
            break;
        case 0x4000:
            LOG("  name=", s.readStr());
            LOG("  length=", s.read32f());
            break;
        case 0x4010:
            LOG("  baseName=", s.readStr());
            LOG("  baseKeyFrameTime=", s.read32f());
            break;
        case 0x4100:
            LOG("  boneIndex=", s.read16());
            break;
        case 0x4110: {
            LOG("  time=", s.read32f());
            LOG("  rotate=[", s.read32f(), ", ", s.read32f(), ", ", s.read32f(), ", ", s.read32f(), "]");
            Vec3 trans, scale;
            trans.x = s.read32f();
            trans.y = s.read32f();
            trans.z = s.read32f();
            LOG("  translate=", trans);
            if (chunkLen > 4 + 16 + 12 + 6) { // see Ogre::SkeletonSerializer::readKeyFrame
                scale.x = s.read32f();
                scale.y = s.read32f();
                scale.z = s.read32f();
                LOG("  scale=", scale);
            }
            break;
        }
        case 0x5000: {
            LOG("  skeletonName=", s.readStr());
            LOG("  scale=", s.read32f());
            break;
        }
        default: {
            int cur = s.tellg();
            CHECK(false, "Unsupported id at " << cur);
        }
        } // switch

        string csbuf = s.consumedBuf();
        LOG("  CONSUMED ", csbuf.size());
        chunkStack.consumeBuf(csbuf);

    }
    chunkStack.checkDone();
}

// based on OgreMeshFileFormat.h
void Mesh::parseMesh(Deserializer& s, ostream* out, int fileVer)
{

    m_rootChunk.reset(new Chunk(0, s.remainSize()));
    ChunkStack<isMeshSubChunk> chunkStack(m_rootChunk);


    while (!s.eof())
    {
        int chunkLen = 0;
        auto id = s.chunkHeader(&chunkLen);

        LOG("CHUNK ", hex, id, " ", meshChunkName(id), " (", dec, chunkLen, " bytes)");

        shared_ptr<Chunk> curChunk = chunkStack.push(id, chunkLen);

        switch (id)
        {
        case 0x1000: // header
            LOG("  ", s.readStr());
            break;
        case 0x3000: // mesh
            LOG("  skelAnim= ", (int)s.read8());
            break;
        case 0x4000: {  // submesh
            m_sub.push_back(SubMesh());
            m_cursub = &m_sub.back();
            m_cursub->m_material = s.readStr();
            m_materials.insert(m_cursub->m_material);
            LOG("  material= ", m_cursub->m_material);
            bool useSharedVtx = s.readBool();
            LOG("  useSharedVtx= ", useSharedVtx);
            if (useSharedVtx) {
                CHECK(m_sharedGeom.get() != nullptr, "SharedVtx submesh needs to have shared geom before");
                m_cursub->m_isSharedGeom = useSharedVtx;
            }
            m_cursub->m_indicesCount = s.read32();
            LOG("  indexCount= ", m_cursub->m_indicesCount);
            m_cursub->m_indices32bit = s.readBool();
            LOG("  index32Bit= ", m_cursub->m_indices32bit);
            m_cursub->m_indices.reserve(m_cursub->m_indicesCount);
            LOGN("  indexes=");
            for (uint i = 0; i < m_cursub->m_indicesCount; ++i) {
                if ((i % 20) == 0)
                    LOGN("\n    ");
                uint idx;
                if (m_cursub->m_indices32bit)
                    idx = s.read32();
                else
                    idx = s.read16();
                LOGN(idx, " ");
                m_cursub->m_indices.push_back(idx);
            }
            LOG(""); // end the line
            break;
        }
        case 0x4010:
            LOG("  operationType= ", s.read16());
            break;
        case 0x4100: { // M_SUBMESH_BONE_ASSIGNMENT
            CHECK(m_cursub != nullptr, "missing submesh");
            m_cursub->m_boneAssign.push_back(BoneAssign());
            auto& b = m_cursub->m_boneAssign.back();
            b.vertexIndex = s.read32();
            b.boneIndex = s.read16();
            b.weight = s.read32f();
            LOG("  ", m_cursub->m_boneAssign.size() - 1, "> vertexIndex= ", b.vertexIndex,
                "   boneIndex= ", b.boneIndex, "  weight= ", b.weight);
            break;
        }
        case 0x5000: { // M_GEOMETRY
            //CHECK(m_cursub != nullptr, "missing submesh");
            if (m_cursub == nullptr) { // must be shared geom
                CHECK(m_sharedGeom.get() == nullptr, "Can't have more than 1 shared geometry");
                m_sharedGeom.reset(new SubMesh);
                m_cursub = m_sharedGeom.get();
            }
            m_cursub->m_vertexCount = s.read32();
            m_cursub->m_vtx.resize(m_cursub->m_vertexCount);
            LOG("  VertexCount= ", m_cursub->m_vertexCount);
            break;
        }
        case 0x5100: // M_GEOMETRY_VERTEX_DECLARATION no data for this
            break;
        case 0x5110: { // M_GEOMETRY_VERTEX_ELEMENT
            ushort source = s.read16();
            LOGN("  source= ", source);
            ushort type = s.read16();
            LOGN(", type= ", typeName(type), "(", type, ")");
            ushort sem = s.read16();
            LOG(", semantic= ", semanticName(sem), "(", sem, ")");
            int offset = s.read16();
            LOGN("  offset= ", offset);
            int index = s.read16();
            LOG("  index= ", index);

            // validation that it's something we support
            if (sem == VES_TEXTURE_COORDINATES) {
                CHECK(index < 4, "Indexed sematics for texture coordinates supports only 4 m_entries");
                CHECK(index == m_cursub->m_texCoordCount, "Vertex coordinate index in nor monotonic");
                ++m_cursub->m_texCoordCount;
            }
            else
                CHECK(index == 0, "Indexed sematics supported only for texture coordinates");
            auto vtxflag = vtxFlagFromSem(sem, index);
            CHECK(!checkFlag(m_cursub->m_hasEntries, vtxflag), "Same fields appears more than once");
            m_cursub->m_hasEntries |= vtxflag;

            if (source >= m_cursub->m_entries.size()) {
                m_cursub->m_entries.push_back(VtxBind());
            }
            CHECK(source == m_cursub->m_entries.size() - 1, "Vertex sources are not monotonic");
            VtxBind* entriesBind = &m_cursub->m_entries.back();

            if (entriesBind->e.empty())
                CHECK(offset == 0, "Vertex m_entries are not monotonic-0");
            else {
                const auto& lastEntry = entriesBind->e.back();
                CHECK(offset = lastEntry.offset + typeSize(lastEntry.type), "Vertex m_entries are not monotonic");
            }
            entriesBind->e.push_back(VtxEntry{source, offset, type, semanticName(sem), sem, index, curChunk});
            entriesBind->entriesSize = offset + typeSize(type);

            break;
        }
        case 0x5200: { // M_GEOMETRY_VERTEX_BUFFER
            int newBind = s.read16();
            CHECK(newBind > m_cursub->m_vertexBind, "Bound buffers not monotonic");
            // index of the buffer
            m_cursub->m_vertexBind = newBind;
            LOG("  bindIndex= ", m_cursub->m_vertexBind);
            m_cursub->m_vertexSize = s.read16();
            LOG("  vertexSize= ", m_cursub->m_vertexSize);
            CHECK(m_cursub->m_vertexBind < m_cursub->m_entries.size(), "Bind buffer that was not declared");
            auto& entBind = m_cursub->m_entries[m_cursub->m_vertexBind];
            CHECK(m_cursub->m_vertexSize == entBind.entriesSize, "Buffer entry vertex entry size different from declaraion");
            entBind.bufferChunk = curChunk;
            break;
        }
        case 0x5210: { // M_GEOMETRY_VERTEX_BUFFER_DATA
            LOGN("  vertices=");
            for(int i = 0; i < m_cursub->m_vertexCount; ++i)
            {
                bool doOut = m_outAllVertices || (i == 0) || (i == m_cursub->m_vertexCount - 1);
                if (doOut)
                    LOGN("\n    ", i, "> ");
                VtxInfo & vtx = m_cursub->m_vtx[i];
                vtx.index = i;

                int startOffset = s.tellg();

                for(const auto& e: m_cursub->m_entries[m_cursub->m_vertexBind].e)
                {
                    if (doOut)
                        LOGN(string(e.name).substr(4,3), ":");
                    float vf[4] = {0};
                    uint vi = 0;
                    switch(e.type) {
                    case VET_FLOAT1:
                    case VET_FLOAT2:
                    case VET_FLOAT3:
                    case VET_FLOAT4: {
                        int count = e.type - VET_FLOAT1 + 1;
                        if (doOut)
                            LOGN("(");
                        for(int j = 0; j < count; ++j) {
                            vf[j] = s.read32f();
                            if (doOut)
                                LOGN(vf[j], ((j < count-1) ? ", " : ")"));
                        }
                        break;
                    }
                    case VET_COLOUR_ABGR:
                    case VET_COLOUR_ARGB:
                        vi = s.read32();
                        if (doOut)
                            LOGN(hex, "0x", vi, dec);
                        break;
                    default:
                        throw Exception("Unsupported vertex type");
                    }; // type switch
                    if (doOut)
                        LOGN("\t");

                    //CHECK(!vtx.has[e.sem], "Vertex already has this semantic"); // can happen with multiple sematics with indices
                    switch(e.sem)
                    {
                    case VES_POSITION: vtx.pos.set(e.type, vf); break;
                    case VES_NORMAL:   vtx.normal.set(e.type, vf); break;
                    case VES_TANGENT:  vtx.tangent.set(e.type, vf); break;
                    case VES_BINORMAL: vtx.binormal.set(e.type, vf); break;
                    case VES_TEXTURE_COORDINATES:
                        vtx.tex[e.index].set(e.type, vf);
                        break;
                    case VES_DIFFUSE:
                        CHECK(e.type == VET_COLOUR_ABGR || e.type == VET_COLOUR_ARGB, "unexpected diffuse type");
                        vtx.diffuse = vi;
                        break;
                    default:
                        throw Exception("Unexpected sematic");
                    }
                    //vtx.has[e.sem] = true;

                }

                int endOffset = s.tellg();
                CHECK(endOffset - startOffset == m_cursub->m_entries[m_cursub->m_vertexBind].entriesSize, "Unexpected entry size");
                CHECK(vtx.selfBufs.size() == m_cursub->m_vertexBind, "Unexpected selfBufs size");
                vtx.selfBufs.push_back(s.getSubBuf(startOffset, endOffset));


            }
            LOG("");
            break;
        }
        case 0x6000: { // M_MESH_SKELETON_LINK
            LOG("  skeletonName= ", s.readStr());
            break;
        }
        case 0x9000: { // M_MESH_BOUNDS
            LOGN("  minX= ", s.read32f(), "  ");
            LOGN("  minY= ", s.read32f(), "  ");
            LOG("  minZ= ", s.read32f(), "  ");
            LOGN("  maxX= ", s.read32f(), "  ");
            LOGN("  maxY= ", s.read32f(), "  ");
            LOG("  maxZ= ", s.read32f(), "  ");
            LOG("  radius= ", s.read32f());
            break;
        }
        case 0xA000: // M_SUBMESH_NAME_TABLE
            break;
        case 0xA100: // M_SUBMESH_NAME_TABLE_ELEMENT
            LOGN("  index= ", s.read16());
            LOG("  name= ", s.readStr());
            break;
        case 0xb000: // M_EDGE_LISTS
            m_cursub->m_hasEdges = true;
            break;
        case 0xb100: { // M_EDGE_LIST_LOD
            LOG("  lodIndex= ", s.read16());
            bool isManual = s.readBool();
            LOG("  isManual= ", isManual);
            if (!isManual) {
                LOG("  isClosed= ", s.readBool());
                uint numTriangles = s.read32();
                LOG("  numTriangles= ", numTriangles);
                uint numEdgeGroups = s.read32(); // the number of subsequent chunks for groups
                LOG("  numEdgeGroups= ", numEdgeGroups);
                for(uint i = 0; i < numTriangles; ++i) {
                    uint indexSet = s.read32();
                    uint vertexSet = s.read32();
                    uint vertIndex[3], sharedVertIndex[3];
                    LOGN("    TRI: iset=", indexSet, " vset=", vertexSet, " idx=");
                    for(int j = 0; j < 3; ++j) {
                        vertIndex[j] = s.read32();
                        LOGN(vertIndex[j], ",");
                    }
                    LOGN("  sharedIdx=");
                    for(int j = 0; j < 3; ++j) {
                        sharedVertIndex[j] = s.read32();
                        LOGN(sharedVertIndex[j], ",");
                    }
                    LOGN("  normal=");
                    float normal[4];
                    for(int j = 0; j < 4; ++j) {
                        normal[j] = s.read32f();
                        LOGN(normal[j], ",");
                    }
                    LOG("");
                }
            }
            break;
        }
        case 0xb110: { // M_EDGE_GROUP
            LOGN("  vertexSet=", s.read32());
            LOGN("  triStart=", s.read32());
            LOGN("  triCount=", s.read32());
            uint numEdges = s.read32();
            LOG("  numEdges=", numEdges);
            for(uint i = 0; i < numEdges; ++i) {
                LOGN("    EDGE: triIdx=", s.read32());
                LOGN(",", s.read32());
                LOGN("  vertIdx=", s.read32());
                LOGN(",", s.read32());
                LOGN("  shaderVertIdx=", s.read32());
                LOGN(",", s.read32());
                LOG("   degenerate=", s.readBool());
            }
            break;
        }
        case 0xd000: // M_ANIMATIONS
            m_cursub->m_hasVtxAnimation = true;
            break;
        case 0xd100: // M_ANIMATION
            LOG("  name=", s.readStr());
            LOG("  length=", s.read32f());
            break;
        case 0xd105: // M_ANIMATION_BASEINFO
            LOG("  baseAnimationName=", s.readStr());
            LOG("  baseKeyFrameTime=", s.read32f());
            break;
        case 0xd110: // M_ANIMATION_TRACK
            LOG("  type=", s.read16());
            LOG("  target=", s.read16());
            break;
        case 0xd111: { // M_ANIMATION_MORPH_KEYFRAME
            LOG("  time=", s.read32f());
            bool hasNormals = false;
            if (fileVer >= 180) {
                hasNormals = s.readBool(); // 1.8+ only
                LOG("  includesNormals=", hasNormals);
            }
            for(int i = 0; i < m_cursub->m_vertexCount; ++i)  { // using vertexCount here is wrong
                Vec3 vtx, normal;
                vtx.x = s.read32f();
                vtx.y = s.read32f();
                vtx.z = s.read32f();
                LOGN("    ", i, "> POS=", vtx);
                if (hasNormals) {
                    normal.x = s.read32f();
                    normal.y = s.read32f();
                    normal.z = s.read32f();
                    LOGN("    NORMAL=", normal);
                }
                LOG("");
            }
            break;
        }
        case 0xd112: // M_ANIMATION_POSE_KEYFRAME
            LOG("  time=", s.read32f());
            break;
        case 0xd113: // M_ANIMATION_POSE_REF
            LOG("  poseIndex=", s.read16());
            LOG("  influence=", s.read32f());
            break;

        default: {
            int cur = s.tellg();
            CHECK(false, "Unsupported id at " << cur);
        }
        } // switch

        string csbuf = s.consumedBuf();
        LOG("  CONSUMED ", csbuf.size());
        chunkStack.consumeBuf(csbuf);

    }
    chunkStack.checkDone();

}

struct SaveState
{
    SaveState(Mesh& mesh) : m_mesh(mesh)
    {}

    void recSave(Serializer& s, const shared_ptr<Chunk>& chunk);

    Mesh& m_mesh;

    int m_cursubIndex = -1;
    SubMesh* m_cursub = nullptr;
    int m_bindIndex = -1;

    int m_writeEntryBind = 0; // bind of the next entry that needs to be written
    int m_writeEntryIndex = 0; // next entry that needs to be written
    int m_writeEntryOffset = 0; // offset in buffer of the entry after the one we just wrote, for validation

    int m_boneAssignIndex = -1; // index of the last bone assignment chunk
};

void SaveState::recSave(Serializer& s, const shared_ptr<Chunk>& chunk)
{
    s.write16(chunk->id);
    s.write32(chunk->size); // fixed size

    m_cursub = m_mesh.m_sharedGeom.get(); // geometry that comes before submesh is the shared geometry

    bool wrote = false;
    switch (chunk->id)
    {
    case 0x4000: // M_SUBMESH
        ++m_cursubIndex;
        m_cursub = &m_mesh.m_sub[m_cursubIndex];
        m_bindIndex = -1;

        m_writeEntryBind = 0;
        m_writeEntryIndex = 0;
        m_writeEntryOffset = 0;
        m_boneAssignIndex = -1;

        s.writeStr(m_cursub->m_material);
        s.writeBool(m_cursub->m_isSharedGeom);
        CHECK(m_cursub->m_indicesCount == m_cursub->m_indices.size(), "Modified indicies but not count?");
        s.write32(m_cursub->m_indicesCount);
        s.writeBool(m_cursub->m_indices32bit);
        for(uint i = 0; i < m_cursub->m_indicesCount; ++i) {
            if (m_cursub->m_indices32bit)
                s.write32(m_cursub->m_indices[i]);
            else
                s.write16(m_cursub->m_indices[i]);
        }
        wrote = true;
        break;
    case 0x4100: { // M_SUBMESH_BONE_ASSIGNMENT
        ++m_boneAssignIndex;
        CHECK(m_boneAssignIndex < m_cursub->m_boneAssign.size(), "Unexpected bone assign chunk");
        const auto& b = m_cursub->m_boneAssign[m_boneAssignIndex];
        s.write32(b.vertexIndex);
        s.write16(b.boneIndex);
        s.write32f(b.weight);
        wrote = true;
        break;
    }
    case 0x5000: // M_GEOMETRY
        s.write32(m_cursub->m_vertexCount);
        wrote = true;
        break;
    case 0x5110: { // M_GEOMETRY_VERTEX_ELEMENT
        CHECK(m_writeEntryBind < m_cursub->m_entries.size(), "Unexpected bind index");
        const auto& bind = m_cursub->m_entries[m_writeEntryBind];
        CHECK(m_writeEntryIndex < bind.e.size(), "Unexpected entry index");
        const auto& entry = bind.e[m_writeEntryIndex];
        CHECK(entry.offset == m_writeEntryOffset, "Unexpected entry offset");
        s.write16(entry.source);
        s.write16(entry.type);
        s.write16(entry.sem);
        s.write16(entry.offset);
        s.write16(entry.index);
        ++m_writeEntryIndex;
        m_writeEntryOffset += typeSize(entry.type);
        if (m_writeEntryIndex == bind.e.size()) {
            ++m_writeEntryBind;
            m_writeEntryIndex = 0;
            m_writeEntryOffset = 0;
        }
        wrote = true;
        break;
    }
    case 0x5200: // M_GEOMETRY_VERTEX_BUFFER
        ++m_bindIndex;
        s.write16((ushort)m_bindIndex);
        s.write16((ushort)m_cursub->m_entries[m_bindIndex].entriesSize); // might have been updated changing fields
        wrote = true;
        break;
    case 0x5210: { // M_GEOMETRY_VERTEX_BUFFER_DATA
        for(int i = 0; i < m_cursub->m_vertexCount; ++i) {
            s.write( m_cursub->m_vtx[i].selfBufs[m_bindIndex] );
        }
        wrote = true;
        break;
    }
    } // switch

    // generic write of the chunk content, for chunks that were not written above
    if (!wrote)
        s.write(6, chunk->selfBuf);

    for(const auto& child: chunk->sub) {
        recSave(s, child);
    }
}


void Mesh::save(const string& filename) {
    ofstream outf(filename, ios::binary);
    CHECK(outf.good(), "Failed reading file `" << filename << "`");
    save(outf);
}

void Mesh::save(ostream& outf)
{
    Serializer s(outf);
    s.write(m_headerBuf);

    SaveState state(*this);
    for(const auto& child: m_rootChunk->sub) {
        state.recSave(s, child);
    }
}

uint Mesh::gatheredEntries() {
    uint gatheredEntries = 0;
    for(const auto& sub: m_sub)
        gatheredEntries |= sub.m_hasEntries;
    return gatheredEntries;
}


int Mesh::countVtx() {
    int count = 0;
    for(auto& sub: m_sub) {
        CHECK(sub.m_vtx.size() == sub.m_vertexCount, "vertex count inconsistant");
        count += sub.m_vtx.size();
    }
    if (m_sharedGeom)
        count += m_sharedGeom->m_vtx.size();
    return count;
}

int Mesh::countTri() {
    int count = 0;
    for(auto& sub: m_sub) {
        count += sub.m_indices.size() / 3;
    }
    return count;
}


template<typename TC, typename TF>
bool anyOf(const TC& container, const TF& pred) {
    for(const auto& a: container)
        if (pred(a))
            return true;
    return false;
}
template<typename TC, typename TF>
uint maxOf(const TC& container, const TF& pred) {
    uint maxv = 0;
    for(const auto& a: container) {
        uint v = (uint)pred(a);
        if (v > maxv) {
            maxv = v;
        }
    }
    return false;
}

bool Mesh::hasBinormal() {
    return anyOf(m_sub, [](const SubMesh& s) { return checkFlag(s.m_hasEntries, VF_BINORMAL); });
}
bool Mesh::hasEdgeList() {
    return anyOf(m_sub, [](const SubMesh& s) { return s.m_hasEdges; });
}
bool Mesh::hasVertexAnim() {
    return anyOf(m_sub, [](const SubMesh& s) { return s.m_hasVtxAnimation; });
}

void Mesh::statline(ostream& out)
{
    out << "VtxC " << countVtx();

    if (m_sub.size() == 1)
        out << ", OneSub";
    else if (m_sub.size() != m_materials.size())
        out << ", DiffSubs=" << m_materials.size() << "/" << m_sub.size();
    else
        out << ", Subs=" << m_sub.size();

    int texCount = maxOf(m_sub, [](const SubMesh& s) { return s.m_texCoordCount; });
    if (texCount == 0)
        out << ", NoTexC";
    else if (texCount == 1)
        out << ", OneTexC";
    else
        out << ", TexC=" << texCount;

    int bindCount = maxOf(m_sub, [](const SubMesh& s) { return s.m_entries.size(); });
    if (bindCount == 1)
        out << ", OneBuf";
    else
        out << ", Bufs=" << bindCount;

    if ( hasBinormal())
        out << ", Binormal";
    else
        out << ", NoBinorm";

    if ( hasEdgeList())
        out << ", EdgeList";
    else
        out << ", NoEdgeLs";

    if ( hasVertexAnim() )
        out << ", VtxAnim";
    else
        out << ", NoVtxAn";

}

