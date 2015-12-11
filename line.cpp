#include "line.h"

bool operator==(const Line& l1, const Line& l2)
{
    return l1.p1 == l2.p1 && l1.p2 == l2.p2 && l1.length == l2.length;
}

bool operator<(const Line& l1, const Line& l2)
{
    return l1.length < l2.length;
}

bool operator<=(const Line& l1, const Line& l2)
{
    return l1.length <= l2.length;
}

bool operator>(const Line& l1, const Line& l2)
{
    return l1.length > l2.length;
}

bool operator>=(const Line& l1, const Line& l2)
{
    return l1.length >= l2.length;
}

bool isLine(const std::vector<Coord>& path, int i, int j, double maxError)
{
    // Make sure we don't get in an infinte loop. We need to reach point j
    // to break out of this function, so make sure we will reach it.
    if (i < 0 || j < 0 || i >= path.size() || j >= path.size())
        return false;

    std::vector<double> dist;

    // Added complexity to make this work even if j < i, i.e. wrap around works
    for (int k = i+1; k != j; k=(k+1)%path.size())
        dist.push_back(distance(path[i], path[j], path[k]));

    // Right now just look at all the points between these two
    //const int checkjump = 1;
    //for (int k = i+1; k < j; k+=checkjump)
    //    dist.push_back(distance(path[i], path[j], path[k]));

    double avg = average(dist);
    double stddev = stdev(dist);

    // Maximum average distance from the two lines is the max error (e.g. 1% as
    // maxError = 0.01) times the distance between the two points. This way we
    // won't have a tendency toward small line segments.
    double avgThresh = distance(path[i], path[j])*maxError;

    // Make sure it's line-ish (TODO: better reasoning)
    double stddevThresh = avgThresh/2;

    // If low average distance and standard deviation, then this is
    // approximately straight
    return avg < avgThresh && stddev < stddevThresh;
}

double lineError(const std::vector<Coord>& path, int i, int j)
{
    std::vector<double> dist;

    // Make sure we don't get in an infinte loop. We need to reach point j
    // to break out of this function, so make sure we can reach it.
    i = i%path.size();
    j = j%path.size();

    for (int k = (i+1)%path.size(); k != j; k=(k+1)%path.size())
        dist.push_back(distance(path[i], path[j], path[k]));

    // Convert average distance to percentage of line length and multiply by
    // the standard deviation to make us look for real lines instead of
    // segments with lots of noise
    return average(dist)/distance(path[i], path[j]);//*stdev(dist);
}

std::vector<Line> findLinesHalvingExtending(
        const std::vector<Coord>& path, double maxError)
{
    const int minLength = 10; // Minimum line length TODO: set this elsewhere

    std::vector<Line> lines;

    // If empty, no lines
    if (path.empty())
        return lines;

    const int size = path.size();

    // If the whole path is a line, we're done
    double wholeLength = distance(path[0], path[size-1]);

    if (isLine(path, 0, size-1, maxError) && wholeLength > minLength)
    {
        lines.push_back(Line(path[0], path[size-1], wholeLength));
        return lines;
    }

    // Otherwise, look for a line of length half the total number of points,
    // starting at the beginning and sliding along toward the end, ending at
    // the last point wrapping around to the beginning
    int length = size/2;

    // Save the first point of the line
    int firstStart = -1;
    int firstEnd = 0;

    for (int i = 0; i < size; ++i)
    {
        // Stop if the length is too small
        //
        // Note: we're somewhat assuming that the points in the path are
        // basically one pixel apart, so this length is about the length in
        // pixels. Otherwise, there's no guarantee that a length of 1 isn't
        // 1000 pixels or so and definitely a straight line.
        if (length < minLength)
            break;

        // If this is a line, extend it until it isn't a line
        if (isLine(path, i, i+length, maxError))
        {
            int largerLength = length+1;

            for ( ; i+largerLength < size; ++largerLength)
                if (!isLine(path, i, i+largerLength, maxError))
                    break;

            // Subtract one since the last one resulted in it not being a line
            --largerLength;

            // Save this for the next step
            firstStart = i;
            firstEnd = (i+largerLength)%size;

            // Save this as a line
            lines.push_back(Line(path[firstStart], path[firstEnd]));

            break;
        }

        // If we're still going, shrink the length again until we end up finding
        // a line or hit the minimum length limit
        length /= 2;
    }

    // If we didn't find any lines greater than the minimum length, give up
    if (firstStart == -1)
        return lines;

    // Now that we found the first line, go to the next point and find the
    // longest line from that point onward, continuing to segment the path into
    // lines
    for (int i = firstEnd; i < size+firstStart; ++i)
    {
        // Look at the longest possible line
        if (isLine(path, i, i+minLength, maxError))
        {
            int largerLength = minLength+1;

            for ( ; i+largerLength < size; ++largerLength)
                if (!isLine(path, i, i+largerLength, maxError))
                    break;

            // Subtract one since the last one resulted in it not being a line
            --largerLength;

            // Save this as a line
            lines.push_back(Line(path[i], path[(i+largerLength)%size]));

            // Skip to the end of this line and look for another line
            i += largerLength;

            break;
        }
    }

    return lines;
}

