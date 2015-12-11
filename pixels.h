/*
 * Class to allow pixel access and rotation to an image
 *
 * Note: Remember to ilInit() before using this
 */

#ifndef H_PIXELS
#define H_PIXELS

#include <cmath>
#include <array>
#include <mutex>
#include <vector>
#include <string>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <IL/il.h>

#include "log.h"
#include "coord.h"
#include "math.h"
#include "utils.h"
#include "pixels.h"
#include "histogram.h"

// The default gray value
static const int GRAY_SHADE = 127;
// How big to make the marks
static const int MARK_SIZE = 5;
// Color of mark
static const unsigned char MARK_COLOR = 127;

enum class OutputColor
{
    Color,
    BlackAndWhite,
    Grayscale,
    Unknown
};

struct Mark
{
    Coord coord;
    int size;

    Mark(Coord c, int s)
        :coord(c), size(s) { }
};

// Templated on number of channels, i.e.
//      Pixels<1> is Grayscale
//      Pixels<3> is RGB
//      Pixels<4> is RGBA
template<int N>
class Pixels
{
    static_assert(N == 1 || N == 3 || N == 4, "Must have 1, 3, or 4 channels");

    typedef std::vector<std::vector<std::array<unsigned char, N>>> PixelArray;

    std::vector<Mark> marks;
    PixelArray p;
    int w;
    int h;
    bool loaded;
    std::string fn;
    unsigned char gray_shade;

    // Lock this so that only one thread can read an image or save()
    // OpenIL/DevIL is not multithreaded
    static std::mutex lock;

public:
    Pixels(); // Useful for placeholder
    Pixels(std::vector<std::vector<std::array<unsigned char, N>>>&& pixels,
            const std::string& fn = "");
    Pixels(const std::vector<std::vector<std::array<unsigned char, N>>>& pixels,
            const std::string& fn = "");
    Pixels(ILenum type, const char* lump, const int size, const std::string& fn = "");

    inline bool valid()  const { return loaded; }
    inline int  width()  const { return w; }
    inline int  height() const { return h; }
    inline const std::string& filename() const { return fn; }

    // This doesn't extend the image at all. If rotation and points
    // are determined correctly, it won't rotate out of the image.
    // Note: rad is angle of rotation in radians
    void rotate(double rad, const Coord& point);

    // Default is used if coord doesn't exist (which should never happen)
    // Default to white to assume that this isn't a useful pixel
    inline bool black(const Coord& c, const bool default_value = false) const;

    // If not dealing with black and white photos, then get the color of a pixel
    // Default color is white unless second argument specified
    inline std::array<unsigned char, N> color(const Coord& c) const;
    inline std::array<unsigned char, N> color(const Coord& c,
        const std::array<unsigned char, N>& default_color) const;

    // Get reference to the data so we can extensively process it
    const std::vector<std::vector<std::array<unsigned char, N>>>& ref() const;

    // When saving, we'll display marks optionally
    void mark(const Coord& m, int size = MARK_SIZE);

    // Mark every pixel on the line between p1 and p2
    void line(const Coord& p1, const Coord& p2);

    // Used for debugging, all processing (converting to black-and-white, adding
    // the marks, dimming the image) is done on a copy of the image
    void save(const std::string& filename, const bool show_marks = true,
              const bool dim = true, const OutputColor color = OutputColor::Color) const;

    // Rotate p around origin an amount in radians, sin_rad = sin(rad) and
    // cos_rad = cos(rad). We pass in these values because otherwise we calculate
    // them thousands of times.
    Coord rotatePoint(const Coord& origin, const Coord& p,
        double sin_rad, double cos_rad) const;

    // Rotate all points in a vector
    void rotateVector(std::vector<Coord>& v, const Coord& point, double rad) const;

    // Was the image successfully loaded?
    bool isLoaded() const { return loaded; }

    // Get the grayscale value
    unsigned char grayShade() const { return gray_shade; }

    // A simple quantization rounding each channel value into a certain number
    // of bins
    Pixels<N> quantize(const int amount) const;

    // Gaussian blur
    Pixels<N> blurPerfect(const int amount) const;
    // Fast blur
    Pixels<N> blur(const int amount) const;

private:
    // For blur
    //
    // From: http://blog.ivank.net/fastest-gaussian-blur.html
    std::vector<int> boxesForGauss(int sigma, int n) const;
    void gaussBlur_4(PixelArray& scl, PixelArray& tcl, int r) const;
    void boxBlur_4(PixelArray& scl, PixelArray& tcl, int r) const;
    void boxBlurH_4(const PixelArray& scl, PixelArray& tcl, int r) const;
    void boxBlurT_4(const PixelArray& scl, PixelArray& tcl, int r) const;
};

