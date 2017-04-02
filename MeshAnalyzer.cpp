#include "MeshAnalyzer.h"
#include "NullStream.h"

MESH_ANALYZER_DLL_API IMeshAnalyzer* createMeshAnalyzer() {
    return new MeshAnalyzer();
}

void MeshAnalyzer::setLogOut(ostream* out) {
    m_logOut = out;
}

void MeshAnalyzer::parse(const string& filename) {
    m_mesh.parse(filename, m_logOut);
}
void MeshAnalyzer::parse(istream& infile) {
    m_mesh.parse(infile, m_logOut);
}

void MeshAnalyzer::save(const string& filename) {
    m_mesh.save(filename);
    // verify that the saved mesh can be loaded correctly
    Mesh rm;
    rm.parse(filename, m_logOut);
}
void MeshAnalyzer::save(ostream& outfile) {
    m_mesh.save(outfile);
}

Mesh* MeshAnalyzer::getMesh() {
	return &m_mesh;
}

// --------------

void Proc::getAfterStats(int *numVtx, int *sizeBytes) {
    *numVtx = m_mesh->countVtx() - m_countDupVtx;

    uint singleVtxSize = 0;
    for(const auto& bind: m_mesh->m_sub[0].m_entries) {
        singleVtxSize += bind.entriesSize;
    }

    *sizeBytes = m_mesh->m_rootChunk->size - m_countDupVtx * singleVtxSize;
}

bool Proc::genericCheckVtxDup(uint vflag) {
    // if the mesh doesn't have that flag, abort
    if (!checkFlag(m_mesh->gatheredEntries(), vflag))
        return false;
    if (m_mesh->hasEdgeList() || m_mesh->hasVertexAnim())
        return false;
    return true;
}


class UnifyByTanEpsilon : public Proc
{
public:
    virtual ~UnifyByTanEpsilon() {}
    virtual const char* getName() const {
        return "unify_by_tan_epsilon";
    }
    virtual const char* getDescription() const {
        return "Unify the mesh vertices by almost-identical 'tangent' vector value";
    }

    virtual bool prepare() {
        if (!genericCheckVtxDup(VF_TANGENT))
            return false;
        m_countDupVtx = m_mesh->dupsByTanEpsilon(0.0); // uses the default		
        return true;

    }
    virtual void run() {
        m_countDupVtx = m_mesh->dupsByTanEpsilon(0.0);
        m_mesh->dedup();
    }
};

class JustUnify : public Proc
{
public:
    JustUnify()
    {}
    virtual const char* getName() const {
        return "just_unify";
    }
    virtual const char* getDescription() const {
        return "Unify vertices without changing anything else";
    }

    virtual bool prepare() {
        m_countDupVtx = m_mesh->dupsExact(-1, 0);
        return true;
    }
    virtual void run() {
        m_countDupVtx = m_mesh->dupsExact();
        m_mesh->dedup();
    }

};

class RemoveFieldAndUnify : public Proc
{
public:
    RemoveFieldAndUnify(const string& name, int sem, int index) 
        : m_sem(sem), m_index(index)
    {
        m_name = "remove_" + name + "_and_unify";
        m_desc = "Remove the field " + name + " from all vertices and unify vertices that are exacty identical";
    }
    virtual const char* getName() const {
        return m_name.c_str();
    }
    virtual const char* getDescription() const {
        return m_desc.c_str();
    }    

    virtual bool prepare() {
        if (!genericCheckVtxDup(vtxFlagFromSem(m_sem, m_index)))
            return false;
        m_countDupVtx = m_mesh->dupsExact(m_sem, m_index);
        return true;
    }
    virtual void run() {
        m_mesh->removeField(m_sem, m_index);
        m_countDupVtx = m_mesh->dupsExact();
        m_mesh->dedup();
    }

private:
    string m_name, m_desc;
    int m_sem, m_index;
};

