/*
 * Find regions in an image
 */

#include <vector>
#include <iterator>
#include <iostream>
#include <algorithm>

#include "rect.h"
#include "pixels.h"

// Is a particular region of an image "interesting", part of the background
// that we will look at further later
template<int N>
bool interesting(Pixels<N>& img, const Rect& rect)
{
    // Rectangular region we're looking at
    int width  = rect.width();
    int height = rect.height();
    int minX = rect.tl.x;
    int minY = rect.tl.y;
    int maxX = rect.br.x;
    int maxY = rect.br.y;

    const std::vector<std::vector<std::array<unsigned char, N>>>& ref = img.ref();

    // Local threshold value
    const Histogram<N> histWhole(ref, rect);
    unsigned char hist_thresh = histWhole.threshold(GRAY_SHADE);

    /*
     * Requirements:
     *      Fairly white
     *      Approximately grayscale
     *      Not much range in min to max values
     *      Fairly low saturation?
     *      Low stdev for whole image threshold
     *      High stdev for local threshold
     */
    bool isWhite = false;
    bool isGray = false;
    bool isSimilar = false;

    double grayThresh = 20; // Max difference in averages among channels
    double similarThresh = 15; // Max differences if max-min pixels in any channel
    double whiteThresh = 0.90; // What is considered "white"

    /*
     * Count number of pixels matching the requirements
     */
    int white = 0;
    int total = 0;
    std::array<int, N> valuesSum{};
    std::array<int, N> minValue; // Initialize to 255
    std::array<int, N> maxValue{}; // Initialize to 0
    std::fill(minValue.begin(), minValue.end(), 255);

    for (int y = minY; y < maxY; ++y)
    {
        for (int x = minX; x < maxX; ++x, ++total)
        {
            // White
            if (!img.black(Coord(x, y)))
                ++white;

            // Grayscale
            const std::array<unsigned char, N>& values = ref[y][x];

            for (int i = 0; i < N && i < 3; ++i)
            {
                if (values[i] > maxValue[i])
                    maxValue[i] = values[i];
                if (values[i] < minValue[i])
                    minValue[i] = values[i];

                valuesSum[i] += values[i];
            }
        }
    }

    // Since this is thresholded, it is actually quite likely that 100% of
    // these pixels on the background are white; however, it's also quite
    // possible that there will be a few black pixels of noise
    isWhite = 1.0*white/total > whiteThresh;

    // Look for the difference in min to max values in each channel
    isSimilar = true;

    for (int i = 0; i < N; ++i)
    {
        int diff = maxValue[i] - minValue[i];

        if (diff < 0 || diff > similarThresh)
        {
            isSimilar = false;
            break;
        }
    }

    // We'll say it's gray if the difference between min and max averages for
    // each channel are within a certain threshold
    double maxAvg = 0;
    double minAvg = 255;

    for (int i = 0; i < N; ++i)
    {
        double avg = 1.0*valuesSum[i]/total;

        if (avg > maxAvg)
            maxAvg = avg;
        if (avg < minAvg)
            minAvg = avg;
    }

    double diff = maxAvg - minAvg;

    if (diff >= 0 && diff < grayThresh)
        isGray = true;

    /*
     * Low standard deviation for thresholding of the entire image
     */

    /*
     * High standard deviation for thresholding of the local region
     */

    return isWhite && isGray && isSimilar;

    /*
     * Detect edge/corner of photo
    // Find histogram standard deviations of the mean of the four quadrants of
    // this region
    std::array<Rect, 4> rects = {{
            // Top left
            Rect(Coord(minX, minY), Coord(minX + width/2, minY + height/2)),
            // Top right
            Rect(Coord(minX + width/2, minY), Coord(maxX, minY + height/2)),
            // Bottom left
            Rect(Coord(minX, minY + height/2), Coord(minX + width/2, maxY)),
            // Bottom right
            Rect(Coord(minX + width/2, minY + height/2), Coord(maxX, maxY)),
        }};

    bool debug = rect.inside(Coord(2576,123));
    //bool debug = rect.inside(Coord(526,819));

    if (debug)
        std::cout << "Rect: " << rect << std::endl;

    std::array<std::array<double, N+1>, 4> stdev;

    for (int i = 0; i < 4; ++i)
    {
        const Histogram<N> hist(ref, rects[i]);

        // Standard deviation of grayscale and each channel
        double stdev_gray = hist.stdevGrayNorm();
        std::array<double, N> stdev_color = hist.stdevNorm();

        stdev[i][0] = stdev_gray;

        if (debug)
            std::cout << stdev_gray << ", ";

        for (int j = 0; j < N; ++j)
        {
            stdev[i][j+1] = stdev_color[j];

            if (debug)
                std::cout << stdev_color[j] << ", ";
        }

        if (debug)
            std::cout << std::endl;
    }

    // Verify standard deviation of each of the four regions is below threshold
    int count = 0;

    // Thresholds
    double stdevThresh = 1.03;
    double blackThresh = 0.5;

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < N+1; ++j)
        {
            if (stdev[i][j] < stdevThresh)
            {
                ++count;
                break;
            }
        }
    }

    if (count == 4)
    {
        // See if exactly one or two of the quadrants are colored in at least
        // one of the channels
        int blackQuadrant = 0;

        for (int i = 0; i < 4; ++i)
        {
            int minX = rects[i].tl.x;
            int minY = rects[i].tl.y;
            int maxX = rects[i].br.x;
            int maxY = rects[i].br.y;

            for (int channel = 0; channel < N; ++channel)
            {
                int black = 0;
                int white = 0;

                for (int y = minY; y < maxY; ++y)
                {
                    for (int x = minX; x < maxX; ++x)
                    {
                        //if (img.black(Coord(x,y)) || img.ref()[y][x][channel] < hist_thresh)
                        if (img.ref()[y][x][channel] < hist_thresh)
                            ++black;
                        else
                            ++white;
                    }
                }

                if (black == 0)
                    continue;

                if (debug)
                    std::cout << 1.0*black/(black+white) << std::endl;

                if (1.0*black/(black+white) > blackThresh)
                {
                    ++blackQuadrant;
                    break;
                }
            }
        }

        if (debug)
            std::cout << "Thresh: " << (int)hist_thresh
                << ", Black quad: " << blackQuadrant
                << std::endl;

        // 1, 2, or 3, we want edges or corners
        if (blackQuadrant != 0 && blackQuadrant != 4)
            return true;
    }*/

    //
    // Interesting: half black, half white based on local histogram
    //
    /*
    int black = 0;
    int white = 0;

    for (int y = minY; y < maxY; ++y)
    {
        for (int x = minX; x < maxX; ++x)
        {
            if (img.black(Coord(x, y)))
                ++black;
            else
                ++white;

            // TODO: the below is even worse than the above version

            // We won't want to add in the alpha channel here, so change it if
            // we ever support that
            static_assert(N <= 3,
                    "interesting() must be changed to support alpha channel");

            int sum = 0;
            const std::array<unsigned char, N>& values = ref[y][x];

            for (int i = 0; i < N && i < 4; ++i)
                sum += values[i];

            if (sum/N < hist_thresh)
                ++black;
            else
                ++white;
        }
    }

    if (black == 0 || white == 0)
        return false;

    // Allow for 50% black pixels +/- this percent (between 0 and 1)
    double thresh = 0.3;

    // Is the number of black pixels within a certain percentage of 50%
    if (std::abs(1.0*black/(black+white) - 0.5) < thresh)
        return true;*/

    //
    // Interesting: half saturated, half unsaturated
    //

    //
    // Interesting: local histogram: half black, half white
    //

    return false;
}

