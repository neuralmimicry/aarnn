// VisualProcessor.cpp

#include "VisualProcessor.h"
#include <opencv2/opencv.hpp> // OpenCV library for image processing

VisualProcessor::VisualProcessor() : processing(false) {}

VisualProcessor::~VisualProcessor() {
    stopProcessing();
}

bool VisualProcessor::initialise() {
    // Initialize camera or image source
    // For example, open a video capture device
    return true;
}

void VisualProcessor::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&VisualProcessor::captureVisualData, this);
    }
}

void VisualProcessor::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void VisualProcessor::setVisualReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    visualReceptors = receptors;
}

void VisualProcessor::captureVisualData() {
    // Example using OpenCV to capture frames from a camera
    cv::VideoCapture cap(0); // Open default camera
    if (!cap.isOpened()) {
        processing = false;
        return;
    }

    while (processing.load()) {
        cv::Mat frame;
        if (!cap.read(frame)) {
            continue;
        }

        // Process the captured frame
        processVisualData(frame);

        // Sleep briefly to simulate real-time processing
        std::this_thread::sleep_for(std::chrono::milliseconds(30)); // ~33 FPS
    }
}

void VisualProcessor::processVisualData(cv::Mat& frame) {
    // Convert frame to grayscale
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    // For simplicity, we'll average the intensity and stimulate receptors accordingly
    double averageIntensity = cv::mean(gray)[0];

    // Stimulate receptors based on intensity
    stimulateReceptors(averageIntensity);
}

void VisualProcessor::stimulateReceptors(double intensity) {
    // Map intensity to receptors
    for (auto& receptor : visualReceptors) {
        receptor->stimulate(intensity);
    }
}
