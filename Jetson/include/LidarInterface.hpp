#ifndef LIDAR_INTERFACE_HPP
#define LIDAR_INTERFACE_HPP

#include <string>
#include <fstream>

class LidarInterface {
public:
    explicit LidarInterface(const std::string& devicePath,
                            const std::string& logFilePath);
    ~LidarInterface();

    bool initialize();
    bool readAndLog();
    void shutdown();

private:
    int serialFd;
    std::string device;
    std::ofstream logFile;

    bool configureSerialPort();
    double getTimestampSeconds() const;
};

#endif // LIDAR_INTERFACE_HPP