// Used so frequently and so small, so make this inline
template<int N>
inline bool Pixels<N>::black(const Coord& c, const bool default_value) const
{
    if (c.x >= 0 && c.y >= 0 &&
        c.x < w  && c.y < h)
    {
        int sum = 0;

        for (int i = 0; i < N; ++i)
            sum += p[c.y][c.x][i];

        return sum/N < gray_shade;
    }

    return default_value;
}

template<int N>
inline std::array<unsigned char, N> Pixels<N>::color(const Coord& c) const
{
    if (c.x >= 0 && c.y >= 0 &&
        c.x < w  && c.y < h)
    {
        return p[c.y][c.x];
    }

    return make_array<N, unsigned char>(0xff);
}

template<int N>
inline std::array<unsigned char, N> Pixels<N>::color(const Coord& c,
        const std::array<unsigned char, N>& default_color) const
{
    if (c.x >= 0 && c.y >= 0 &&
        c.x < w  && c.y < h)
    {
        return p[c.y][c.x];
    }

    return default_color;
}

// Get constant reference
template<int N>
const std::vector<std::vector<std::array<unsigned char, N>>>& Pixels<N>::ref() const
{
    return p;
}

/*
 * Implementation
 */

// DevIL/OpenIL isn't multithreaded
template<int N>
std::mutex Pixels<N>::lock;

template<int N>
Pixels<N>::Pixels()
    :w(0), h(0), loaded(false), gray_shade(GRAY_SHADE)
{
}

// type is either IL_JPG, IL_TIF, or IL_PNM in this case
template<int N>
Pixels<N>::Pixels(ILenum type, const char* lump, const int size, const std::string& fn)
    :w(0), h(0), loaded(false), fn(fn), gray_shade(GRAY_SHADE)
{
    // Only execute in one thread since DevIL/OpenIL doesn't support multithreading,
    // so use a unique lock here. But, we'll do a bit more that doesn't need to be
    // in a single thread, so make the lock go out of scope when we're done.
    {
        std::unique_lock<std::mutex> lck(lock);

        ILuint name;
        ilGenImages(1, &name);
        ilBindImage(name);

        if (ilLoadL(type, lump, static_cast<ILuint>(size)))
        {
            w = ilGetInteger(IL_IMAGE_WIDTH);
            h = ilGetInteger(IL_IMAGE_HEIGHT);

            // If the image height or width is larger than int's max, it will appear
            // to be negative. Just don't use extremely large (many gigapixel) images.
            if (w < 0 || h < 0)
                throw std::runtime_error("use a smaller image, can't store dimensions in int");

            // 3 because IL_RGB
            const int total = w*h*3;
            unsigned char* data = new unsigned char[total];

            ilCopyPixels(0, 0, 0, w, h, 1, IL_RGB, IL_UNSIGNED_BYTE, data);

            // Move data into a nicer format
            int x = 0;
            int y = 0;
            p = std::vector<std::vector<std::array<unsigned char, N>>>(
                    h, std::vector<std::array<unsigned char, N>>(w));

            // Start at third
            for (int i = 2; i < total; i+=3)
            {
                // Grayscale
                if (N == 1)
                {
                    // Average min and max to get lightness
                    //  p[y][x] = smartFloor((min(data[i-2], data[i-1], data[i]) +
                    //                        max(data[i-2], data[i-1], data[i]))/2);
                    // For average:
                    //  p[y][x] = smartFloor((1.0*data[i-2]+data[i-1]+data[i])/3);
                    //
                    // For luminosity:
                    //  p[y][x] = smartFloor(0.2126*data[i-2] + 0.7152*data[i-1] + 0.0722*data[i]);
                    //
                    // Use the simplest. It doesn't seem to make a difference.
                    p[y][x][0] = smartFloor((1.0*data[i-2]+data[i-1]+data[i])/3);
                }
                // RGB
                else if (N == 3)
                {
                    p[y][x][0] = data[i-2];
                    p[y][x][1] = data[i-1];
                    p[y][x][2] = data[i];
                }
                // RGBA
                else if (N == 4)
                {
                    p[y][x][0] = data[i-2];
                    p[y][x][1] = data[i-1];
                    p[y][x][2] = data[i];
                    p[y][x][3] = 255; // TODO: load RGBA images properly
                }

                // Increase y every time we get to end of row
                if (x+1 == w)
                {
                    x=0;
                    ++y;
                }
                else
                {
                    ++x;
                }
            }

            loaded = true;
            delete[] data;
        }
        else
        {
            ilDeleteImages(1, &name);
            throw std::runtime_error("could not read image");
        }

        ilDeleteImages(1, &name);
    }

    // After loading, determine the real gray shade to view this as a black and white
    // image. We'll be using this constantly, so we might as well do it now.
    const Histogram<N> h(p);
    gray_shade = h.threshold(gray_shade);
}

