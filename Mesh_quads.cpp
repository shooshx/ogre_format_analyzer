#include "Mesh.h"
#include <cmath>
#include <cfloat>
#include <fstream>

#include "QuadGrid.h"

// TBD
// - remove tangets from the meshes
// - remove diff color
// - remove texture coordinate

// NOT working well since there is the same triangle in both orientations... TBD
// for some reason some triangles appear twice...
void Mesh::removeDupTri()
{
    int removedTri = 0, totalTri = 0;
    for(auto& sub: m_sub)
    {
        vector<uint> newindices;
        newindices.reserve(sub.m_indices.size());

        set<tuple<int,int,int>> m_tri; // indexes ordered so they will be unique
        for(int i = 0; i < sub.m_indices.size(); i += 3)
        {
            int a = sub.m_indices[i], b = sub.m_indices[i+1], c = sub.m_indices[i+2];
            ++totalTri;
            int mini = std::min(a, std::min(b, c));
            int maxi = std::max(a, std::max(b, c));
            int midi = -1;
            if (a == mini && b == maxi)
                midi = c;
            else if (a == mini && c == maxi)
                midi = b;
            else
                midi = a;
            auto k = make_tuple(mini, midi, maxi);
            auto it = m_tri.find(k);
            if (it != m_tri.end()) {
                ++removedTri;
                continue;
            }
            m_tri.insert(k);

            newindices.push_back(a);
            newindices.push_back(b);
            newindices.push_back(c);
        }

        sub.m_indices = std::move(newindices);
        sub.m_indicesCount = (int)sub.m_indices.size();
    }
    cout << "Duplicate triangles removed=" << removedTri << "/" << totalTri << endl;
}


void Mesh::markUsedVertices()
{
    for(auto& sub: m_sub)
    {
        vector<VtxInfo>& vtx = sub.m_isSharedGeom ? m_sharedGeom->m_vtx : sub.m_vtx;
        for(int i = 0; i < sub.m_indices.size(); i += 3)
        {
            int ai = sub.m_indices[i], bi = sub.m_indices[i+1], ci = sub.m_indices[i+2];
            vtx[ai].isUsed = true;
            vtx[bi].isUsed = true;
            vtx[ci].isUsed = true;
        }
    }
}


vector<float> Mesh::getPlaneHeights() const
{
    vector<pair<float, int>> heights; // map height of flat triangle to count of triangles
    for(auto& sub: m_sub)
    {
        const vector<VtxInfo>& vtx = sub.m_isSharedGeom ? m_sharedGeom->m_vtx : sub.m_vtx;


        for (int i = 0; i < sub.m_indices.size(); i += 3)
        {
            int ai = sub.m_indices[i];
            int bi = sub.m_indices[i+1];
            int ci = sub.m_indices[i+2];

            Vec3 a = vtx[ai].pos;
            Vec3 b = vtx[bi].pos;
            Vec3 c = vtx[ci].pos;

            // triangles that are flat and that are big enough to be a part of a quad
            if (a.y == b.y && b.y == c.y && std::abs(a.x - b.x) > 10.0) {
                bool found = false;
                for(auto& p: heights) {
                    if (std::abs(p.first - a.y) < 0.001) {
                        p.second++;
                        found = true;
                        break;
                    }
                }
                if (!found)
                    heights.push_back(make_pair(a.y, 1));
            }
        }
    }

    vector<float> res;
    for(auto& p: heights) {
        if (p.second > 20)
            res.push_back(p.first);
    }

    return res;
}


bool epEq(float a, float b) {
    return (std::abs(a-b) < 0.001);
}

