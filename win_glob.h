#pragma once

#include <Windows.h>
#include <string>
#include <vector>

typedef struct {
    size_t   gl_pathc;    /* Count of paths matched so far  */
    const char   **gl_pathv;    /* List of matched pathnames.  */
    size_t   gl_offs;     /* Slots to reserve in gl_pathv.  */
} glob_t;

int glob(const char *pattern, int flags, int (*errfunc) (const char *epath, int eerrno), glob_t *pglob)
{
    WIN32_FIND_DATAA fdata;
    HANDLE fh = FindFirstFileA(pattern, &fdata);
    if (fh == INVALID_HANDLE_VALUE)
        return 1;

    static std::vector<std::string> res;
    static std::vector<const char*> pathv;
    res.clear();
    pathv.clear();

    std::string dir(pattern);
    auto lastSlash = dir.find_last_of ("/\\");
    if (lastSlash != std::string::npos)
        dir = dir.substr(0, lastSlash);
    else
        dir.clear();

    pglob->gl_pathc = 0;
    do {
        std::string name(fdata.cFileName);
        if (name == "." || name == "..")
            continue;
        ++pglob->gl_pathc;
        res.push_back(dir + "\\" + name);
    } while (FindNextFileA(fh, &fdata) != 0);

    for(const auto& s: res)
        pathv.push_back(s.c_str());
    pglob->gl_pathv = &pathv[0];
    pglob->gl_offs = 0;

    FindClose(fh);
    return 0;

}
