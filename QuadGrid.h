#pragma once
#include <vector>
#include <iostream>

#include "Except.h"
#include "Mesh.h"

using namespace std;

class QuadGrid
{
public:
    struct Cell {
        bool v = false; // is it on
        bool initv = false;
        QuadIndex tinf;
        int sqind = -1;
    };

    void init(int w, int h, float ylevel) {
        m_width = w;
        m_height = h;
        m_ylevel = ylevel;
        m_data.clear();
        m_data.resize(m_width * m_height);
        for(auto& c: m_data)
            c.tinf.init();
    }

    Cell& get(int x, int y) {
        CHECK(x >= 0 && x < m_width && y >= 0 && y < m_height, "Cell out of range");
        return m_data[x + y*m_width];
    }
    const Cell& get(int x, int y) const {
        CHECK(x >= 0 && x < m_width && y >= 0 && y < m_height, "Cell out of range");
        return m_data[x + y*m_width];
    }

                   //                            1         2         3         4
    void print() { //                            0123456789012345678901234567890
        static const string markers = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        for(int y = 0; y < m_height; ++y) {
            for(int x = 0; x < m_width; ++x) {
                const auto& c = get(x,y);
                if (c.v)
                    cout << "*";
                else if (c.sqind != -1) {
                    if (c.sqind < markers.size())
                        cout << markers[c.sqind];
                    else
                        cout << "$";
                }
                else
                    cout << " ";
            }
            cout << "\n";
        }

    }

    int m_width = 0;
    int m_height = 0;
    float m_ylevel = NAN; // for debugging
    vector<Cell> m_data;

    struct Square {
        Square(int _x, int _y, int _w, int _h) : x(_x), y(_y), width(_w), height(_h) {}
        int x, y, width, height;
    };
    void add(const Square &square, int ind);
    void initMarks();
    Square candidate(int c);
    void reinit();
    void solve();

    vector<Square> m_squares;

    int mark[8][2];

};
