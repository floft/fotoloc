/*
 * Rectangle
 */

#ifndef H_RECT
#define H_RECT

#include <iostream>

#include "coord.h"

struct Rect
{
    Coord tl;
    Coord br;

    Rect() { }

    Rect(const Coord& tl, const Coord& br)
        : tl(tl), br(br) { }

    // Is a particular point within this rectangle
    bool inside(const Coord& point) const;

    // Dimensions
    int width()  const { return br.x - tl.x + 1; }
    int height() const { return br.y - tl.y + 1; }
};

static const Rect default_rect(default_coord, default_coord);

std::ostream& operator<<(std::ostream& os, const Rect& c);
bool operator==(const Rect& c1, const Rect& c2);
bool operator!=(const Rect& c1, const Rect& c2);

#endif
