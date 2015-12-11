/*
 * Random useful things
 */

#include <array>

// Useful for initiailizing a vector of arrays
//
// From: http://stackoverflow.com/a/17923795/2698494
template <size_t N, class T>
std::array<T, N> make_array(const T& v) {
    std::array<T, N> a;
    a.fill(v);
    return a;
}
