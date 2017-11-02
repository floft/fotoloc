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
