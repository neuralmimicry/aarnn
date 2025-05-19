// VisualProcessor.cpp
#include "VisualProcessor.h"
#include "StimuliData.h"
#include <opencv2/opencv.hpp>
#include <iostream>

VisualProcessor::VisualProcessor(const std::string& host, unsigned short port, const std::string& id)
        : id(id), processing(false), healthy(false) {
    networkClient = std::make_unique<AsyncNetworkClient>(host, port);
}

VisualProcessor::~VisualProcessor() {
    stopProcessing();
}

bool VisualProcessor::initialise() {
    healthy = networkClient->connect();
    if (!healthy) {
        std::cerr << "VisualProcessor: Failed to connect to sensory receptor server." << std::endl;
    }
    return healthy;
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
        networkClient->disconnect();
    }
}

bool VisualProcessor::isHealthy() const {
    return healthy.load();
}

void VisualProcessor::setVisualReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    visualReceptors = receptors;
}

void VisualProcessor::captureVisualData() {
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        processing = false;
        return;
    }

    while (processing.load()) {
        cv::Mat frame;
        if (!cap.read(frame)) {
            continue;
        }

        processVisualData(frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
}

void VisualProcessor::processVisualData(cv::Mat& frame) {
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    double averageIntensity = cv::mean(gray)[0];
    stimulateReceptors(averageIntensity);

    StimuliData data;
    data.receptorType = "Visual:" + id;
    data.values = { averageIntensity };

    std::string json = serializeStimuliData(data);
    try {
        networkClient->send(json);
        healthy = true;
    } catch (...) {
        std::cerr << "VisualProcessor: Failed to send data. Marking as unhealthy." << std::endl;
        healthy = false;
    }
}

void VisualProcessor::stimulateReceptors(double intensity) {
    for (auto& receptor : visualReceptors) {
        receptor->stimulate(intensity);
    }
}