// Initialize all the pixels from a vector, copying the vector
template<int N>
Pixels<N>::Pixels(const std::vector<std::vector<std::array<unsigned char, N>>>& pixels,
            const std::string& fn)
    :w(0), h(0), loaded(false), fn(fn), gray_shade(GRAY_SHADE)
{
    if (pixels.size() > 0 && pixels[0].size() > 0)
    {
        w = pixels[0].size();
        h = pixels.size();
        p = pixels;
        loaded = true;
    }
}

// Initialize all the pixels from a vector, moving the vector
template<int N>
Pixels<N>::Pixels(std::vector<std::vector<std::array<unsigned char, N>>>&& pixels,
            const std::string& fn)
    :w(0), h(0), loaded(false), fn(fn), gray_shade(GRAY_SHADE)
{
    if (pixels.size() > 0 && pixels[0].size() > 0)
    {
        w = pixels[0].size();
        h = pixels.size();
        p = std::move(pixels);
        loaded = true;

        const Histogram<N> h(p);
        gray_shade = h.threshold(gray_shade);
    }
}

// Quantize the image
//
// TODO: don't use this terrible algorithm, instead do some sort of clustering algorithm
//   http://en.wikipedia.org/wiki/Color_quantization
//   http://en.wikipedia.org/wiki/Cluster_analysis
template<int N>
Pixels<N> Pixels<N>::quantize(const int amount) const
{
    // only works with amount > 2
    if (amount < 2)
        return Pixels();

    std::vector<std::vector<std::array<unsigned char, N>>> pixels(
            h, std::vector<std::array<unsigned char, N>>(w, make_array<N, unsigned char>(0)));

    // Quantize the image by rounding the pixels into the "amount" number of
    // bins, using amount-1 to get "amount" instead of amount+1.
    double divisor = 256/(amount-1);

    // Based on each channel value
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            for (int channel = 0; channel < N; ++channel)
                pixels[y][x][channel] = std::floor(p[y][x][channel]/divisor)*divisor;

    /*
    // Based on grayscale average
    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            int sum = 0;

            for (int i = 0; i < N; ++i)
                sum += p[y][x][i];

            unsigned char val = std::floor(sum/3.0/divisor)*divisor;

            for (int i = 0; i < N; ++i)
                pixels[y][x][i] = val;
        }
    }
    */

    Pixels<N> quantized(std::move(pixels), fn);

    return quantized;
}

// Blur the image, perfect Gaussian blur
// See: http://blog.ivank.net/fastest-gaussian-blur.html
template<int N>
Pixels<N> Pixels<N>::blurPerfect(const int r) const
{
    // The image is the same if the radius is zero
    if (r < 1)
        return *this;

    std::vector<std::vector<std::array<unsigned char, N>>> pixels(
            h, std::vector<std::array<unsigned char, N>>(w, make_array<N, unsigned char>(0)));

    // Significant radius
    int rs = std::ceil(r * 2.57);

    for(int i = 0; i < h; ++i)
    {
        for(int j = 0; j < w; ++j)
        {
            for (int channel = 0; channel < N; ++channel)
            {
                double val = 0;
                double wsum = 0;

                for (int iy = i-rs; iy < i+rs+1; ++iy)
                {
                    for (int ix = j-rs; ix < j+rs+1; ++ix)
                    {
                        int x = std::min(w-1, std::max(0, ix));
                        int y = std::min(h-1, std::max(0, iy));
                        double dsq = (ix-j)*(ix-j)+(iy-i)*(iy-i);
                        double weight = std::exp( -dsq / (2*r*r) ) / (M_PI*2*r*r);
                        val += p[y][x][channel] * weight;
                        wsum += weight;
                    }
                }

                pixels[i][j][channel] = std::round(val/wsum);
            }
        }
    }

    Pixels<N> blurred(std::move(pixels), fn);

    return blurred;
}

