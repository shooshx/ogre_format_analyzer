#pragma once

#include <string>
#include <vector>

#ifdef _WINDLL
  #ifdef COMPILING_MESH_ANALYZER
    #define MESH_ANALYZER_DLL_API __declspec(dllexport)
  #else
    #define MESH_ANALYZER_DLL_API __declspec(dllimport)
  #endif
#else
  #define MESH_ANALYZER_DLL_API
#endif

class Mesh;


class MESH_ANALYZER_DLL_API IProc
{
public:
    virtual ~IProc() {}
    virtual const char* getName() const = 0;
    virtual const char* getDescription() const = 0;
    virtual void getAfterStats(int *numVtx, int *sizeBytes) = 0;
};

class MESH_ANALYZER_DLL_API  IMeshAnalyzer
{
public:
    virtual ~IMeshAnalyzer() {}
    virtual void parse(const std::string& filename) = 0;
    virtual void parse(std::istream& stream) = 0;
    
    virtual std::vector<IProc*>& analyze() = 0;
    virtual const char* getMessages() const = 0;
    virtual void getStats(int *numVtx, int *sizeBytes) = 0;
    virtual void runProc(const std::string& name) = 0;
    virtual void setProcEpsilon(float epsilon) = 0; // for 'unify_by_tan_epsilon' proc which needs an epsilon value. use 0.0 to use the default (which is 0.2)
    
    virtual void save(const std::string& filename) = 0;

    // call with either &cout or nullptr which is the default
    virtual void setLogOut(std::ostream* out) = 0;
	virtual Mesh* getMesh() = 0;

};

MESH_ANALYZER_DLL_API IMeshAnalyzer* createMeshAnalyzer(); 

