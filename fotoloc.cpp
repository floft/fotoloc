/*
 * Extract images from scanned pages
 */

#include <locale>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

// For loading/saving images
#include <IL/il.h>

// For determining if files exist
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

// Our code
#include "line.h"
#include "math.h"
#include "blobs.h"
#include "outline.h"
#include "pixels.h"
#include "regions.h"

// Get the lowercase extension from filename (the last bit after the .), e.g.
//  a.b.c.JPG would yield 'jpg'
std::string getExt(const std::string& filename)
{
    std::string ext;

    // Go back through the string till the first dot
    for (int i = filename.length()-1; i >= 0; --i)
    {
        if (filename[i] == '.')
            break;

        // Insert this letter lowercase at the beginning
        ext.insert(0, 1, std::tolower(filename[i]));
    }

    return ext;
}

int main(int argc, char* argv[])
{
    ilInit();

    // Get the files to parse
    struct stat info;
    std::vector<std::string> files;

    for (int i = 1; i < argc; ++i)
    {
        if (stat(argv[i], &info) == 0 && (info.st_mode&S_IFREG))
            files.push_back(argv[i]);
        else
            std::cerr << "Warning: " << argv[i] << " not found" << std::endl;
    }

    unsigned int uid = 0;

    // For each of them, extract the images
    for (unsigned int i = 0; i < files.size(); ++i)
    {
        // Load file, from: http://stackoverflow.com/a/18816228/2698494
        std::ifstream file(files[i], std::ios::binary);
        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<char> buffer(size);

        if (!file.read(buffer.data(), size))
        {
            std::cerr << "Warning: couldn't read file \"" << files[i] << "\"" << std::endl;
            continue;
        }

        // Get extension
        ILenum type = ilTypeFromExt(files[i].c_str());

        if (type == IL_TYPE_UNKNOWN)
        {
            std::string extension = getExt(files[i]);

            if (extension == "pdf")
            {
                std::cerr << "Warning: PDF image extraction not implemented yet" << std::endl;
                continue;
            }
            else
            {
                std::cerr << "Warning: not supported file type \"" << files[i] << "\"" << std::endl;
                continue;
            }
        }

        // Output filename
        std::ostringstream s;
        s << "image" << uid << ".png";
        std::ostringstream s_contours;
        s_contours << "image" << uid << "_contours.png";
        /*
        // TODO: remove
        std::ostringstream s_blurred;
        s_blurred << "image" << uid << "_blurred.png";
        std::ostringstream s_quantized;
        s_quantized << "image" << uid << "_quantized.png";
        */
        ++uid;

        // Load image
        const Pixels<3> img(type, &buffer[0], size, files[i]);

        if (!img.valid())
        {
            std::cerr << "Warning: invalid image \"" << files[i] << "\"" << std::endl;
            continue;
        }

        // Recursive rectangular finder
        //std::vector<Rect> regions = findRegions(img);

        //
        // Options
        //
        const int quantizeAmount = 10;
        const int blurAmount = 2;
        const int min_dist = 100;
        const int max_length = 2*img.width()*img.height();

        // Average dist from line between two points as percentage of line length
        const double maxLineError = 0.04;

        // Blur and quantize
        std::cout << "Blur" << std::endl;
        Pixels<3> blurred = img.blur(blurAmount);
        std::cout << "Quantize" << std::endl;
        Pixels<3> quantized = blurred.quantize(quantizeAmount);
        Pixels<3> contours(quantized.ref(), quantized.filename());

        // Detect blobs
        std::cout << "Blobs" << std::endl;
        const Blobs blobs(quantized);

        std::cout << "Outline" << std::endl;
        for (const CoordPair& pair : blobs)
        {
            // This may be the height, width, or diagonal
            double dist = distance(pair.first, pair.last);

            // Get rid of most the really big or really small objects
            if (dist > min_dist)
            {
                // Find the region boundary, i.e. compute the points on the
                // outline of the blob
                const Outline outline(blobs, pair.first, max_length);
                const std::vector<Coord>& points = outline.points();
                //const std::vector<Line> lines = findLinesHalvingExtending(points, maxLineError);
                const std::vector<Line> lines = findLinesExtendingDecreasingError(points, maxLineError);

                for (const Coord& c : points)
                    contours.mark(c, 1);

                for (const Line& line : lines)
                {
                    std::cout << line.p1 << " " << line.p2 <<  " Len: " << line.length << std::endl;
                    quantized.line(line.p1, line.p2);
                    quantized.mark(line.p1);
                    quantized.mark(line.p2);
                }

                /* Naive line detection
                const int maxLines = 6; // Max number of lines for a region
                const int minjump = 500; // Minimum length of straight line
                const int linejump = 100; // Amount to jump when checking for new straight lines
                const int checkjump = 5; // Amount to jump between points between the two points
                const int avgThresh = 10; // Max average distance from line between points
                const int stddevThresh = 10; // Max standard deviation from line between points

                const int size = points.size();

                // The lines for this region
                std::vector<Line> lines;

                // Find the long straight portions of the region boundary
                for (int i = 0; i < size-minjump; i+=linejump)
                {
                    // Start out with the longest line possible
                    for (int j = size-1; j > i+minjump; j-=linejump)
                    {
                        double length = distance(points[i], points[j]);

                        // Skip if the two points are too close together
                        if (length < minjump)
                            continue;

                        // Look at how close the points between points i and j
                        // on this contour fall to the line between i and j
                        std::vector<double> dist;

                        for (int k = i+1; k < j; k+=checkjump)
                            dist.push_back(distance(points[i], points[j], points[k]));

                        double avg = average(dist);
                        double stddev = stdev(dist);

                        // If low average distance and standard deviation, then
                        // this is approximately straight
                        if (avg < avgThresh && stddev < stddevThresh)
                        {
                            lines.push_back(Line(points[i], points[j], length));

                            // Jump extra if we detect a line
                            i += std::max(minjump-linejump, 0);

                            // Exit this inner loop since we found the longest
                            // line with the first point
                            break;
                        }
                    }
                }

                // Sort by length, longest first, so we only process the first
                // few longest lines
                std::sort(lines.begin(), lines.end(), std::greater<Line>());

                for (int i = 0; i < maxLines && i < lines.size(); ++i)
                {
                    quantized.mark(findMidpoint(lines[i].p1, lines[i].p2));
                    quantized.line(lines[i].p1, lines[i].p2);
                }*/
            }
        }

        // Save image
        std::cout << "Saving " << s.str() << std::endl;
        quantized.save(s.str(), true, true, OutputColor::Color);
        std::cout << "Saving " << s_contours.str() << std::endl;
        contours.save(s_contours.str(), true, true, OutputColor::Color);
        /*
        std::cout << "Saving " << s_blurred.str() << std::endl;
        blurred.save(s_blurred.str(), false, false, OutputColor::Color);
        std::cout << "Saving " << s_quantized.str() << std::endl;
        quantized.save(s_quantized.str(), false, false, OutputColor::Color);
        */
    }

    return 0;
}