int findLargerLength(const std::vector<Coord>& path, double currentError,
        int start, int currentLength, int maxLookAhead)
{
    int increasing = 0;
    int size = path.size();
    int largerLength = currentLength+1;

    // Extend the current line until we reach a point where the error keeps
    // increasing
    for ( ; (start+largerLength)%size < start; ++largerLength)
    {
        double newError = lineError(path, start, start+largerLength);

        // If still a line, then make sure the next line is better than this
        // one. If not, then continue looking for a decrease for a bit and then
        // exit if we didn't find a better line.
        if (newError < currentError)
        {
            currentError = newError;
            increasing = 0;
        }
        else if (increasing < maxLookAhead)
        {
            ++increasing;
        }
        else
        {
            break;
        }
    }

    // Subtract off however many we tried after finding the good line
    largerLength -= increasing+1;

    return largerLength;
}

std::vector<Line> findLinesExtendingDecreasingError(
        const std::vector<Coord>& path, double maxError)
{
    const int minLength = 100; // Minimum line length TODO: set this elsewhere
    const int linejump = 1; // Distance to jump between line segments
    const int maxLookAhead = 25; // Max number of points to look ahead when error is increasing

    std::vector<Line> lines;

    // If empty, no lines
    if (path.empty())
        return lines;

    const int size = path.size();

    // Look for a line of length half the total number of points, starting at
    // the beginning and sliding along toward the end, ending at the last point
    // wrapping around to the beginning
    int length = size/2;

    // Save the first point of the line
    int firstStart = -1;
    int firstEnd = 0;

    for (int i = 0; i < size; ++i)
    {
        // Stop if the length is too small
        //
        // Note: we're somewhat assuming that the points in the path are
        // basically one pixel apart, so this length is about the length in
        // pixels. Otherwise, there's no guarantee that a length of 1 isn't
        // 1000 pixels or so and definitely a straight line.
        if (length < minLength)
            break;

        // What is the current error?
        double currentError = lineError(path, i, i+length);

        // If this is a line, extend it while the error is decreasing
        if (currentError < maxError)
        {
            // TODO: take into consideration minlength??? make sure the lines
            // don't suddenly decrease in length even if largerLength is
            // increasing
            int largerLength = findLargerLength(path, currentError, i, length, maxLookAhead);

            firstStart = i;
            firstEnd = (i+largerLength)%size;

            lines.push_back(Line(path[firstStart], path[firstEnd]));

            break;
        }

        // If we're still going, shrink the length again until we end up finding
        // a line or hit the minimum length limit
        length /= 2;
    }

    // If we didn't find any lines greater than the minimum length, give up
    if (firstStart == -1)
        return lines;

    // Decrease the length by half. We found about the maximum length of line
    // for this path, so if we look for shorter lines and try to extend them,
    // we'll probably find about the right length of lines for this path.
    length /= 2;

    // But, if now it's too short, just return that longest line.
    if (length < minLength)
        return lines;

    // Now that we found the first line, go to the next point and find the
    // longest line from that point onward, continuing to segment the path into
    // lines
    for (int i = firstEnd; i < size+firstStart; i+=linejump)
    {
        // What is the current error?
        double currentError = lineError(path, i, i+length);

        // If this is a line, extend it while the error is decreasing
        if (currentError < maxError)
        {
            int largerLength = findLargerLength(path, currentError, i, length, maxLookAhead);

            // Save this as a line
            lines.push_back(Line(path[i%size], path[(i+largerLength)%size]));

            // Skip to the end of this line and look for another line
            i += largerLength;
        }
    }

    return lines;
}
