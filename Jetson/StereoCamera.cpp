#include "StereoCamera.hpp"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <cstring>
#include <iostream>
#include <chrono>

#define BUFFER_COUNT 4

StereoCamera::StereoCamera(const std::string& leftDevice,
                           const std::string& rightDevice,
                           uint32_t w,
                           uint32_t h,
                           uint32_t f)
    : width(w), height(h), fps(f) {
    leftCam.device = leftDevice;
    rightCam.device = rightDevice;
    leftCam.fd = -1;
    rightCam.fd = -1;
}

StereoCamera::~StereoCamera() {
    shutdown();
}

bool StereoCamera::initialize() {
    if (!openDevice(leftCam) || !openDevice(rightCam)) {
        return false;
    }

    if (!configureDevice(leftCam) || !configureDevice(rightCam)) {
        return false;
    }

    if (!initMMap(leftCam) || !initMMap(rightCam)) {
        return false;
    }

    if (!startStreaming(leftCam) || !startStreaming(rightCam)) {
        return false;
    }

    return true;
}

bool StereoCamera::openDevice(Camera& cam) {
    cam.fd = open(cam.device.c_str(), O_RDWR | O_NONBLOCK);
    if (cam.fd < 0) {
        std::cerr << "Failed to open " << cam.device << std::endl;
        return false;
    }
    return true;
}


bool StereoCamera::configureDevice(Camera& cam) {
    v4l2_format fmt{};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(cam.fd, VIDIOC_S_FMT, &fmt) < 0) {
        std::cerr << "Failed to set format on " << cam.device << std::endl;
        return false;
    }

    v4l2_streamparm parm{};
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator = 1;
    parm.parm.capture.timeperframe.denominator = fps;

    ioctl(cam.fd, VIDIOC_S_PARM, &parm);
    return true;
}

bool StereoCamera::initMMap(Camera& cam) {
    v4l2_requestbuffers req{};
    req.count = BUFFER_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(cam.fd, VIDIOC_REQBUFS, &req) < 0) {
        return false;
    }

    cam.buffers.resize(req.count);
    cam.bufferSizes.resize(req.count);

    for (uint32_t i = 0; i < req.count; ++i) {
        v4l2_buffer buf{};
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        ioctl(cam.fd, VIDIOC_QUERYBUF, &buf);

        cam.bufferSizes[i] = buf.length;
        cam.buffers[i] = mmap(nullptr, buf.length,
                               PROT_READ | PROT_WRITE,
                               MAP_SHARED, cam.fd, buf.m.offset);

        ioctl(cam.fd, VIDIOC_QBUF, &buf);
    }
    return true;
}

bool StereoCamera::startStreaming(Camera& cam) {
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    return ioctl(cam.fd, VIDIOC_STREAMON, &type) == 0;
}

bool StereoCamera::readFrame(Camera& cam) {
    v4l2_buffer buf{};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(cam.fd, VIDIOC_DQBUF, &buf) < 0) {
        return false;
    }

    double timestamp = getTimestampSeconds();

    // Frame data is available at cam.buffers[buf.index]
    // Size: cam.bufferSizes[buf.index]

    // (Here you would pass the frame to processing / NN pipeline)

    ioctl(cam.fd, VIDIOC_QBUF, &buf);
    return true;
}

bool StereoCamera::captureFrame() {
    bool leftOk = readFrame(leftCam);
    bool rightOk = readFrame(rightCam);

    return leftOk && rightOk;
}


double StereoCamera::getTimestampSeconds() const {
    using clock = std::chrono::steady_clock;
    auto now = clock::now().time_since_epoch();
    return std::chrono::duration<double>(now).count();
}


void StereoCamera::stopStreaming(Camera& cam) {
    v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(cam.fd, VIDIOC_STREAMOFF, &type);
}

void StereoCamera::shutdown() {
    stopStreaming(leftCam);
    stopStreaming(rightCam);

    for (auto& cam : {&leftCam, &rightCam}) {
        for (size_t i = 0; i < cam->buffers.size(); ++i) {
            munmap(cam->buffers[i], cam->bufferSizes[i]);
        }
        if (cam->fd >= 0) {
            close(cam->fd);
        }
    }
}
