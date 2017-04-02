#include <cmath>
#include <cfloat>
#include <fstream>
#include "Mesh.h"



// translate ogre "sematic" fields + the semantic index to the above mask constant
uint vtxFlagFromSem(int sem, int index) {
    switch(sem) {
    case VES_POSITION: CHECK(index == 0, "Undexpected index"); return VF_POSITION;
    case VES_NORMAL:   CHECK(index == 0, "Undexpected index"); return VF_NORMAL;
    case VES_DIFFUSE:  CHECK(index == 0, "Undexpected index"); return VF_DIFFUSE;
    case VES_TEXTURE_COORDINATES:
        CHECK(index < 4, "Undexpected index");
        return (VF_TEXCOORD0 << index);
    case VES_BINORMAL: CHECK(index == 0, "Undexpected index"); return VF_BINORMAL;
    case VES_TANGENT:  CHECK(index == 0, "Undexpected index"); return VF_TANGENT;
    case VES_SPECULAR:
    case VES_BLEND_WEIGHTS:
    case VES_BLEND_INDICES:
    default:
        CHECK(false, "Undexpeceted sematic");
    }
}

// copy the data bytes to a string
template<typename T>
string makeStr(const T& v) {
    string s;
    s.resize(sizeof(v));
    memcpy((char*)s.data(), (const char*)&v, sizeof(v));
    return s;
}

typedef string VtxKey;

string makeKey(const VtxInfo & v, uint which) {
    // these are the fields the are going to be compared exactly
    string k;
    if (checkFlag(which, VF_POSITION))  k += makeStr(v.pos);
    if (checkFlag(which, VF_NORMAL))    k += makeStr(v.normal);
    if (checkFlag(which, VF_DIFFUSE))   k += makeStr(v.diffuse);
    if (checkFlag(which, VF_TEXCOORD0)) k += makeStr(v.tex[0]);
    if (checkFlag(which, VF_TEXCOORD1)) k += makeStr(v.tex[1]);
    if (checkFlag(which, VF_TEXCOORD2)) k += makeStr(v.tex[2]);
    if (checkFlag(which, VF_TEXCOORD3)) k += makeStr(v.tex[3]);
    if (checkFlag(which, VF_TANGENT))   k += makeStr(v.tangent);
    if (checkFlag(which, VF_BINORMAL))  k += makeStr(v.binormal);
    return k;
}

float myfabs(float v) {
    return (v < 0) ? (-v) : v;
}
// not used
bool isSameManhatten(const Vec3& a, const Vec3& b, float epsilon) {
    return myfabs(a.x - b.x) < epsilon &&
           myfabs(a.y - b.y) < epsilon &&
           myfabs(a.z - b.z) < epsilon;
}
// (not used) distance between normalized vectors that are small enough is a good approximation of the angle between them in radians
double dist(const Vec3& a, const Vec3& b) {
    double dx = (double)a.x - (double)b.x;
    double dy = (double)a.y - (double)b.y;
    double dz = (double)a.z - (double)b.z;
    double dsq = dx*dx + dy*dy + dz*dz;
    double d = sqrt(dsq);
    return d;
}
// not used
bool isSame(const Vec3& a, const Vec3& b, float epsilon) {
    double d = dist(a, b);
    return (d < (double)epsilon);
}

Vec3 getValue(VtxInfo* vtx, int vtxFlag) {
    switch(vtxFlag) {
    case VF_POSITION: return vtx->pos;
    case VF_NORMAL:   return vtx->normal;
    case VF_BINORMAL: return vtx->binormal;
    case VF_TANGENT:  return vtx->tangent;
    default:
        CHECK(false, "Undexpeceted vtxFlag");
    }
}


struct DiffValue {
    DiffValue(const Vec3 _val, VtxInfo * first) : val(_val) {
        vtxs.push_back(first);
    }
    Vec3 val;
    vector<VtxInfo *> vtxs;
};