// Blur the image, fast blur
// See: http://blog.ivank.net/fastest-gaussian-blur.html
template<int N>
Pixels<N> Pixels<N>::blur(const int r) const
{
    // The image is the same if the radius is zero
    if (r < 1)
    {
        log("Not blurring, zero blur radius");
        return *this;
    }

    // Can't have the radius bigger than the width or height
    if (r > w || r > h)
    {
        log("Not blurring, radius greater than image width or height");
        return *this;
    }

    // Check for int overflows
    if (255*w > std::numeric_limits<int>::max() || 255*h > std::numeric_limits<int>::max())
        log("Possible integer overflow while blurring", LogType::Warning);

    PixelArray copy(p);
    PixelArray output(h, std::vector<std::array<unsigned char, N>>(
                w, make_array<N, unsigned char>(0)));

    gaussBlur_4(copy, output, r);
    Pixels<N> blurred(std::move(output), fn);

    return blurred;
}

// sigma = standard deviation, n = number of boxes
template<int N>
std::vector<int> Pixels<N>::boxesForGauss(int sigma, int n) const
{
    // Ideal averaging filter width
    double wIdeal = std::sqrt((12.0*sigma*sigma/n)+1);

    int wl = std::floor(wIdeal);

    if (wl%2 == 0)
        wl--;

    int wu = wl+2;
    double mIdeal = (12.0*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
    int m = std::round(mIdeal);

    std::vector<int> sizes(n);

    for (int i = 0; i < n; ++i)
        sizes[i] = (i<m)?wl:wu;

    return sizes;
}

template<int N>
void Pixels<N>::gaussBlur_4(PixelArray& scl, PixelArray& tcl, int r) const
{
    std::vector<int> boxes = boxesForGauss(r, 3);
    boxBlur_4(scl, tcl, std::round((1.0*boxes[0]-1)/2));
    boxBlur_4(tcl, scl, std::round((1.0*boxes[1]-1)/2));
    boxBlur_4(scl, tcl, std::round((1.0*boxes[2]-1)/2));
}

template<int N>
void Pixels<N>::boxBlur_4(PixelArray& scl, PixelArray& tcl, int r) const
{
    tcl = scl;
    boxBlurH_4(tcl, scl, r);
    boxBlurT_4(scl, tcl, r);
}

template<int N>
void Pixels<N>::boxBlurH_4(const PixelArray& scl, PixelArray& tcl, int r) const
{
    double iarr = 1.0 / (r+r+1);

    for (int i = 0; i < h; ++i)
    {
        for (int channel = 0; channel < N; ++channel)
        {
            int ti = 0;
            int li = 0;
            int ri = r;
            unsigned char fv = scl[i][0][channel];
            unsigned char lv = scl[i][w-1][channel];
            int val = (r+1)*fv;

            for (int j = 0; j < r; ++j)
            {
                val += scl[i][j][channel];
            }

            for (int j = 0; j <= r && ri < w && ti < w; ++j)
            {
                val += scl[i][ri++][channel] - fv;
                tcl[i][ti++][channel] = std::round(val*iarr);
            }

            for (int j = r+1; j < w-r && ri < w && li < w && ti < w; ++j)
            {
                val += scl[i][ri++][channel] - scl[i][li++][channel];
                tcl[i][ti++][channel] = std::round(val*iarr);
            }

            for (int j = w-r; j < w && li < w && ti < w; ++j)
            {
                val += lv - scl[i][li++][channel];
                tcl[i][ti++][channel] = std::round(val*iarr);
            }
        }
    }
}

template<int N>
void Pixels<N>::boxBlurT_4(const PixelArray& scl, PixelArray& tcl, int r) const
{
    double iarr = 1.0 / (r+r+1);

    for (int i = 0; i < w; ++i)
    {
        for (int channel = 0; channel < N; ++channel)
        {
            int ti = 0;
            int li = 0;
            int ri = r;
            unsigned char fv = scl[0][i][channel];
            unsigned char lv = scl[h-1][i][channel];
            int val = (r+1)*fv;

            for (int j = 0; j < r; ++j)
            {
                val += scl[j][i][channel];
            }

            for (int j = 0; j <= r && ri < h && ti < h; ++j)
            {
                val += scl[ri++][i][channel] - fv;
                tcl[ti++][i][channel] = std::round(val*iarr);
            }

            for (int j = r+1; j < h-r && ri < h && li < h && ti < h; ++j)
            {
                val += scl[ri++][i][channel] - scl[li++][i][channel];
                tcl[ti++][i][channel] = std::round(val*iarr);
            }

            for (int j = h-r; j < h && li < h && ti < h; ++j)
            {
                val += lv - scl[li++][i][channel];
                tcl[ti++][i][channel] = std::round(val*iarr);
            }
        }
    }
}

template<int N>
void Pixels<N>::mark(const Coord& c, int size)
{
    if (c.x >= 0 && c.y >= 0 &&
        c.x < w  && c.y < h)
        marks.push_back(Mark(c, size));
}

template<int N>
void Pixels<N>::line(const Coord& p1, const Coord& p2)
{
    const int min_x = std::min(p1.x, p2.x);
    const int max_x = std::max(p1.x, p2.x);
    const int min_y = std::min(p1.y, p2.y);
    const int max_y = std::max(p1.y, p2.y);

    // One for more vertical, one for more horizontal
    if (min_x == max_x || (max_y-min_y) > (max_x-min_x))
        for (int y = min_y; y <= max_y; ++y)
            mark(Coord(lineFunctionX(p1, p2, y), y), 1);
    else
        for (int x = min_x; x <= max_x; ++x)
            mark(Coord(x, lineFunctionY(p1, p2, x)), 1);
}

template<int N>
void Pixels<N>::save(const std::string& filename, bool show_marks, bool dim, OutputColor color) const
{
    // We'll use the default color unless we're dimming the image. Then we'll use
    // black since the default color might blend in.
    unsigned char mark_color = MARK_COLOR;

    // If the image is grayscale and we're trying to output color, change the
    // output to be grayscale
    if (N == 1 && color == OutputColor::Color)
        color = OutputColor::Grayscale;

    // Output 3 channels with color, 1 for black and white or grayscale
    int channels = (color == OutputColor::Color)?3:1;

    // Work on a separate copy of this image
    std::vector<std::vector<std::vector<unsigned char>>> copy(
            h, std::vector<std::vector<unsigned char>>(
                w, std::vector<unsigned char>(channels, 0)));

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            // If grayscale input, just copy the image
            if (N == 1)
            {
                copy[y][x][0] = p[y][x][0];
            }
            // If color, copy all the channels
            else if (color == OutputColor::Color)
            {
                for (int i = 0; i < channels; ++i)
                    copy[y][x][i] = p[y][x][i];
            }
            // If color and we want grayscale or black and white, then average
            // the channels
            else
            {
                double sum = 0;

                // 3 instead of N since with RGBA we ignore A
                for (int i = 0; i < 3; ++i)
                    sum += p[y][x][i];

                copy[y][x][0] = smartFloor(sum/N);
            }
        }
    }

    // Converting both at once is faster
    if (dim && color == OutputColor::BlackAndWhite)
    {
        mark_color = 0;

        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                copy[y][x][0] = (copy[y][x][0]>gray_shade)?255:170; // 255-255/3 = 170
    }
    // Convert to black and white
    else if (color == OutputColor::BlackAndWhite)
    {
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                copy[y][x][0] = (copy[y][x][0]>gray_shade)?255:0;
    }
    // Dim the image
    else if (dim)
    {
        mark_color = 0;

        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                for (int i = 0; i < channels; ++i)
                    copy[y][x][i] = 170 + copy[y][x][i]/3; // 255-255/3 = 170
    }

    // Draw the marks on a copy of the image
    if (show_marks)
    {
        for (const Mark& m : marks)
        {
            if (m.size > 1)
            {
                // Left
                for (int i = m.coord.x; i > m.coord.x-m.size && i >= 0; --i)
                    for (int j = 0; j < channels; ++j)
                        copy[m.coord.y][i][j] = mark_color;
                // Right
                for (int i = m.coord.x; i < m.coord.x+m.size && i < w; ++i)
                    for (int j = 0; j < channels; ++j)
                        copy[m.coord.y][i][j] = mark_color;
                // Up
                for (int i = m.coord.y; i > m.coord.y-m.size && i >= 0; --i)
                    for (int j = 0; j < channels; ++j)
                        copy[i][m.coord.x][j] = mark_color;
                // Down
                for (int i = m.coord.y; i < m.coord.y+m.size && i < h; ++i)
                    for (int j = 0; j < channels; ++j)
                        copy[i][m.coord.x][j] = mark_color;
            }
            else
            {
                for (int j = 0; j < channels; ++j)
                    copy[m.coord.y][m.coord.x][j] = mark_color;
            }
        }
    }

    // One thread again
    std::unique_lock<std::mutex> lck(lock);

    // Convert this back to a real black and white image
    ILuint name;
    ilGenImages(1, &name);
    ilBindImage(name);
    ilEnable(IL_FILE_OVERWRITE);

    // 3 because IL_RGB
    const int total = w*h*3;
    unsigned char* data = new unsigned char[total];

    // Position in data
    int pos = 0;

    // For some reason the image is flipped vertically when loading it with
    // ilTexImage, so start at the bottom and go up.
    for (int y = h-1; y >= 0; --y)
    {
        for (int x = 0; x < w; ++x)
        {
            // Output RGB if color output otherwise just the grayscale or B&W channel
            if (color == OutputColor::Color)
            {
                data[pos]   = copy[y][x][0];
                data[pos+1] = copy[y][x][1];
                data[pos+2] = copy[y][x][2];
            }
            else
            {
                data[pos]   = copy[y][x][0];
                data[pos+1] = copy[y][x][0];
                data[pos+2] = copy[y][x][0];
            }

            // For R, G, and B that we just set
            pos+=3;
        }
    }

    ilTexImage(w, h, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, data);

    if (!ilSaveImage(filename.c_str()) || ilGetError() == IL_INVALID_PARAM)
    {
        delete[] data;
        throw std::runtime_error("could not save image");
    }

    ilDeleteImages(1, &name);
    delete[] data;
}

