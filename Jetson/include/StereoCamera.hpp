#ifndef STEREO_CAMERA_HPP
#define STEREO_CAMERA_HPP

#include <string>
#include <vector>
#include <linux/videodev2.h>

class StereoCamera {
public:
    StereoCamera(const std::string& leftDevice,
                 const std::string& rightDevice,
                 uint32_t width,
                 uint32_t height,
                 uint32_t fps);

    ~StereoCamera();

    bool initialize();
    bool captureFrame();
    void shutdown();

private:
    struct Camera {
        int fd;
        std::string device;
        std::vector<void*> buffers;
        std::vector<size_t> bufferSizes;
    };

    Camera leftCam;
    Camera rightCam;

    uint32_t width;
    uint32_t height;
    uint32_t fps;

    bool openDevice(Camera& cam);
    bool configureDevice(Camera& cam);
    bool initMMap(Camera& cam);
    bool startStreaming(Camera& cam);
    bool readFrame(Camera& cam);
    void stopStreaming(Camera& cam);
    double getTimestampSeconds() const;
};

#endif