void SubMesh::extractQuads(float height, SubMesh* sharedGeom, vector<Quad2D>* outQuads)
{
    int countIdx = (int)m_indices.size();

    vector<VtxInfo>& vtx = m_isSharedGeom ? sharedGeom->m_vtx : m_vtx;

    map<pair<int,int>, QuadIndex> quadsInds; // map diagonal d1-d2 to the quad

    for (int i = 0; i < countIdx; i += 3)
    {
        int ai = m_indices[i];
        int bi = m_indices[i+1];
        int ci = m_indices[i+2];

        Vec3 a = vtx[ai].pos;
        Vec3 b = vtx[bi].pos;
        Vec3 c = vtx[ci].pos;

        if (!epEq(a.y, height) || !epEq(b.y, height) || !epEq(c.y, height))
            continue;
        int d1 = -1, d2 = -1, dex = -1;

        // find the diagonal pair, either a-b, b-c or c-a, and make sure the others are axis aligned
        // d1-d2 is the diagonal, dex is the extra vertex of the triangle
        // a-b
        if (a.x != b.x && a.z != b.z) {
            if (! ((a.x == c.x && b.z == c.z) || (a.z == c.z && b.x == c.x)) )
                continue; // not an axis aligned
            d1 = ai; d2 = bi; dex = ci;
        }
            // b-c
        else if (b.x != c.x && b.z != c.z) {
            if (! ((b.x == a.x && c.z == a.z) || (b.z == a.z && c.x == a.x)) )
                continue; // not an axis aligned
            d1 = bi; d2 = ci; dex = ai;
        }
            // c-a
        else if (c.x != a.x && c.z != a.z) {
            if (! ((c.x == b.x && a.z == b.z) || (c.z == b.z && a.x == b.x)) )
                continue; // not an axis aligned
            d1 = ci; d2 = ai; dex = bi;
        }
        else {
            continue;
        }
        // make the one with lower x in d1 to have a defined order in the pair
        if (vtx[d1].pos.x > vtx[d2].pos.x)
            std::swap(d1, d2);

        // check if we've seen this quad
        auto k = make_pair(d1, d2);
        auto it = quadsInds.find(k);
        if (it == quadsInds.end()) {
            quadsInds[k] = QuadIndex{d1, d2, dex, -1, i, -1};
        }
        else {
            CHECK(it->second.dex2 == -1, "Quad was already filled??");
            it->second.dex2 = dex;
            it->second.t2 = i;
        }
    }

    // output quads
    for(const auto& qip: quadsInds)
    {
        const QuadIndex& qi = qip.second;
        if (qi.dex2 == -1) // unpaired triangle
            continue;
        Vec3 d1 = vtx[qi.d1].pos;
        Vec3 d2 = vtx[qi.d2].pos;

        outQuads->push_back( Quad2D{ d1.x, d1.z, d2.x, d2.z, qi} );
    }

}


static void quadsToObj(vector<Quad2D>& quads)
{
    ofstream ofs("/Users/shyshalom/Projects/tmp4/quadsUnify/test1.obj");
    if (!ofs.good())
        return;

    int ind = 1;
    for(const auto& q: quads) {
        ofs << "v " << q.x1 << " 0 " << q.z1 << "\n";
        ofs << "v " << q.x2 << " 0 " << q.z1 << "\n";
        ofs << "v " << q.x2 << " 0 " << q.z2 << "\n";
        ofs << "v " << q.x1 << " 0 " << q.z2 << "\n";
        ofs << "f " << ind << " " << ind+1 << " " << ind+2 << "\n";
        ofs << "f " << ind << " " << ind+2 << " " << ind+3 << "\n";
        ind += 4;
    }
}


static void gridDim(const vector<Quad2D>& quads, int* height, int* width, Vec2* outmin, Vec2* outDelta)
{
    Vec2 min{FLT_MAX, FLT_MAX}, max{FLT_MIN, FLT_MIN};
    auto firstq = quads[0];
    // used for making sure that the size of all quads is uniform at this point
    float stdDx = std::abs(firstq.x1 - firstq.x2), stdDz = std::abs(firstq.z1 - firstq.z2);
    for(const auto& q: quads) {
        float dx = std::abs(q.x1 - q.x2), dz = std::abs(q.z1 - q.z2);
        if (dx != stdDx || dz != stdDz) {
            cout << "Mesh::extractQuads different size quads! " << dx << "," << dz << endl;
            return;
        }
        min.minimize(q.x1, q.z1);
        min.minimize(q.x2, q.z2);
        max.maximize(q.x1, q.z1);
        max.maximize(q.x2, q.z2);
    }
    float gdx = max.x - min.x;
    float gdz = max.y - min.y;
    float w = gdx / stdDx;
    float h = gdz / stdDz;
    *width = (int)std::ceil(w);
    *height = (int)std::ceil(h);
    *outmin = min;
    *outDelta = Vec2{stdDx, stdDz};
};

