#pragma once

class NullBuffer : public std::streambuf {
public:
    int overflow(int c) { return c; }
};


inline std::ostream& null_stream() {
    static NullBuffer null_buffer;
    static std::ostream null_s(&null_buffer);
    return null_s;
}