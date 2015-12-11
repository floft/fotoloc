/*
 * Take an image (Pixels) and find all colored blobs/objects within it.
 *
 *   const Blobs blobs(img);
 *   for (const CoordPair& b : blobs)
 *     std::cout << b.first << std::endl;
 */

#ifndef H_BLOBS
#define H_BLOBS

#include <map>
#include <vector>

#include "log.h"
#include "pixels.h"
#include "maputils.h"
#include "disjointset.h"

// Remember the first and last times we saw a label so we can search just part
// of the image when updating a label.
struct CoordPair
{
    Coord first;
    Coord last;

    CoordPair() { }
    CoordPair(const Coord& f, const Coord& l)
        :first(f), last(l) { }
};

class Blobs
{
public:
    static const int default_label;
    typedef std::map<int, CoordPair>::size_type size_type;
    typedef MapValueIterator<std::map<int, CoordPair>> const_iterator;

private:
    int w = 0;
    int h = 0;
    DisjointSet<int> set;
    std::map<int, CoordPair> objs;
    std::vector<std::vector<int>> labels;

public:
    template<int N> Blobs(const Pixels<N>& img);
    int label(const Coord& p) const;
    CoordPair object(int label) const;

    // Allow moving
    Blobs(Blobs&&);
    Blobs& operator=(Blobs&& other);

    // Get all first points that have part of the object in the rectangle
    // around p1 and p2 (with p1 to the left and above p2).
    std::vector<Coord> in(const Coord& p1, const Coord& p2) const;

    // Get all the first points of the label within a rectangle around
    // the points p1 and p2. This assumes that p2 is down and to the right
    // of p1.
    std::vector<Coord> startIn(const Coord& p1, const Coord& p2) const;

    // Standard functions
    const_iterator begin() const { return objs.begin(); }
    const_iterator end() const { return objs.end(); }
    size_type size() const { return objs.size(); }

private:
    // Merge object o into object n by changing labels and updating object
    void switchLabel(int old_label, int new_label);
};

// We only need to template this one function because it is the only one that
// depends on the number of channels in a passed in image
template<int N>
Blobs::Blobs(const Pixels<N>& img)
    : set(default_label)
{
    w = img.width();
    h = img.height();
    labels = std::vector<std::vector<int>>(h, std::vector<int>(w, default_label));

    int next_label = default_label+1;

    // Go through points making them part of bordering objects if next to one
    // already and otherwise a new object
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const Coord point(x, y);
            const std::array<unsigned char, N> current_color = img.color(point);

            const std::array<Coord, 4> points = {{
                Coord(x-1, y),   // Left
                Coord(x-1, y-1), // Up left
                Coord(x,   y-1), // Up
                Coord(x+1, y-1)  // Up right
            }};

            // Conditions we care about:
            //   One neighboring pixel is the same color
            //   Multiple the same color, all the same label
            //   Multiple the same color, not all the same labels
            //
            // We can determine this by counting the number of pixels that are
            // the same color around us and knowing if two are different.
            int count = 0;
            bool different = false;
            std::vector<int> equivalent_labels;

            // We will have at most four since the current pixel takes on the
            // label of one of these 4 surrounding pixels
            equivalent_labels.reserve(4);

            for (const Coord& p : points)
            {
                // We need the boundary check here because it is possible that
                // a pixel in the image will be the same as the default color
                // and we don't want to access memory outside of our arrays
                if (p.x >= 0 && p.y >= 0 &&
                    p.x < w  && p.y < h &&
                    img.color(p) == current_color)
                {
                    if (count == 0)
                    {
                        labels[y][x] = labels[p.y][p.x];
                    }
                    // Detect when we have multiple pixels of this color around
                    // us with different labels
                    else if (labels[y][x] != labels[p.y][p.x])
                    {
                        different = true;
                        equivalent_labels.push_back(labels[p.y][p.x]);
                    }

                    ++count;
                }
            }

            // No neighboring pixels the same color
            if (count == 0)
            {
                // New label for a potentially new object
                labels[y][x] = next_label;
                set.add(next_label);

                ++next_label;
            }
            // Multiple of same color with different labels
            else if (count > 1 && different)
            {
                // Save that these are all equivalent to the current one
                for (int label : equivalent_labels)
                    set.join(labels[y][x], label);
            }
            // Otherwise: One neighbor same color or multiple but all same
            // label, and we already set the current pixel's label
        }
    }

    // Go through again reducing the labeling equivalences
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const Coord point(x, y);
            int currentLabel = labels[y][x];

            if (currentLabel != default_label)
            {
                int repLabel = set.find(currentLabel);

                if (repLabel != set.notfound())
                {
                    labels[y][x] = repLabel;

                    // If not found, add this object
                    if (objs.find(repLabel) == objs.end())
                        objs[repLabel] = CoordPair(point, point);
                    // If it is found, this is the last place we've seen the object
                    else
                        objs[repLabel].last = point;
                }
                else
                {
                    log("couldn't find representative of label");
                }
            }
        }
    }
}

#endif
