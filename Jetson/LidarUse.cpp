#include "LidarInterface.hpp"
#include <iostream>

int main() {
    LidarInterface lidar("/dev/ttyUSB0");

    if (!lidar.initialize()) {
        return -1;
    }

    float distance;
    while (true) {
        if (lidar.readDistance(distance)) {
            std::cout << "LiDAR distance: " << distance << " m\n";
        }
        usleep(100000);
    }

    return 0;
}