template<int N>
Coord Pixels<N>::rotatePoint(const Coord& origin, const Coord& c,
        double sin_rad, double cos_rad) const
{
    // Translate to origin
    const int trans_x = c.x - origin.x;
    const int trans_y = c.y - origin.y;

    // Rotate + translate back
    // Using std::round seems to make them closer to what is expected
    const int new_x = std::round(trans_x*cos_rad + trans_y*sin_rad) + origin.x;
    const int new_y = std::round(trans_y*cos_rad - trans_x*sin_rad) + origin.y;

    if (new_x >= 0 && new_y >= 0 &&
        new_x < w && new_y < h)
        return Coord(new_x, new_y);

    return default_coord;
}

// In the future, it may be a good idea to implement something like the "Rotation by
// Area Mapping" talked about on http://www.leptonica.com/rotation.html
template<int N>
void Pixels<N>::rotate(double rad, const Coord& point)
{
    // Right size, default to white (255 or 1111 1111)
    std::vector<std::vector<std::array<unsigned char, N>>> copy(
            h, std::vector<std::array<unsigned char, N>>(w, make_array<N, unsigned char>(0xff)));

    // -rad because we're calculating the rotation to get from the new rotated
    // image to the original image. We're walking the new image instead of the
    // original so as to not get blank spots from rounding.
    const double sin_rad = std::sin(-rad);
    const double cos_rad = std::cos(-rad);

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            const Coord c = rotatePoint(point, Coord(x,y), sin_rad, cos_rad);

            if (c != default_coord)
                copy[y][x] = p[c.y][c.x];
        }
    }

    p = copy;

    // Rotate marks as well. This time we'll rotate to the new image, calculating the new
    // point instead of looking for what goes at every pixel in the new image.
    const double mark_sin_rad = std::sin(rad);
    const double mark_cos_rad = std::cos(rad);

    for (Mark& m : marks)
    {
        const Coord c = rotatePoint(point, m.coord, mark_sin_rad, mark_cos_rad);

        if (c != default_coord)
            m.coord = c;
    }
}

// Rotate all points in a vector (more or less the same as rotating the image)
// This is in Pixels since it uses the width and height of an image
template<int N>
void Pixels<N>::rotateVector(std::vector<Coord>& v, const Coord& point, double rad) const
{
    const double sin_rad = std::sin(rad);
    const double cos_rad = std::cos(rad);

    for (Coord& m : v)
    {
        const Coord c = rotatePoint(point, m, sin_rad, cos_rad);

        if (c != default_coord)
            m = c;
    }
}

#endif
