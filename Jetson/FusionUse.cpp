#include "sensor_fusion.hpp"
#include <iostream>

int main()
{
    // Example uncertainty estimates (experimentally determined)
    constexpr double lidar_variance  = 0.01;  // meters^2
    constexpr double camera_variance = 0.09;  // meters^2

    // Synchronization tolerance: 10 ms
    constexpr uint64_t max_time_delta = 10'000;

    // Consistency threshold: 1 meter
    constexpr double consistency_thr = 1.0;

    SensorFusion fusion(lidar_variance,
                        camera_variance,
                        max_time_delta,
                        consistency_thr);

    DistanceMeasurement lidar   {12.5, 1'000'000};
    DistanceMeasurement camera  {12.1, 1'002'000};

    FusedEstimate estimate = fusion.fuse(lidar, camera);

    if (estimate.valid)
    {
        std::cout << "Fused distance: "
                  << estimate.value << " m\n";
    }
    else
    {
        std::cout << "Fusion failed\n";
    }

    return 0;
}

