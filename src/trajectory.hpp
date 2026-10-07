#pragma once
#include <algorithm>
#include <cmath>
#include <optional>

namespace fc {
struct Vec { double x, y, z; };
struct Limits { double drop = 1000; double horizontal = -1; };
struct Flight { double time, vertical; bool repaired; };

// Upright MovePattern 2 uses g=980 and horizontal acceleration at most 500.
// Retain its outer envelope and existing throws; extend only into its hollow centre.
inline std::optional<Flight> solve(Vec delta, Limits limits, bool remove_centre) {
    constexpr double g = 980, acceleration = 500, max_vertical = 700;
    if (!std::isfinite(delta.x) || !std::isfinite(delta.y) || !std::isfinite(delta.z)) return {};
    double r = std::hypot(delta.x, delta.y), z = delta.z;
    if (z < -std::clamp(limits.drop, 0., 1000.) || (limits.horizontal >= 0 && r > limits.horizontal)) return {};
    double t = std::sqrt(2*r/acceleration);
    if (t > 0) {
        double v = z/t + g*t/2;
        if (v >= 0 && v <= max_vertical && v-g*t < 0) return Flight{t,v,false};
    }
    if (!remove_centre || z >= max_vertical*max_vertical/(2*g)) return {};
    double upper = (max_vertical + std::sqrt(max_vertical*max_vertical - 2*g*z))/g;
    if (t > upper) return {};
    if (z < 0) t = std::max(t, std::sqrt(-2*z/g));
    else if (z > 0) {
        double apex = std::sqrt(2*z/g);
        if (t <= apex) t = apex + (upper-apex)*.01;
    } else if (t == 0) t = .1;
    if (t <= 0 || t > upper) return {};
    double v = z/t + g*t/2;
    // Roundoff at v=0 must not produce a tiny negative launch speed.
    return Flight{t,std::max(0.,v),true};
}
}