// set isDupOf according by comparing the tangent of vertices that are otherwise exactly the same
int SubMesh::dupsByVecEpsilon(float epsilon, int vtxFlags)
{
    if (!checkFlag(m_hasEntries, vtxFlags))
        return 0;

    map<string, vector<VtxInfo *>> acc;     // pointers to m_vtx

    for(auto& vtx: m_vtx) {
        acc[makeKey(vtx, ALL_BUT(vtxFlags))].push_back(&vtx);
    }

    // there might still be differences in the tangent, mark only those who are actually dups

    int epsilonDups = 0;
    for(auto it = acc.begin(); it != acc.end(); ++it)
    {
        vector<VtxInfo *>& samevtxs = it->second;
        if (samevtxs.size() == 1)
            continue;

        for(int i = 0; i < 16; ++i) // awkward way to go over the lit bits in vtxFlags
        {
            int vtxFlag = 1 << i;
            if ((vtxFlags & vtxFlag) == 0)
                continue;
            vector<DiffValue> different; // for every value of Tangent that is really distinct, have a list of vertices
            for(auto vtxp : samevtxs)
            {
                if (vtxp->isDupOf != nullptr)
                    continue;
                bool foundSame = false;
                const auto& val = getValue(vtxp, vtxFlag); // ->tangent;
                for(auto& df : different)
                {
                    if (isSame(val, df.val, epsilon)) {
                        df.vtxs.push_back(vtxp);
                        vtxp->isDupOf = df.vtxs[0];
                        ++epsilonDups;
                        foundSame = true;
                        break;
                    }
                }
                if (!foundSame)
                    different.push_back(DiffValue(val, vtxp));
            }
        }
    }

    //cout << "EpsilonDups," << epsilonDups << "," << m_vtx.size() <<  ", " << (float)epsilonDups / m_vtx.size() * 100.0 << "%";

    return epsilonDups;
}

// marks duplicates
int Mesh::dupsByTanEpsilon(float epsilon)
{
    if (epsilon == 0.0) {
        if (m_defaultEpsilon != 0.0)
            epsilon = m_defaultEpsilon;
        else
            epsilon = 0.2f;
    }
    int count = 0;
    for(auto& sub: m_sub) {
        sub.clearIsDupOf();
        count += sub.dupsByVecEpsilon(epsilon, VF_TANGENT | VF_BINORMAL);
    }

    return count;
}



// argument is useful for checking how many vertices are going to be duplicated it a field is removed
int SubMesh::dupsExact(int sem, int index)
{
#ifndef MESH_TOOL
    return 0; // this function takes a long time so it was disabled. removing duplicate vertices is apparently not important
#else
    uint keyFlag = m_hasEntries; // default is by all entries
    if (sem != -1) {
        uint gotFlag = vtxFlagFromSem(sem, index);
        if (!checkFlag(m_hasEntries, keyFlag)) //, "Mesh does not have entry " << semanticName(sem) << " " << sem << " index " << index);
            return 0;
        keyFlag = ALL_BUT(gotFlag);
    }
    // maps key to the first occurance of this information
    map<string, VtxInfo *> acc;     // pointers to m_vtx

    int count = 0;
    for(auto& vtx: m_vtx) {
        auto k = makeKey(vtx, keyFlag);
        auto it = acc.find(k);
        if (it == acc.end())
            acc[k] = &vtx;
        else {
            vtx.isDupOf = it->second;
            ++count;
        }
    }
    return count;
#endif
}

void SubMesh::clearIsDupOf()
{
    for(auto& vtx: m_vtx) {
        vtx.isDupOf = nullptr;
    }
}

void SubMesh::clearUsed()
{
    for(auto& vtx: m_vtx) {
        vtx.isUsed = false;
    }
}




// find duplicates of verteces that contain exactly the same information
// useful after removing fields
int Mesh::dupsExact(int sem, int index) {
    int count = 0;
    for(auto& sub: m_sub) {
        sub.clearIsDupOf();
        count += sub.dupsExact(sem, index);
    }
    return count;
}


// given an isDupOf field on vertices, deduplicate the mesh
void SubMesh::dedup(vector<int>* outOldToNew)
{
    CHECK(!m_hasEdges, "Deduplication no supported for mesh with edge data");
    // this would require going over the entire animation to verify the the duplication
    CHECK(!m_hasVtxAnimation, "Deduplication not supported for mesh with vertex animation");
    if (m_isSharedGeom)
        return;

    // create a new vertices list
    uint countVtx = (uint)m_vtx.size();
    vector<int> oldToNew(countVtx);
    fill(oldToNew.begin(), oldToNew.end(), -1);
    vector<VtxInfo> newvtx;
    newvtx.reserve(countVtx);

    for(uint i = 0; i < countVtx; ++i)
    {
        auto& vtx = m_vtx[i];
        if (vtx.isDupOf != NULL) {
            // the index to this vertex is transated to the index of the vertex it is duplicate of in the new array
            oldToNew[i] = oldToNew[vtx.isDupOf->index];
        }
        else if (vtx.isUsed) { // for terrain culling
            uint newIndex = (uint)newvtx.size();
            oldToNew[i] = newIndex;
            newvtx.push_back(vtx);
            newvtx.back().index = newIndex;
        }
    }

    m_vertexCount = (int)newvtx.size();
    m_vtx = std::move(newvtx);

    fixIndices(oldToNew);

    if (outOldToNew)
        *outOldToNew = std::move(oldToNew);
}