struct RemoveNormal : public RemoveFieldAndUnify {
    RemoveNormal() : RemoveFieldAndUnify("normal", VES_NORMAL, 0) {}
};
struct RemoveDiffuse : public RemoveFieldAndUnify {
    RemoveDiffuse() : RemoveFieldAndUnify("diffuse_color", VES_DIFFUSE, 0) {}
};
struct RemoveTangent : public RemoveFieldAndUnify {
    RemoveTangent() : RemoveFieldAndUnify("tangent", VES_TANGENT, 0) {}
};
struct RemoveBinormal : public RemoveFieldAndUnify {
    RemoveBinormal() : RemoveFieldAndUnify("binormal", VES_BINORMAL, 0) {}
};
struct RemoveTex0 : public RemoveFieldAndUnify {
    RemoveTex0() : RemoveFieldAndUnify("text_coordinate_0", VES_TEXTURE_COORDINATES, 0) {}
};
struct RemoveTex1 : public RemoveFieldAndUnify {
    RemoveTex1() : RemoveFieldAndUnify("text_coordinate_1", VES_TEXTURE_COORDINATES, 1) {}
};
struct RemoveTex2 : public RemoveFieldAndUnify {
    RemoveTex2() : RemoveFieldAndUnify("text_coordinate_2", VES_TEXTURE_COORDINATES, 2) {}
};
struct RemoveTex3 : public RemoveFieldAndUnify {
    RemoveTex3() : RemoveFieldAndUnify("text_coordinate_3", VES_TEXTURE_COORDINATES, 3) {}
};


class MergeBuffers : public Proc 
{
public:
    virtual ~MergeBuffers() {}
    virtual const char* getName() const {
        return "merge_vertex_buffers";
    }
    virtual const char* getDescription() const {
        return "In meshes that have more than a single vertex buffer, merge all bufferes into a single buffer. This has some performance gain.";
    }

    virtual bool prepare() {
        bool needMerge = false;
        for(const auto& s: m_mesh->m_sub) {
            if (s.m_entries.size() > 1)
                needMerge = true;
        }
        return needMerge;
    }
    virtual void run() {
        m_mesh->unifyBuffers();
    }
};


vector<IProc*>& MeshAnalyzer::analyze()
{
    if (m_procFactory.m_names.empty()) {
#ifdef MESH_TOOL
        m_procFactory.add<UnifyByTanEpsilon>();
        m_procFactory.add<JustUnify>();
#endif
        m_procFactory.add<RemoveNormal>();
        m_procFactory.add<RemoveDiffuse>();
        m_procFactory.add<RemoveTangent>();
        m_procFactory.add<RemoveBinormal>();
        m_procFactory.add<RemoveTex0>();
        m_procFactory.add<RemoveTex1>();
        m_procFactory.add<RemoveTex2>();
        m_procFactory.add<RemoveTex3>();
        m_procFactory.add<MergeBuffers>();
    }

    for(const auto& name: m_procFactory.m_names) {
        shared_ptr<Proc> p( m_procFactory.create(name) );
        p->setMesh(&m_mesh);
        if (p->prepare())
            m_procs.push_back(p);
    }
    for(const auto& p: m_procs)
        m_iprocs.push_back(p.get());


    // ** check warnings
    // submesh count
    stringstream ss;
    if (m_mesh.m_sub.size() > 1) {
        ss << "WARNING: Mesh contains more than 1 submeshes (" << m_mesh.m_sub.size() << " Submeshes). Make sure this is ok\n";
    }
    // tex coord count
    uint gatheredEntries = m_mesh.gatheredEntries();
    int texChans = (checkFlag(gatheredEntries, VF_TEXCOORD0) ? 1:0) + (checkFlag(gatheredEntries, VF_TEXCOORD1) ? 1:0)
                 + (checkFlag(gatheredEntries, VF_TEXCOORD2) ? 1:0) + (checkFlag(gatheredEntries, VF_TEXCOORD3) ? 1:0);
    if (texChans > 1) {
        ss << "WARNING: Mesh contains more than 1 texture coordinate channel (" << texChans << " channels). Make sure this is ok\n";
    }
    
    if (m_mesh.hasVertexAnim()) {
        ss << "WARNING: Mesh contains veretx animation. This will prevent unifying vertices\n";
    }
    if (m_mesh.hasEdgeList()) {
        ss << "WARNING: Mesh has edge-list information. This will prevent unifying vertices\n";
    }

    m_warnings = ss.str();
    return m_iprocs;
}


void MeshAnalyzer::getStats(int *numVtx, int *sizeBytes) 
{
    *numVtx = m_mesh.countVtx();
    *sizeBytes = m_mesh.m_rootChunk->size;
}
void MeshAnalyzer::runProc(const string& name) 
{
    shared_ptr<Proc> p( m_procFactory.create(name) );
    p->setMesh(&m_mesh);
    p->run();
}

