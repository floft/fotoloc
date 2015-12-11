/*
 * Provides two useful elements:
 * - A class for storing lines that can be sorted based on length
 * - A function to split up a list of points into line segments
 */

#ifndef H_LINE
#define H_LINE

#include "math.h"
#include "coord.h"

struct Line
{
    Coord p1;
    Coord p2;
    double length;

    Line() : p1(Coord(0,0)), p2(Coord(0,0)), length(0) { }
    Line(const Coord& p1, const Coord& p2)
        : p1(p1), p2(p2), length(distance(p1, p2)) { }
    Line(const Coord& p1, const Coord& p2, double length)
        : p1(p1), p2(p2), length(length) { }
};

bool operator==(const Line& l1, const Line& l2);
bool operator<(const Line& l1, const Line& l2);
bool operator<=(const Line& l1, const Line& l2);
bool operator>(const Line& l1, const Line& l2);
bool operator>=(const Line& l1, const Line& l2);

// Look at how close the points between points i and j on this path fall to the
// line between points i and j to determine if these two points form a line
bool isLine(const std::vector<Coord>& path, int i, int j, double maxError);

// Return the average distance from the line between points i and j as a
// percentage of the length of the line, giving the line error
double lineError(const std::vector<Coord>& path, int i, int j);

// Non-overlapping halving and extending line search algorithm
//
// Split the path into line segments with a maximum error as the maximum
// percentage of the line length the points between two end points of a line
// can be from the line between those two endpoints on average
std::vector<Line> findLinesHalvingExtending(
        const std::vector<Coord>& path, double maxError);

// Extend the line while the error is decreasing, proceed a bit futher, and
// stop if the error doesn't drop below what it was before indicating that
// we've already found the best line
int findLargerLength(const std::vector<Coord>& path, double currentError,
        int start, int currentLength, int maxLookAhead);

// Non-overlapping extending while decreasing error line search algorithm
std::vector<Line> findLinesExtendingDecreasingError(
        const std::vector<Coord>& path, double maxError);

#endif
