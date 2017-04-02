#include <iostream>
#include <string>
#include <string.h>

#ifdef _WIN32
  #include "win_glob.h"
#else
  #include <glob.h>
#include <fstream>

#endif

#include "Except.h"
#include "Mesh.h"
#include "NullStream.h"
#include "QuadGrid.h"


// TBD:

// endians?
// reaper
// when removing indices:
// V   bone assignments with vertex index
// X   edge vertex index ??
// X   vertex animation cull  0xd000
// X shared vertices?
// X int vertex types
// V fix mesh and submesh sizes properly


using namespace std;

ostream* g_out = NULL;
ostream& out() {
    return *g_out;
}


static string basename(const string& path) {
    auto lastSlash = path.rfind("/");
    if (lastSlash != string::npos)
        return path.substr(lastSlash);
    return path;
}


class ObjMesh
{
public:
    ObjMesh(const string& outname) {
        m_outf.open(outname);
        CHECK(m_outf.good(), "failed open");

    }
    void addFile(const string& s);

    ofstream m_outf;
    int m_curOffset = 0;
};

void ObjMesh::addFile(const string& s)
{
    ifstream ifs(s);
    if (!ifs.good())
        return;
    int vcount = 0;
    while (!ifs.eof()) {
        string line;
        std::getline(ifs, line);
        istringstream ss(line);
        char cmd;
        ss >> cmd;
        if (cmd == 'v') {
            ++vcount;
            m_outf << line << "\n";
        }
        else if (cmd == 'f') {
            int a, b, c;
            ss >> a >> b >> c;
            m_outf << "f " << a + m_curOffset << " " << b + m_curOffset << " " << c + m_curOffset << "\n";
        }
        else {
            cout << "bad cmd `" << cmd << "`";
        }
    }
    m_curOffset += vcount;
}

void unifyObjs(const string& path)
{
    string filename;
    filename = path + "/t_*.mesh.obj";

    glob_t globbuf;
    glob(filename.c_str(), 0, NULL, &globbuf);

    ObjMesh omesh("./ater.obj");
    for(int i = 0; i < globbuf.gl_pathc; ++i)
    {
        filename = globbuf.gl_pathv[i];
        cout << i << ", " << filename << ",   " << endl;

        omesh.addFile(filename);
    }

}

int convertToObj(const string& dir, const string& outfile)
{

    string filename = dir + "/t_*.mesh";

    ObjMesh omesh(outfile);

    glob_t globbuf;
    glob(filename.c_str(), 0, NULL, &globbuf);
    for(int i = 0; i < globbuf.gl_pathc; ++i)
    {
        filename = globbuf.gl_pathv[i];
        cout << i << ", " << filename << ",   " << endl;

        Mesh m;
        m.parse(filename, g_out);
        m.exportObj(filename + ".obj");

        omesh.addFile(filename + ".obj");
    }
    return 0;

}

string fname(const string& s) {
    auto p = s.rfind('/');
    if (p == string::npos)
        return s;
    return s.substr(p+1);
}

// convert test
int convertAllToObjec(const string& fromGlob, const string& outbase)
{
    g_out = &null_stream();
    //g_out = &cout;

    glob_t globbuf;
    glob(fromGlob.c_str(), 0, NULL, &globbuf);
    for(int i = 0; i < globbuf.gl_pathc; ++i)
    {
        string filename = globbuf.gl_pathv[i];

        string basename = fname(filename);
        string unzpath = outbase + "/" + basename;
        string cmd = "unzip " + filename + " -d " + unzpath;
        //system( cmd.c_str() );

        string allfile = outbase + "/" + basename + "_at.obj";

        convertToObj(unzpath, allfile);
    }

    return 0;
}


// optimizations
// V - unify vertices by epsilon tangent
// V - remove tangent and unify
// V - remove bitangent and unify
// - remove edges section
// - remove vertex animation section
// X - submeshes with same m_material



int analyzer_main(const string& filename, const string& outdir);
void printAnalyzeStats(const string& filename);

int main_print(const string& filename, bool allVtx)
{
    g_out = &cout;
    Mesh m;
    m.m_outAllVertices = allVtx;
    m.parse(filename, g_out);
    return 0;
}

// the two extreme eye forward vectors from
// Vector3(-0.40558, -0.819152, -0.40558)    normal view point
// Vector3(-0.122788, -0.984808, -0.122788)  most zoomed out
// Vector3(-0.612372, -0.5, -0.612372)       most zoomed in
// see commented out log line with CameraForward in InputCameraObject.cpp


