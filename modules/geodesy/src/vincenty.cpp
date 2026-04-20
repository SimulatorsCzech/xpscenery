#include "xpscenery/geodesy/vincenty.hpp"

#include <cmath>
#include <format>
#include <numbers>

namespace xps::geodesy
{

    namespace
    {

        // WGS84 ellipsoid constants.
        constexpr double kA = 6378137.0;           // semi-major axis, meters
        constexpr double kF = 1.0 / 298.257223563; // flattening
        constexpr double kB = kA * (1.0 - kF);     // semi-minor axis

        inline double deg2rad(double d) noexcept { return d * std::numbers::pi / 180.0; }
        inline double rad2deg(double r) noexcept { return r * 180.0 / std::numbers::pi; }

        inline double normalize_deg(double d) noexcept
        {
            double v = std::fmod(d, 360.0);
            if (v < 0.0)
                v += 360.0;
            return v;
        }

    } // namespace

    std::expected<InverseResult, std::string>
    vincenty_inverse(const xps::core_types::LatLon &a,
                     const xps::core_types::LatLon &b) noexcept
    {
        if (!a.is_valid() || !b.is_valid())
        {
            return std::unexpected(std::format(
                "vincenty_inverse: invalid input ({} , {})",
                a.to_string(), b.to_string()));
        }

        const double L = deg2rad(b.lon() - a.lon());
        const double U1 = std::atan((1.0 - kF) * std::tan(deg2rad(a.lat())));
        const double U2 = std::atan((1.0 - kF) * std::tan(deg2rad(b.lat())));

        const double sinU1 = std::sin(U1), cosU1 = std::cos(U1);
        const double sinU2 = std::sin(U2), cosU2 = std::cos(U2);

        double lambda = L;
        double lambda_prev;
        double sinLambda = 0.0, cosLambda = 0.0;
        double sinSigma = 0.0, cosSigma = 0.0, sigma = 0.0;
        double sinAlpha = 0.0, cos2Alpha = 0.0, cos2SigmaM = 0.0;
        double C = 0.0;

        int iter = 0;
        constexpr int kMaxIter = 200;
        do
        {
            sinLambda = std::sin(lambda);
            cosLambda = std::cos(lambda);
            const double t1 = cosU2 * sinLambda;
            const double t2 = cosU1 * sinU2 - sinU1 * cosU2 * cosLambda;
            sinSigma = std::sqrt(t1 * t1 + t2 * t2);
            if (sinSigma == 0.0)
            {
                // Coincident points
                InverseResult r;
                r.distance_m = 0.0;
                r.initial_bearing_deg = 0.0;
                r.final_bearing_deg = 0.0;
                r.iterations = iter;
                return r;
            }
            cosSigma = sinU1 * sinU2 + cosU1 * cosU2 * cosLambda;
            sigma = std::atan2(sinSigma, cosSigma);
            sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;
            cos2Alpha = 1.0 - sinAlpha * sinAlpha;
            cos2SigmaM = (cos2Alpha == 0.0)
                             ? 0.0
                             : (cosSigma - 2.0 * sinU1 * sinU2 / cos2Alpha);
            C = kF / 16.0 * cos2Alpha * (4.0 + kF * (4.0 - 3.0 * cos2Alpha));
            lambda_prev = lambda;
            lambda = L + (1.0 - C) * kF * sinAlpha *
                             (sigma + C * sinSigma *
                                          (cos2SigmaM + C * cosSigma *
                                                            (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM)));
            if (++iter >= kMaxIter)
            {
                return std::unexpected(
                    "vincenty_inverse: failed to converge (near-antipodal points?)");
            }
        } while (std::fabs(lambda - lambda_prev) > 1e-12);

        const double u2 = cos2Alpha * (kA * kA - kB * kB) / (kB * kB);
        const double A = 1.0 + u2 / 16384.0 *
                                   (4096.0 + u2 * (-768.0 + u2 * (320.0 - 175.0 * u2)));
        const double B = u2 / 1024.0 * (256.0 + u2 * (-128.0 + u2 * (74.0 - 47.0 * u2)));
        const double deltaSigma =
            B * sinSigma *
            (cos2SigmaM + B / 4.0 *
                              (cosSigma * (-1.0 + 2.0 * cos2SigmaM * cos2SigmaM) -
                               B / 6.0 * cos2SigmaM *
                                   (-3.0 + 4.0 * sinSigma * sinSigma) *
                                   (-3.0 + 4.0 * cos2SigmaM * cos2SigmaM)));
        const double s = kB * A * (sigma - deltaSigma);

        const double alpha1 = std::atan2(cosU2 * sinLambda,
                                         cosU1 * sinU2 - sinU1 * cosU2 * cosLambda);
        const double alpha2 = std::atan2(cosU1 * sinLambda,
                                         -sinU1 * cosU2 + cosU1 * sinU2 * cosLambda);

        InverseResult r;
        r.distance_m = s;
        r.initial_bearing_deg = normalize_deg(rad2deg(alpha1));
        r.final_bearing_deg = normalize_deg(rad2deg(alpha2));
        r.iterations = iter;
        return r;
    }

} // namespace xps::geodesy
