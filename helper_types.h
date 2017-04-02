#pragma once
#include <exception>
#include <math.h>

using namespace std;


struct Vec3 {
    float x,y,z;

    void set(ushort type, float* v) {
        CHECK(type == VET_FLOAT3, "expected FLOAT3 type");
        x = v[0]; y = v[1]; z = v[2];
    }
    void clear() {
        x = 0.0f; y = 0.0f; z = 0.0f;
    }

    static Vec3 crossProd(const Vec3 &a, const Vec3& b)
    {
        return Vec3{a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x};
    }

    static float dotProd(const Vec3 &a, const Vec3& b)
    {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    }

    float length() {
        return sqrt(x*x + y*y + z*z);
    }

    void normalize() {
        double ilen = 1.0 / sqrt(x*x + y*y + z*z);
        x *= ilen; y *= ilen; z *= ilen;
    }
    void normalize(float len) {
        double ilen = 1.0 / len;
        x *= ilen; y *= ilen; z *= ilen;
    }

};
inline bool operator==(const Vec3& a, const Vec3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline bool operator!=(const Vec3& a, const Vec3& b) {
    return a.x != b.x || a.y != b.y || a.z != b.z;
}
inline bool operator<(const Vec3& a, const Vec3& b) {
    if (a.x != b.x)
        return a.x < b.x;
    if (a.y != b.y)
        return a.y < b.y;
    if (a.z != b.z)
        return a.z < b.z;
    return false;
}
inline ostream& operator<<(ostream& ostr, const Vec3& a) {
    ostr << "(" << a.x << ", " << a.y << ", " << a.z << ")";
    return ostr;
}
inline Vec3 operator-(const Vec3& a, const Vec3& b) {
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}



struct Vec2 {
    float x,y;

    void set(ushort type, float* v) {
        CHECK(type == VET_FLOAT2, "expected FLOAT2 type");
        x = v[0]; y = v[1];
    }

    void maximize(float vx, float vy) {
        if (vx > x)
            x = vx;
        if (vy > y)
            y = vy;
    }
    void minimize(float vx, float vy) {
        if (vx < x)
            x = vx;
        if (vy < y)
            y = vy;
    }

};
inline bool operator==(const Vec2& a, const Vec2& b) {
    return a.x == b.x && a.y == b.y;
}
inline bool operator!=(const Vec2& a, const Vec2& b) {
    return a.x != b.x || a.y != b.y;
}
inline bool operator<(const Vec2& a, const Vec2& b) {
    if (a.x != b.x)
        return a.x < b.x;
    if (a.y != b.y)
        return a.y < b.y;
    return false;
}
inline ostream& operator<<(ostream& ostr, const Vec2& a) {
    ostr << "(" << a.x << ", " << a.y << ")";
    return ostr;
}


inline bool checkFlag(uint v, uint f) {
    return ((v & f) == f);
}

