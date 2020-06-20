
#pragma once


namespace dpfb {


struct Point {
    int x;
    int y;

    Point()
        : Point(0, 0)
    {

    }

    Point(int x, int y)
        : x {x}
        , y {y}
    {

    }
};


struct Size {
    int w;
    int h;

    Size()
        : Size(0, 0)
    {

    }

    Size(int w, int h)
        : w {w}
        , h {h}
    {

    }
};


struct Edge {
    int top;
    int bottom;
    int left;
    int right;

    explicit Edge(int edge = 0)
        : Edge(edge, edge, edge, edge)
    {

    }

    Edge(int top, int bottom, int left, int right)
        : top(top)
        , bottom(bottom)
        , left(left)
        , right(right)
    {

    }
};


}
