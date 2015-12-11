#include "rect.h"

bool Rect::inside(const Coord& point) const
{
    return point.x >= tl.x && point.y >= tl.y &&
           point.x <= br.x && point.y <= br.y;
}

std::ostream& operator<<(std::ostream& os, const Rect& c)
{
    return os << "{ " << c.tl << ", " << c.br << " }";
}

bool operator==(const Rect& r1, const Rect& r2)
{
    return (r1.tl == r2.tl && r1.br == r2.br);
}

bool operator!=(const Rect& r1, const Rect& r2)
{
    return !(r1==r2);
}
