#pragma once
#include <sstream>
#include <exception>
#include <string>

class Exception : public std::exception
{
public:
    Exception(const std::string& s) :m_s(s) {}
    virtual ~Exception() {}
#ifdef _WIN32
    virtual const char* what() const override {
#else
    virtual const char* what() const noexcept override{
#endif
        return m_s.c_str();
    }
private:
    std::string m_s;
};

#define CHECK(cond, strm) do { if (!(cond)) { std::stringstream ss; ss << strm; throw Exception(ss.str()); } } while(0)