// given a mapping of old indices to new, go over the m_indices list and fix it
void SubMesh::fixIndices(const vector<int>& oldToNew)
{
    CHECK(m_indices.size() == m_indicesCount, "mismatch indices size?"); // sanity
    // create a new indices list
    if (m_indicesCount == 0)
        return; // in case this is the shared geometry submesh

    vector<uint> newindices(m_indicesCount); // same size as the existing one
    for(uint i = 0; i < m_indicesCount; ++i)
    {
        int ni = oldToNew[m_indices[i]];
        CHECK(ni != -1, "new index not found");
        newindices[i] = (uint)ni;
    }

    // translate the bone assignments indices
    for(auto& b: m_boneAssign) {
        int ni = oldToNew[b.vertexIndex];
        CHECK(ni != -1, "new index not found (bone)");
        b.vertexIndex = ni;
    }

    // commit
    m_indices = std::move(newindices);
}

// removed the vertices that has a duplicate or marked as unused and fix the indices
void Mesh::dedup()
{
    for(auto& sub: m_sub)
        sub.dedup();

    if (m_sharedGeom.get() != nullptr)
    {
        vector<int> sharedOldToNew;
        m_sharedGeom->dedup(&sharedOldToNew);
        for(auto& sub: m_sub) {
            if (sub.m_isSharedGeom) {
                sub.fixIndices(sharedOldToNew);
            }
        }
    }
}


void SubMesh::cullFaces(const vector<Vec3>& possibleEyes, SubMesh* sharedGeom)
{
    vector<Vec3> npossibleEyes;
    for(auto n: possibleEyes) {
        n.normalize();
        npossibleEyes.push_back(n);
    }

    int countIdx = (int)m_indices.size();
    vector<uint> newindices;
    newindices.reserve(countIdx);

    int culledTri = 0, totalTri = 0;

    vector<VtxInfo>& vtx = m_isSharedGeom ? sharedGeom->m_vtx : m_vtx;

    int i = 0;
    while (i < countIdx)
    {
        int ai = m_indices[i];
        int bi = m_indices[i+1];
        int ci = m_indices[i+2];
        i += 3;
        ++totalTri;

        Vec3 a = vtx[ai].pos;
        Vec3 b = vtx[bi].pos;
        Vec3 c = vtx[ci].pos;
        Vec3 ab = b - a;
        Vec3 ac = c - a;
        Vec3 norm = Vec3::crossProd(ab, ac);
        float len = norm.length();
        norm.normalize(len);

        bool cull = true;
        for(const auto& n: npossibleEyes) {
            float dp = Vec3::dotProd(norm, n);
            cull &= (dp > 0.1);
            // finger in the wind threshold. This should not be 0 since that would account only for triangles viewed directly at the center of the screen
        }

        if (!cull) {
            newindices.push_back(ai);
            newindices.push_back(bi);
            newindices.push_back(ci);
            vtx[ai].isUsed = true;
            vtx[bi].isUsed = true;
            vtx[ci].isUsed = true;
        }
        else {
            ++culledTri;
        }
    }
    m_indices = std::move(newindices);
    m_indicesCount = (int)m_indices.size();


    cout << "Culled " << culledTri << "/" << totalTri << " = " << ((float)culledTri / totalTri * 100.0) << endl;
}

void Mesh::clearUsed()
{
    if (m_sharedGeom.get() != nullptr)
        m_sharedGeom->clearUsed();
    for(auto& sub: m_sub)
        sub.clearUsed();
}


void Mesh::cullFaces(const vector<Vec3>& possibleEyes)
{
    for(auto& sub: m_sub) {
        sub.cullFaces(possibleEyes, m_sharedGeom.get());
    }
}