#define TR_CULL_BACK 0x01
#define TR_UNIFY_QUADS 0x02
#define TR_REMOVE_DIFF 0x04
#define TR_REMOVE_TAN 0x08
#define TR_ALL 0xFF


int main_terrainProcess(const string& dir, const string& outdir, int actions)
{
    string filename = dir + "/t_*.mesh";

    // most zoomed out eye direction and normal eye direction
    vector<Vec3> eyes = { Vec3{-0.122788, -0.984808, -0.122788}, Vec3{-0.40558, -0.819152, -0.40558}, Vec3{-0.612372, -0.5, -0.612372} };

    int beforeTri = 0, afterTri = 0;

    glob_t globbuf;
    glob(filename.c_str(), 0, NULL, &globbuf);
    for(int i = 0; i < globbuf.gl_pathc; ++i)
    {
        filename = globbuf.gl_pathv[i];
        cout << i << ", " << filename << ",   " << endl;

        Mesh m;
        m.parse(filename, g_out);
        beforeTri += m.countTri();
        //m.removeDupTri();

        if (actions & TR_CULL_BACK)
            m.cullFaces(eyes);

        if (actions & TR_UNIFY_QUADS)
        {
            auto heights = m.getPlaneHeights();
            for(float h : heights)
            {
                QuadGrid grid;
                if (!m.extractQuads(h, &grid))
                    continue; // not enough quads found
                grid.solve();
                m.replaceQuads(grid);
            }
        }

        // remove unused vertices
        if ((actions & TR_CULL_BACK) || (actions & TR_UNIFY_QUADS))
        {
            m.clearUsed();
            m.markUsedVertices();
            m.dedup();
        }

        if (actions & TR_REMOVE_DIFF) {
            m.removeField(VES_DIFFUSE, 0);
        }
        if (actions & TR_REMOVE_TAN) {
            m.removeField(VES_TANGENT, 0);
        }

        if ((actions & TR_REMOVE_DIFF) || (actions & TR_REMOVE_TAN))
        {
            m.dupsExact(); // mark exact vertex duplicates
            m.dedup();
        }


        afterTri += m.countTri();

        string outfile = outdir + basename(filename);
        m.save(outfile);
        //m.exportObj(filename + "_uni.obj");

    }

    cout << "TerrainProcess  " << afterTri << "/" << beforeTri << " = " << (float)afterTri / beforeTri*100.0 << "% triangles survived" << endl;

    return 0;

}


int main_dirStats(string filename)
{
    //g_out = &cout;
    g_out = &null_stream();

    glob_t globbuf;
    glob(filename.c_str(), 0, NULL, &globbuf);

    cout << "Count=" << globbuf.gl_pathc << "`" << filename.c_str() << "`" << endl;
    try {
        for(int i = 0; i < globbuf.gl_pathc; ++i)
        {
            filename = globbuf.gl_pathv[i];

            printAnalyzeStats(filename);

        }
    }
    catch (const std::exception& e) {
        cerr << "ERROR: " << e.what() << endl;
        return 1;
    }
    return 0;
}


// command line inspector
int main(int argc, char* argv[])
{
    if (argc < 2) {
        cout << "Usage: ogre_format print <filename.mesh> [allvtx]\n" <<
                "       ogre_format optimize <filename.mesh> <output-folder>\n"
                "       ogre_format toobj <from-mission-dir> <to-file.obj>\n"
                "       ogre_format terrainProcess <in-dir> <out-dir>\n"
                        << endl;
        return 1;
    }

    string argv1 = argv[1];
    if (argc == 4 && strcasecmp(argv[1], "toobj") == 0) {
        return convertToObj(argv[2], argv[3]);
    }

    if (argc == 4 && strcasecmp(argv[1], "optimize") == 0) {
        return analyzer_main(argv[2], argv[3]);
    }

    if (argc >= 3 && strcasecmp(argv[1], "print") == 0) {
        bool allVtx = false;
        if (argc == 4)
            allVtx = (string(argv[3]) == "allvtx");
        return main_print(argv[2], allVtx);
    }

    if (argc >= 4 && strcasecmp(argv[1], "terrainProcess") == 0) {
        return main_terrainProcess(argv[2], argv[3], TR_ALL);
    }


    return main_dirStats(argv[1]);
};



