#pragma once

#include <iostream>

// wrap an istream
class Deserializer
{
public:
    Deserializer(istream& ins) : m_in(ins)
    {
        m_in.seekg(0, ios_base::end);
        m_filesize = (int)m_in.tellg();
        m_in.seekg(0);
    }

    template<typename T>
    T read() {
        T v;
        m_in.read((char*)&v, sizeof(T));
        CHECK(m_in.good(), "failed reading from stream");
        return v;
    }

    ubyte read8() {
        return read<ubyte>();
    }
    bool readBool() {
        return read<ubyte>() != 0;
    }
    ushort read16() {
        return read<ushort>();
    }
    uint read32() {
        return read<uint>();
    }
    float read32f() {
        return read<float>();
    }

    string readStr() {
        string s;
        char c;
        while (true) {
            m_in.read(&c, 1);
            if (c == 0x0a)
                break;
            s += c;
        }
        return s;
    }

    ushort chunkHeader(int* chunkLen) {
        m_chunkStart = (uint)m_in.tellg();
        auto id = read16();
        *chunkLen = read32();
        return id;
    }
    int consumedChunkLen() { // how many bytes from the current chunk we consumed from the stream
        return (int)m_in.tellg() - m_chunkStart;
    }
    string consumedBuf() {
        int offset = (int)m_in.tellg();
        m_in.seekg(m_chunkStart);
        string buf;
        int sz = offset - m_chunkStart;
        buf.resize(sz);
        m_in.read((char*)buf.data(), sz);
        return buf;
    }

    bool eof() {
        auto offset = m_in.tellg();
        return m_in.eof() || ((int)offset == m_filesize);
    }
    int remainSize() {
        return (int)(m_filesize - m_in.tellg());
    }
    int tellg() {
        return (int)m_in.tellg();
    }

    string getSubBuf(int start, int end) {
        int cur = (int)m_in.tellg();
        string buf;
        buf.resize(end - start);
        m_in.seekg(start);
        m_in.read((char*)buf.data(), end - start);
        m_in.seekg(cur);
        return buf;
    }

private:
    istream& m_in;

    int m_chunkStart = 0;
    int m_filesize = 0;
};


// wrap an ostream for output
class Serializer
{
public:
    Serializer(ostream& outs) : m_out(outs)
    {}

    void write(const string& s) {
        if (s.size() == 0)
            return;
        m_out.write(s.c_str(), s.size());
    }

    void write(uint offset, const string& s) {
        CHECK(s.size() >= offset, "Write negative size");
        if (s.size() - offset == 0)
            return;
        m_out.write(s.c_str() + offset, s.size() - offset);
    }

    void write16(ushort v) {
        m_out.write((const char*)&v, sizeof(v));
    }
    void write32(uint v) {
        m_out.write((const char*)&v, sizeof(v));
    }
    void write32f(float v) {
        m_out.write((const char*)&v, sizeof(v));
    }
    void writeBool(bool bv) {
        ubyte v = (bv ? 1:0);
        m_out.put(v);
    }
    void writeStr(const string& s) {
        write(s);
        m_out.put(0x0a);
    }

private:
    ostream& m_out;
};