static void quadsToGrid(const vector<Quad2D>& quads, const Vec2& min, const Vec2& delta, QuadGrid* grid)
{
    for(const auto& q: quads) {
        // middle of the quad
        float mx = (q.x1 + q.x2) * 0.5;
        float my = (q.z1 + q.z2) * 0.5;
        // normalize to the grid
        int imx = (int)std::floor((mx - min.x) / delta.x);
        int imy = (int)std::floor((my - min.y) / delta.y);

        auto& cell = grid->get(imx, imy);
        cell.initv = true; // will not change during grid solve
        cell.tinf = q.tinf;

    }

}

// find all the triangle pair that make an axis aligned quad and add it to the grid
bool Mesh::extractQuads(float quadsHeight, QuadGrid* grid)
{
    vector<Quad2D> quads;
    for(auto& sub: m_sub)
        sub.extractQuads(quadsHeight, m_sharedGeom.get(), &quads);

    if (quads.size() < 2) {
        cout << "Mesh::extractQuads no quads! " << quadsHeight << endl;
        return false;
    }
    //cout << "found quads " << quads.size() << endl;

    quadsToObj(quads);

    int height = 0, width = 0;
    Vec2 min, delta;
    gridDim(quads, &height, &width, &min, &delta);

    grid->init(width, height, quadsHeight);
    quadsToGrid(quads, min, delta, grid);
    return true;
}

#define CHECK_PUSH_BACK(into, v) CHECK(v != -1, "unexpected index"); into.push_back(v)

// from the grid, take the m_squares that span more than one cell, and replace the triengles with bigger triangles
void Mesh::replaceQuads(const QuadGrid& grid)
{
    CHECK(m_sub.size() == 1, "more than 1 submesh not supported");
    SubMesh& sub = m_sub[0];
    int markedTri = 0;
    // remove all the triangles that are in the grid, mark them first
    for(int y = 0; y < grid.m_height; ++y) {
        for(int x = 0; x < grid.m_width; ++x) {
            const auto& cell = grid.get(x, y);
            if (!cell.initv)
                continue;
            CHECK(cell.tinf.t1 != -1 && cell.tinf.t2 != -1, "invalid triangles");
            sub.m_indices[cell.tinf.t1] = UINT_MAX;
            sub.m_indices[cell.tinf.t2] = UINT_MAX;
            markedTri += 2;
        }
    }
    // filter them out
    int filteredTri = 0;
    vector<uint> newindices;
    for(int i = 0; i < sub.m_indices.size(); i += 3)
    {
        int ai = sub.m_indices[i];
        if (ai == UINT_MAX) {
            filteredTri += 1;
            continue;
        }
        newindices.push_back(ai);
        newindices.push_back(sub.m_indices[i+1]);
        newindices.push_back(sub.m_indices[i+2]);
    }

    //cout << "Tri: marked=" << markedTri << " filter=" << filteredTri << " / " << sub.m_indices.size() / 3 << endl;

    // now add the new triangles of the replacement m_squares from the grid
    for(const auto& sq: grid.m_squares)
    {
        // from what cells to take the indices
        const auto& cell_dex2 = grid.get(sq.x, sq.y);
        const auto& cell_dex1 = grid.get(sq.x + sq.width-1, sq.y + sq.height-1);
        const auto& cell_d1 = grid.get(sq.x, sq.y + sq.height-1);
        const auto& cell_d2 = grid.get(sq.x + sq.width-1, sq.y);

        CHECK_PUSH_BACK(newindices, cell_d1.tinf.d1);
        CHECK_PUSH_BACK(newindices, cell_dex1.tinf.dex1);
        CHECK_PUSH_BACK(newindices, cell_d2.tinf.d2);
        CHECK_PUSH_BACK(newindices, cell_d1.tinf.d1);
        CHECK_PUSH_BACK(newindices, cell_d2.tinf.d2);
        CHECK_PUSH_BACK(newindices, cell_dex2.tinf.dex2);
       // break;
    }


    sub.m_indices = std::move(newindices);
    sub.m_indicesCount = (int)sub.m_indices.size();


}