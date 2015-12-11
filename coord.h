/*
 * Image coordinates
 */

#ifndef H_COORD
#define H_COORD

#include <iostream>

struct Coord
{
    int x;
    int y;

    Coord() : x(0), y(0) { }
    Coord(const int x, const int y) : x(x), y(y) { }

    Coord& operator+=(const Coord& c);
    Coord operator+(const Coord& c) const;
};

static const Coord default_coord(-1, -1);

// A function object (functor) for sorting points by X position
struct CoordXSort
{
    inline bool operator() (const Coord& p1, const Coord& p2)
    {
         return p1.x < p2.x;
    }
};

std::ostream& operator<<(std::ostream& os, const Coord& c);
std::istream& operator>>(std::istream& is, Coord& c);
bool operator==(const Coord& c1, const Coord& c2);
bool operator!=(const Coord& c1, const Coord& c2);

// Less than: Is the y value less? If the same, is the x value less?
bool operator<(const Coord& c1, const Coord& c2);
bool operator>(const Coord& c1, const Coord& c2);

#endif
