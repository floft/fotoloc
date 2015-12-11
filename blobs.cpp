#include "blobs.h"

const int Blobs::default_label = 0;

Blobs::Blobs(Blobs&& other)
    : w(other.w), h(other.h), set(std::move(other.set)),
      objs(std::move(other.objs)), labels(std::move(other.labels))
{
}

Blobs& Blobs::operator=(Blobs&& other)
{
    w = other.w;
    h = other.h;
    set = std::move(other.set);
    objs = std::move(other.objs);
    labels = std::move(other.labels);

    return *this;
}

int Blobs::label(const Coord& p) const
{
    if (p.x >= 0 && p.x < w &&
        p.y >= 0 && p.y < h)
        return labels[p.y][p.x];
    else
        return default_label;
}

std::vector<Coord> Blobs::in(const Coord& p1, const Coord& p2) const
{
    std::set<int> used_labels;
    std::vector<Coord> subset;

    for (int y = p1.y; y < p2.y; ++y)
    {
        for (int x = p1.x; x < p2.x; ++x)
        {
            if (labels[y][x] != default_label &&
                    used_labels.find(labels[y][x]) == used_labels.end())
            {
                const std::map<int, CoordPair>::const_iterator obj = objs.find(labels[y][x]);

                if (obj != objs.end())
                {
                    subset.push_back(obj->second.first);
                    used_labels.insert(labels[y][x]);
                }
                else
                {
                    log("couldn't find object with label");
                }
            }
        }
    }

    return subset;
}

std::vector<Coord> Blobs::startIn(const Coord& p1, const Coord& p2) const
{
    std::vector<Coord> subset;

    for (const std::pair<int, CoordPair>& p : objs)
    {
        if (p.second.first.y >= p1.y && p.second.first.y <= p2.y &&
            p.second.first.x >= p1.x && p.second.first.x <= p2.x)
            subset.push_back(p.second.first);

        // Break early when we've gone past it
        else if (p.second.first.y > p2.y)
            break;
    }

    return subset;
}

CoordPair Blobs::object(int label) const
{
    const std::map<int, CoordPair>::const_iterator obj = objs.find(label);

    if (obj != objs.end())
        return obj->second;
    else
        return CoordPair();
}
