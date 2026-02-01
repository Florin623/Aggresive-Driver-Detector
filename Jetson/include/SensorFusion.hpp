#ifndef SENSOR_FUSION_HPP
#define SENSOR_FUSION_HPP

#include <cstdint>
#include <optional>

/**
 * @brief Container for a distance measurement with timestamp.
 */
struct DistanceMeasurement
{
    double value;       // Measured distance [meters]
    uint64_t timestamp; // Timestamp [microseconds]
};

/**
 * @brief Fused distance estimate.
 */
struct FusedEstimate
{
    double value;       // Fused distance [meters]
    bool valid;         // Indicates whether fusion was successful
};

/**
 * @brief Sensor fusion class implementing weighted linear fusion.
 */
class SensorFusion
{
public:
    /**
     * @param lidar_variance   Estimated variance of LiDAR measurements
     * @param camera_variance  Estimated variance of camera NN estimates
     * @param max_time_delta   Maximum allowed timestamp difference [µs]
     * @param consistency_thr  Maximum allowed measurement disagreement [m]
     */
    SensorFusion(double lidar_variance,
                 double camera_variance,
                 uint64_t max_time_delta,
                 double consistency_thr);

    /**
     * @brief Fuse LiDAR and camera distance estimates.
     */
    FusedEstimate fuse(const DistanceMeasurement& lidar,
                       const DistanceMeasurement& camera) const;

private:
    double sigma_l2_;     // LiDAR variance
    double sigma_c2_;     // Camera variance
    uint64_t max_dt_;     // Synchronization tolerance
    double consistency_; // Consistency threshold
};

#endif // SENSOR_FUSION_HPP

