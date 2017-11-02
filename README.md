fotoloc
=======
A family member was given a PDF of many scanned old photographs that were to be
put in a slideshow. She had to manually extract all the photos from each of the
PDF pages, cropping and rotating each one. Because this is a task well suited
to computers, I decided to write software to make this process easier if she or
somebody else wished to do this in the future.

To use this program, type ``make``, and then ``./fotoloc a.png b.pdf ...`` It
will output PNG images named "image0.png", "image1.png", etc. as it recognizes
the images in the input images or PDFs.

### Ideas for going forward... ###
I haven't updated this in 2 years. I'll probably scratch most of my previous
methods and instead investigate some new ideas.

 - Maybe use entropy or something with Lempel-Ziv, looking at how much
   information is in a region of the image. For example, your photos contain
   more information than the white scanner background behind, or similarly your
   photo is not as easy to compress. However, this depends on the type of
   photos you're working with. If your "photo" is a stick figure drawing, it
   might be hard to find the bounds of your image. Hopefully there's a slight
   shadow from the edge of the paper you're scanning?
 - Redo in Python and use OpenCV for some algorithms. Hopefully there already
   exists some PDF image extracting library so I don't have to use what I coded
   for C++ for freetron with PoDoFo.
 - Maybe detect straight lines, look at enclosed regions, look at those with
   highest entropy (or some form of information gain?).

### Dependencies ###
*a C++11 compiler*  
PoDoFo (LGPL)  
OpenIL/DevIL (LGPL)  
libtiff (custom: http://www.libtiff.org/misc.html)  

### Example ###
*Will be added once it works...*

### License ###
The majority of this code is licensed under the
[MIT](http://floft.net/files/mit-license.txt) license. In addition, this
software includes code from:

Ivan Kuckir, [Fast Gaussian Blur](http://blog.ivank.net/fastest-gaussian-blur.html) (MIT)
