#include "QuadGrid.h"



using namespace std;




void QuadGrid::add(const Square &square, int ind) {
    for (int y = 0; y < square.height; ++y)
        for (int x = 0; x < square.width; ++x) {
            auto& c = get(square.x + x, square.y + y);
            CHECK(c.initv, "setting a cell that was not set");
            c.v = false;
            c.sqind = ind;
        }
    m_squares.push_back(square);
}



void QuadGrid::initMarks() {
    mark[0][0] = 0; mark[0][1] = 0;
    mark[1][0] = 0; mark[1][1] = 0;
    mark[2][0] = m_width - 1; mark[2][1] = 0;
    mark[3][0] = m_width - 1; mark[3][1] = 0;
    mark[4][0] = m_width - 1; mark[4][1] = m_height - 1;
    mark[5][0] = m_width - 1; mark[5][1] = m_height - 1;
    mark[6][0] = 0; mark[6][1] = m_height - 1;
    mark[7][0] = 0; mark[7][1] = m_height - 1;
}

#define MAX_TRI_RATIO (6.0)

// http://www.benzedrine.ch/cimpress2015.cpp
// http://www.benzedrine.ch/polygon-partition.html

QuadGrid::Square QuadGrid::candidate(int c)
{
    int x, y;

    x = mark[c][0]; y = mark[c][1];
    switch (c) {
    case 0:
        while (y < m_height) {
            if (get(x,y).v)
                goto found;
            if (++x == m_width) {
                y++;
                x = 0;
            }
        }
        break;
    case 1:
        while (x < m_width) {
            if (get(x,y).v)
                goto found;
            if (++y == m_height) {
                x++;
                y = 0;
            }
        }
        break;
    case 2:
        while (y < m_height) {
            if (get(x,y).v)
                goto found;
            if (--x == -1) {
                y++;
                x = m_width - 1;
            }
        }
        break;
    case 3:
        while (x >= 0) {
            if (get(x,y).v)
                goto found;
            if (++y == m_height) {
                x--;
                y = 0;
            }
        }
        break;
    case 4:
        while (y >= 0) {
            if (get(x,y).v)
                goto found;
            if (--x == -1) {
                y--;
                x = m_width - 1;
            }
        }
        break;
    case 5:
        while (x >= 0) {
            if (get(x,y).v)
                goto found;
            if (--y == -1) {
                x--;
                y = m_height - 1;
            }
        }
        break;
    case 6:
        while (y >= 0) {
            if (get(x,y).v)
                goto found;
            if (++x == m_width) {
                y--;
                x = 0;
            }
        }
        break;
    case 7:
        while (x < m_width) {
            if (get(x,y).v)
                goto found;
            if (--y == -1) {
                x++;
                y = m_height - 1;
            }
        }
        break;
    }
    return Square(0, 0, 0, 0);
found:
    mark[c][0] = x;
    mark[c][1] = y;
    int dx = (c >= 2 && c <= 5) ? -1 : 1;
    int dy = (c >= 4 && c <= 7) ? -1 : 1;
    int w = 0, h = 0;
    bool widen = true, heigten = true;
    while(widen || heigten)
    {
        if (widen)
        { // advance 1 in width
            w++;
            if (x + w * dx < 0 || x + w * dx >= m_width)
                widen = false;
        }
        if (heigten)
        { // advance 1 in width
            h++;
            if (y + h * dy < 0 || y + h * dy >= m_height)
                heigten = false;
        }

        if (widen) {
            for(int iy = 0; iy < h; ++iy) {
                if (!get(x + dx * w, y + dy * iy).v) {
                    widen = false;
                    break;
                }
            }
        }
        if (heigten) {
            for(int ix = 0; ix < w; ++ix) {
                if (!get(x + dx * ix, y + dy * h).v) {
                    heigten = false;
                    break;
                }
            }
        }
        if (heigten && widen) {
            // check the furthest point, if it's bad, don't heighen to it (could be either heighen or widen, I just chose this)
            if (!get(x + dx * w, y + dy * h).v) {
                heigten = false;
            }
        }

        if ((float)h/w > MAX_TRI_RATIO || (float)w/h > MAX_TRI_RATIO)
            break; // don't make slighter quads

    }

#if 0
    int i = 0, j = 0;
    do {
        i++;
        if (x + i * dx < 0 || x + i * dx >= m_width || y + i * dy < 0 || y + i * dy >= m_height)
            break;
        for (j = 0; j <= i; ++j)
            if (!get(x + dx * i, y + dy * j).v || !get(x + dx * j,y + dy * i).v)
                break;
    } while (j > i);
    w = i; h = i;
#endif
    return Square(dx > 0 ? x : x - w + 1, dy > 0 ? y : y - h + 1, w, h);
}


void QuadGrid::reinit() {
    for(int y = 0; y < m_height; ++y)
        for(int x = 0;x < m_width; ++x) {
            auto& c = get(x,y);
            c.v = c.initv;
            c.sqind = -1;
        }
}


void QuadGrid::solve()
{
    //Grid initial, best;
    srand(0);

    vector<Square> bestSquares;

    int min = INT_MAX, minPass = -1;
    for (int pass = 0; pass < 1000; ++pass)
    {
        reinit();
        initMarks();
        m_squares.clear();
        int ind = 0;
        while (true) {
            Square s = candidate(rand() % 8);
            if (!s.width) {
                if ((int)m_squares.size() < min) {
                    min = m_squares.size();
                    bestSquares = m_squares;
                    minPass = pass;
                }
                break;
            }
            add(s, ind++);
        }
        //cout << "pass " << pass << " " << m_squares.size() << endl;

    }

    m_squares = bestSquares;
    cout << "QuadGrid " << m_ylevel << " found " << m_squares.size() << " squares pass=" << minPass << endl;


}