void SubMesh::removeField(int sem, int index)
{
    if (m_isSharedGeom)
        return;
    auto vtxFlag = vtxFlagFromSem(sem, index);
    CHECK( checkFlag(m_hasEntries, vtxFlag), "Mesh does not have semantic " << semanticName(sem) << " index=" << index);
    // first remove it from the entries list
    VtxEntry removedEntry;
    int foundInBind = -1;
    for(int bindIndex = 0; bindIndex < m_entries.size(); ++bindIndex)
    {
        auto& bind = m_entries[bindIndex];
        int curOffset = 0; // offset in the buffer, for updating the entries that remain
        bool found = false;
        int entryIndex = 0;
        while(entryIndex < bind.e.size())
        {
            auto& entry = bind.e[entryIndex];
            if (entry.sem == sem && entry.index == index) {
                CHECK(!found, "Found same semantic twice??");
                removedEntry = entry;
                removedEntry.entryChunk->detach(); // remove chunk, removes itself from the parent
                bind.e.erase(bind.e.begin() + entryIndex);
                found = true;
                foundInBind = bindIndex;
                // don't increment entryIndex since we just removed one
                continue;
            }
            if (entry.offset != curOffset)
                entry.offset = curOffset; // need to update offsets of subsequent entries after we removed one
            curOffset += typeSize(entry.type);
            ++entryIndex;
        }
        if (found) {
            bind.entriesSize -= typeSize(removedEntry.type); // TBD serialize this
        }
    }
    CHECK(foundInBind != -1, "Did not find semantic " << semanticName(sem) << " " << sem << " in mesh"); // should not happen since we checked above

    m_hasEntries &= ALL_BUT(vtxFlag);

    // if the buffer remains empty, delete it completely
    bool bufIsEmpty = m_entries[foundInBind].entriesSize == 0;
    if (bufIsEmpty)
    {
        CHECK(m_entries[foundInBind].e.size() == 0, "Unexpeceted size of entries vector"); // sanity
        m_entries[foundInBind].bufferChunk->detach(); //remove the buffer chunk
        m_entries.erase(m_entries.begin() + foundInBind); // remove the empty data about it
    }

    // now remove it from the buffer. each vertex holds its own portion of the big buffer
    for(auto& vi: m_vtx)
    {
        auto& buf = vi.selfBufs[foundInBind];
        buf.erase((size_t)removedEntry.offset, (size_t)typeSize(removedEntry.type));
        if (bufIsEmpty) {
            CHECK(buf.size() == 0, "Unexpeceted non empty buffer"); // sanity
            vi.selfBufs.erase(vi.selfBufs.begin() + foundInBind);
        }
    }
}

void Mesh::removeField(int sem, int index)
{
    for(auto& sub: m_sub)
        sub.removeField(sem, index);
    if (m_sharedGeom)
        m_sharedGeom->removeField(sem, index);
}

bool SubMesh::buffersNeedUnify() {
    return m_entries.size() > 1;
}

void SubMesh::unifyBuffers()
{
    if (m_entries.size() == 1)
        return; // nothing to unify
    CHECK(m_entries.size() >= 2, "emptry entries list?");
    // create new entries vector with the unified fields
    vector<VtxBind> newEntries;
    newEntries.push_back(m_entries[0]);
    auto& unifiedBind = newEntries.back();
    unifiedBind.e.clear(); // clear all entries so that the following loop adds them all
    uint curOffset = 0;
    for(int bindIndex = 0; bindIndex < m_entries.size(); ++bindIndex)
    {
        auto& bind = m_entries[bindIndex];
        for(const auto& entry: bind.e) {
            VtxEntry ecopy = entry;
            ecopy.source = 0; // everything is in bind 0 now
            ecopy.offset = curOffset;
            curOffset += typeSize(ecopy.type);
            unifiedBind.e.push_back(ecopy);
        }
        if (bindIndex > 0)
            bind.bufferChunk->detach(); // remove the data chunks of the binds after the first one
    }
    // the number of entry chunks did not change overall so there's no need to change entry chunks
    unifiedBind.entriesSize = curOffset;
    m_entries = newEntries;

    // now unify the buffers for every vertex
    for(auto& vi: m_vtx)
    {
        auto& buf = vi.selfBufs[0];
        for(int i = 1; i < vi.selfBufs.size(); ++i)
            buf += vi.selfBufs[i];
        CHECK(buf.size() == unifiedBind.entriesSize, "Unexpected entriesSize");
        vi.selfBufs.resize(1);
    }
}


void Mesh::unifyBuffers()
{
    for(auto& sub: m_sub)
        sub.unifyBuffers();
}

bool Mesh::buffersNeedUnify()
{
    bool res = false;
    for(auto& sub: m_sub)
        res = res || sub.buffersNeedUnify();
    return res;
}

