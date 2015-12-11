/*
 * Creates a histogram for a 2D array of pixel values
 */

#ifndef H_HISTOGRAM
#define H_HISTOGRAM

#include <array>
#include <vector>
#include <algorithm>

#include "rect.h"

// Templated on number of channels
template<int N>
class Histogram
{
    static_assert(N == 1 || N == 3 || N == 4, "Must have 1, 3, or 4 channels");

    int total;
    std::vector<int> graphGray;
    std::array<std::vector<int>, N> graph;

public:
    // Each inner vector is assumed to have the same length
    Histogram(const std::vector<std::vector<std::array<unsigned char, N>>>& img,
            Rect rect = default_rect);

    // Auto threshold. Specify the initial threshold to use to determine the
    // foreground and background.
    unsigned char threshold(unsigned char initial) const;

    // Standard deviation of the mean for the grayscale histogram or for each
    // of the channels (if N == 1, then it'll return the same, just in an array
    // with stdev())
    double stdevGray() const;
    std::array<double, N> stdev() const;

    // Versions using normalized histograms
    double stdevGrayNorm() const;
    std::array<double, N> stdevNorm() const;

    // Normalize a histogram so the values now range between 0 and 1,
    // where 1 is 100% of the pixels and 0 is 0% (i.e., if there is a
    // 1, then the rest are 0)
    static std::vector<double> normalized(const std::vector<int>& in);
};

/*
 * Implementation
 */

template<int N>
Histogram<N>::Histogram(
        const std::vector<std::vector<std::array<unsigned char, N>>>& img, Rect rect)
    : graphGray(256, 0) // This is unsigned char, so there's 0-255
{
    // Fill the array with the same as graphGray
    graph.fill(graphGray);

    int h = img.size();
    int w = (h>0)?img[0].size():0;
    total = w*h;

    // Specify custom bounds in the image if not the default rectangle
    int minX = 0;
    int minY = 0;
    int maxX = w;
    int maxY = h;

    if (rect != default_rect)
    {
        minX = rect.tl.x;
        minY = rect.tl.y;
        maxX = rect.br.x;
        maxY = rect.br.y;
    }

    // Generate the graph by counting how many pixels are each shade. This
    // is easy with discrete values, would be more interesting with doubles.
    for (int y = minY; y < maxY; ++y)
    {
        for (int x = minX; x < maxX; ++x)
        {
            // Grayscale graph
            int sum = 0;

            for (int i = 0; i < N; ++i)
                sum += img[y][x][i];

            ++graphGray[sum/N];

            // Individual channels
            for (int i = 0; i < N; ++i)
                ++graph[i][img[y][x][i]];
        }
    }
}

// This simple algorithm worked just as good and executed faster than some
// others I found online. Thus, I'll keep this simple one.
template<int N>
unsigned char Histogram<N>::threshold(unsigned char initial) const
{
    typedef std::vector<int>::const_iterator iterator;

    // This is my algorithm that currently works better than the one above.
    const iterator half = graphGray.begin() + initial;
    iterator black_max = std::max_element(graphGray.begin(), half);
    iterator white_max = std::max_element(half, graphGray.end());

    if (black_max != half && white_max != graphGray.end())
        return 0.5*((black_max-graphGray.begin()) + (white_max-graphGray.begin()));

    return initial;
}

template<int N>
std::vector<double> Histogram<N>::normalized(const std::vector<int>& in)
{
    std::vector<double> out;
    out.reserve(in.size());

    double total = std::accumulate(in.begin(), in.end(), 0);

    if (total != 0)
        for (unsigned int i = 0; i < in.size(); ++i)
            out.push_back(1.0*in[i]/total);

    return out;
}

template<int N>
double Histogram<N>::stdevGray() const
{
    return ::stdev(graphGray);
}

template<int N>
double Histogram<N>::stdevGrayNorm() const
{
    std::vector<double> norm = normalized(graphGray);
    return ::stdev(norm);
}

template<int N>
std::array<double, N> Histogram<N>::stdev() const
{
    std::array<double, N> s;

    for (int i = 0; i < N; ++i)
        s[i] = ::stdev(graph[i]);

    return s;
}

template<int N>
std::array<double, N> Histogram<N>::stdevNorm() const
{
    std::array<double, N> s;

    for (int i = 0; i < N; ++i)
    {
        std::vector<double> norm = normalized(graph[i]);
        s[i] = ::stdev(norm);
    }

    return s;
}

#endif
