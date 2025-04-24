// VisualProcessor.h

#pragma once

#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <opencv2/core/mat.hpp>
#include "SensoryReceptor.h"

class VisualProcessor {
public:
    VisualProcessor();
    ~VisualProcessor();

    bool initialise();
    void startProcessing();
    void stopProcessing();

    void setVisualReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors);

private:
    std::vector<std::shared_ptr<SensoryReceptor>> visualReceptors;

    // Thread management
    std::atomic<bool> processing;
    std::thread processingThread;

    // Internal methods
    void captureVisualData();

    // Helper methods
    void stimulateReceptors(/* processed data */);

    void processVisualData(cv::Mat &frame);

    void stimulateReceptors(double intensity);
};
