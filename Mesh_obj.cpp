#include "Mesh.h"
#include <fstream>

using namespace std;



void Mesh::exportObj(const string& filename)
{
    CHECK(m_sub.size() == 1, "Saving to obj supported for only 1 submesh");

    ofstream outf(filename, ios::binary);
    CHECK(outf.good(), "Failed reading file `" << filename << "`");

    SubMesh *sub = &m_sub[0];
    SubMesh *geom = sub;
    if (sub->m_isSharedGeom) {
        geom = m_sharedGeom.get();
    }
    for(const auto& inf: geom->m_vtx)
    {
        outf << "v " << inf.pos.x << " " << inf.pos.y << " " << inf.pos.z << "\n";
    }

    for(int i = 0; i < sub->m_indices.size(); i += 3)
    {
        outf << "f " << sub->m_indices[i]+1 << " " <<  sub->m_indices[i + 1]+1 << " " << sub->m_indices[i + 2]+1 << "\n";
    }

}
