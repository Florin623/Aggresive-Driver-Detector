#include "sensor_fusion.hpp"
#include <cmath>

/**
 * Constructor initializes fusion parameters.
 */
SensorFusion::SensorFusion(double lidar_variance,
                           double camera_variance,
                           uint64_t max_time_delta,
                           double consistency_thr)
    : sigma_l2_(lidar_variance),
      sigma_c2_(camera_variance),
      max_dt_(max_time_delta),
      consistency_(consistency_thr)
{
}

/**
 * Perform weighted sensor fusion.
 */
FusedEstimate SensorFusion::fuse(const DistanceMeasurement& lidar,
                                 const DistanceMeasurement& camera) const
{
    FusedEstimate result{};
    result.valid = false;

    // ---- Temporal alignment check ----
    uint64_t dt = std::llabs(
        static_cast<int64_t>(lidar.timestamp) -
        static_cast<int64_t>(camera.timestamp)
    );

    if (dt > max_dt_)
    {
        // Measurements are not temporally consistent
        return result;
    }

    // ---- Consistency check ----
    double diff = std::fabs(lidar.value - camera.value);
    if (diff > consistency_)
    {
        // Measurements disagree excessively; fall back to LiDAR
        result.value = lidar.value;
        result.valid = true;
        return result;
    }

    // ---- Compute fusion weights (minimum-variance estimator) ----
    double w_lidar  = sigma_c2_ / (sigma_l2_ + sigma_c2_);
    double w_camera = sigma_l2_ / (sigma_l2_ + sigma_c2_);

    // ---- Weighted fusion ----
    result.value = w_lidar * lidar.value + w_camera * camera.value;
    result.valid = true;

    return result;
}