// The recursive part
//
// Returned are temporary regions, the final regions are saved to finalRegions,
template<int N>
std::vector<Rect> regionRecursive(Pixels<N>& img, std::vector<Rect>& finalRegions,
        const Rect& rect, int small)
{
    std::vector<Rect> regions;

    // Rectangular region we're looking at, make sure they are within bounds
    int minX = std::max(rect.tl.x, 0);
    int minY = std::max(rect.tl.y, 0);
    int maxX = std::min(rect.br.x, img.width()-1);
    int maxY = std::min(rect.br.y, img.height()-1);

    // Split this into 4 sections (if we used floor it would be more than 4 due
    // to rounding issues)
    int newX = std::ceil(1.0*(maxX - minX) / 2);
    int newY = std::ceil(1.0*(maxY - minY) / 2);

    // Exit condition, if too small
    if (newX < small || newY < small)
        return regions;

    // Looking at smaller regions
    for (int y = minY; y < maxY; y+=newY)
    {
        for (int x = minX; x < maxX; x+=newX)
        {
            // New region
            Rect rect(Coord(x, y), Coord(x + newX, y + newY));

            // Look at the regions below this
            std::vector<Rect> newRegions = regionRecursive(
                    img, regions, rect, small);

            // TODO: we don't actually use this anywhere, make this function
            // return bool for whether or not we are done or found interesting
            // regions
            regions.insert(
                    regions.end(),
                    std::make_move_iterator(newRegions.begin()),
                    std::make_move_iterator(newRegions.end()));
        }
    }

    // If this region is interesting and we didn't find any interesting ones
    // within this one, append it to the list of final regions
    if (regions.empty() && interesting(img, rect))
    {
        finalRegions.push_back(rect);

        // Debugging
        img.mark(Coord(minX, minY));
        img.mark(Coord(minX, maxY));
        img.mark(Coord(maxX, minY));
        img.mark(Coord(maxX, maxY));
    }

    return regions;
}

// Recursively look at smaller and smaller regions in the image to determine if
// we've found some "interesting" region
template<int N>
std::vector<Rect> findRegions(Pixels<N>& img)
{
    std::vector<Rect> regions;

    // Start with entire image
    Rect all(Coord(0,0), Coord(img.width(), img.height()));

    // Minimum size, we got to look at bigger than 4 since we are looking for
    // about 50% to be black. This means we need a few to check.
    int small = 4;

    // Start recursion
    regionRecursive(img, regions, all, small);

    return regions;
}
