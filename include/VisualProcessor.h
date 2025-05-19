// VisualProcessor.h
#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <string>
#include <opencv2/core/mat.hpp>
#include "SensoryReceptor.h"
#include "AsyncNetworkClient.h"

// VisualProcessor captures frames from a video source, processes them,
// and transmits data over the network via AsyncNetworkClient.
class VisualProcessor {
public:
    VisualProcessor(const std::string& host, unsigned short port, const std::string& id);
    ~VisualProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    bool isHealthy() const;
    void setVisualReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::string id;
    std::atomic<bool> processing;
    std::atomic<bool> healthy;
    std::thread processingThread;

    std::unique_ptr<AsyncNetworkClient> networkClient;
    std::vector<std::shared_ptr<SensoryReceptor>> visualReceptors;

    void captureVisualData();
    void processVisualData(cv::Mat& frame);
    void stimulateReceptors(double intensity);
};
