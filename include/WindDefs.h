//
// Created by jammer on 28/03/17.
//

#ifndef SINGING_WIND_WINDDEFS_H
#define SINGING_WIND_WINDDEFS_H

#include <SFML/Graphics.hpp>
#include <math.h>
#include <bitset>
#include <deque>

using WVec = sf::Vector2f;
using WTransform = sf::Transform;
using bset = std::bitset<12>;

inline std::ostream& operator <<(std::ostream& os, const WVec &v) {
    os << "(" <<v.x << ", " << v.y << ")";
    return os;
}

inline float w_magnitude(const WVec &v) {
    return sqrtf(v.x * v.x + v.y * v.y);
}

inline WVec w_normalize(const WVec &v) {
    if ((v.x * v.x + v.y * v.y) != 0) {
        return v / w_magnitude(v);
    }
    return v;
}

inline float w_dot(const WVec &a, const WVec &b) {
    return a.x * b.x + a.y * b.y;
}

inline float w_cross(const WVec &a, const WVec &b) {
    return a.x * b.y - a.y * b.x;
}

inline WVec w_triple_prod(const WVec &a, const WVec &b, const WVec &c) {
    return b * w_dot(a, c) - c * w_dot(a, b);
}

inline WVec w_slide(const WVec &vec, const WVec &normal) {
    return vec - normal * w_dot(vec, normal);
}

inline float w_angle_to_vec(const WVec &v, const WVec &to) {
    return atan2(w_cross(v, to), w_dot(v, to));
}

inline WVec w_rotated_deg(const WVec &v, float angle) {
    float rad_angle = static_cast<float>(angle * M_PI / 180.f);
    float v_angle = atan2(v.y, v.x);
    return WVec(cos(v_angle + rad_angle), sin(v_angle + rad_angle)) * w_magnitude(v);
}

inline WVec w_tangent(const WVec &v) {
    return WVec(v.y, -v.x);
}

template<typename T>
void clear_arr(std::deque<T> &arr, const T &val) {
    for (unsigned int i = 0; i < arr.size(); ++i) {
        arr[i] = val;
    }
}

#endif //SINGING_WIND_WINDDEFS_H
