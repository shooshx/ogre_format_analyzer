#pragma once

#include "IMeshAnalyzer.h"
#include "Mesh.h"
#include "Except.h"
#include <functional>

using namespace std;

class Proc : public IProc
{
public:
    virtual ~Proc() {}
    virtual void getAfterStats(int *numVtx, int *sizeBytes);

    // returns false if this procedure is not available
    virtual bool prepare() = 0;
    virtual void run() = 0;

    void setMesh(Mesh* m) {
        m_mesh = m;
    }

protected:
    bool genericCheckVtxDup(uint vflag);

protected:
    Mesh* m_mesh = nullptr;
    int m_countDupVtx = 0;
};

class Factory {
public:
    template<typename T>
    void add() {
        T obj;
        m_f[obj.getName()] = []()->Proc* { return new T; };
        m_names.push_back(obj.getName());
    }
    Proc* create(const string& name) {
        auto it = m_f.find(name);
        CHECK(it != m_f.end(), "Unknown proc " << name);
        return it->second();
    }
    map<string, function<Proc*()>> m_f;
    vector<string> m_names;
};



class MeshAnalyzer : public IMeshAnalyzer
{
public:
    virtual ~MeshAnalyzer() {}
    virtual void parse(const string& filename);
    virtual void parse(istream& stream);
    
    virtual vector<IProc*>& analyze();
    virtual const char* getMessages() const {
        return m_warnings.c_str();
    }
    virtual void getStats(int *numVtx, int *sizeBytes);
    virtual void runProc(const string& name);
    virtual void setProcEpsilon(float epsilon) { 
        m_mesh.setEpsilon(epsilon);
    }
    
    virtual void save(const string& filename);
    virtual void save(ostream& stream);

    virtual void setLogOut(ostream* out);
	virtual Mesh* getMesh();

private:
    vector<shared_ptr<Proc>> m_procs;
    vector<IProc*> m_iprocs;

    Factory m_procFactory;

    Mesh m_mesh;
    string m_warnings;
    ostream* m_logOut = nullptr;
};