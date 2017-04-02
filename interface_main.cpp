#include <iostream>

#include "IMeshAnalyzer.h"
#include "MeshAnalyzer.h"

using namespace std;


static string basename(const string& path) {
    auto lastSlash = path.find_last_of("/\\");
    if (lastSlash != string::npos)
        return path.substr(lastSlash);
    return path;
}



int analyzer_main(const string& filename, const string& outdir)
{
    try {
        auto ma = createMeshAnalyzer();
        //ma->setLogOut(&cout);
        ma->parse(filename);
        int numVtx = 0, sizeBytes = 0;
        ma->getStats(&numVtx, &sizeBytes);
        cout << "VtxCount= " << numVtx << "  Size=" << sizeBytes << endl;

        auto vi = ma->analyze();
        cout << "Messages:\n" << ma->getMessages() << endl;

        for(auto* i: vi) {
            cout << "Name= " << i->getName() << endl;
            cout << "  Desc= " << i->getDescription() << endl;
            int thisNumVtx = 0, thisSizeBytes = 0;
            i->getAfterStats(&thisNumVtx, &thisSizeBytes);
            cout << "  VtxCount=" << thisNumVtx << " (" << (float)thisNumVtx / numVtx * 100.0 << "%)  Size=" << thisSizeBytes << " (" << (float)thisSizeBytes / sizeBytes * 100.0 << "%)" << endl;
            cout << endl;
        }

        cout << "RUNNING just_unify" << endl;
        ma->runProc("just_unify");

        cout << "RUNNING unify_by_tan_epsilon" << endl;
        ma->runProc("unify_by_tan_epsilon");
        cout << "RUNNING merge_vertex_buffers" << endl;
        ma->runProc("merge_vertex_buffers");

        string outpath = outdir + basename(filename);

        cout << "SAVING " << outpath << endl;
        ma->save(outpath);
    }
    catch(const std::exception& e) {
        cout << e.what() << endl;
    }
    return 0;
}

void printAnalyzeStats(const string& filename)
{
    try {
        auto ma = createMeshAnalyzer();
        ma->parse(filename);

        int numVtx = 0, sizeBytes = 0;
        ma->getStats(&numVtx, &sizeBytes);

        auto vi = ma->analyze();

        int tanNumVtx = 0, tanSizeBytes = 0;
        bool needMerge = false, hasTan = false;
        float numPerc = 0.0;
        for(auto* i: vi) {
            if (string(i->getName()) == "unify_by_tan_epsilon") {
                i->getAfterStats(&tanNumVtx, &tanSizeBytes);
                numPerc = ((float)tanNumVtx / numVtx)*100.0;
                hasTan = true;
            }
            if (string(i->getName()) == "merge_vertex_buffers") {
                needMerge = true;
            }
        }

        bool red = (hasTan && numPerc < 90.0) || needMerge;
        if (red)
            cout << "\x1b[1;31;40m";
        cout << filename << "  " << numVtx << "  " << tanNumVtx << "  " << numPerc << "  " << (needMerge ? "NEED-MERGE":"no-merge") << endl;
        if (red)
            cout << "\x1b[0m";

    }
    catch(const std::exception& e) {
        cout << e.what() << endl;
    }

}
