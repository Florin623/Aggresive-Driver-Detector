#include "LidarInterface.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <iostream>
#include <chrono>

LidarInterface::LidarInterface(const std::string& devicePath,
                               const std::string& logFilePath)
    : serialFd(-1), device(devicePath) {
    logFile.open(logFilePath);
    logFile << "timestamp_seconds,distance_meters\n";
}

LidarInterface::~LidarInterface() {
    shutdown();
}

bool LidarInterface::initialize() {
    serialFd = open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (serialFd < 0) {
        std::cerr << "Failed to open LiDAR device\n";
        return false;
    }
    return configureSerialPort();
}

bool LidarInterface::configureSerialPort() {
    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(serialFd, &tty) != 0) {
        return false;
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag |= CLOCAL | CREAD;
    tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS);

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_lflag = 0;
    tty.c_oflag = 0;

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 10;

    return tcsetattr(serialFd, TCSANOW, &tty) == 0;
}

double LidarInterface::getTimestampSeconds() const {
    using clock = std::chrono::steady_clock;
    auto now = clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}

bool LidarInterface::readAndLog() {
    char buffer[64];
    int bytesRead = read(serialFd, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        return false;
    }

    buffer[bytesRead] = '\0';

    int distanceMm = 0;
    if (sscanf(buffer, "D:%d", &distanceMm) == 1) {
        double timestamp = getTimestampSeconds();
        double distanceMeters = distanceMm / 1000.0;
        logFile << timestamp << "," << distanceMeters << "\n";
        return true;
    }
    return false;
}

void LidarInterface::shutdown() {
    if (serialFd >= 0) {
        close(serialFd);
        serialFd = -1;
    }
    if (logFile.is_open()) {
        logFile.close();
    }
}

