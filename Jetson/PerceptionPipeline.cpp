#include <iostream>
#include <chrono>

#include "lidar_interface.hpp"
#include "camera_interface.hpp"
#include "nn_inference.hpp"
#include "sensor_fusion.hpp"

using Clock = std::chrono::steady_clock;

/**
 * Helper function to get current timestamp in microseconds
 */
uint64_t get_timestamp_us()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(
        Clock::now().time_since_epoch()).count();
}

int main()
{
    // ---------- Initialize LiDAR ----------
    LiDARInterface lidar("/dev/ttyUSB0");
    if (!lidar.initialize())
    {
        std::cerr << "LiDAR initialization failed\n";
        return -1;
    }

    // ---------- Initialize Camera ----------
    CameraInterface camera(0); // CSI camera index
    if (!camera.initialize())
    {
        std::cerr << "Camera initialization failed\n";
        return -1;
    }

    // ---------- Initialize Neural Network ----------
    NeuralNetwork nn("distance_model.tflite");
    if (!nn.initialize())
    {
        std::cerr << "NN initialization failed\n";
        return -1;
    }

    // ---------- Initialize Fusion ----------
    SensorFusion fusion(
        0.01,     // LiDAR variance
        0.09,     // Camera variance
        10000,    // 10 ms synchronization window
        1.0       // 1 meter consistency threshold
    );

    std::cout << "System initialized successfully\n";

    // ---------- Main Loop ----------
    while (true)
    {
        // --- LiDAR acquisition ---
        double lidar_distance = lidar.read_distance();
        uint64_t t_lidar = get_timestamp_us();

        DistanceMeasurement lidar_meas{
            lidar_distance,
            t_lidar
        };

        // --- Camera acquisition ---
        cv::Mat frame;
        if (!camera.capture(frame))
            continue;

        // --- Neural network inference ---
        double camera_distance = nn.predict(frame);
        uint64_t t_camera = get_timestamp_us();

        DistanceMeasurement camera_meas{
            camera_distance,
            t_camera
        };

        // --- Sensor fusion ---
        FusedEstimate fused = fusion.fuse(lidar_meas, camera_meas);

        if (fused.valid)
        {
            std::cout << "Fused distance: "
                      << fused.value << " m\n";
        }
        else
        {
            std::cout << "Fusion invalid\n";
        }
    }

    return 0;
}

