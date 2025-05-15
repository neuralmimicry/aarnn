// AuditoryManager.cpp

#include "AuditoryManager.h"
#include <iostream>
#include <alsa/asoundlib.h>
#include <cstring>
#include <functional>
#include <sndfile.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <curl/curl.h> // For YouTube Data API
#include <nlohmann/json.hpp>
#include "SensoryReceptor.h"   // <— for std::shared_ptr<SensoryReceptor>
#include <sstream>             // <— for istringstream in YouTube JSON parse

using json = nlohmann::json;

AuditoryManager::AuditoryManager() : capturing(false), inputSource(InputSource::None) {
    gst_init(nullptr, nullptr); // Initialise GStreamer
}

AuditoryManager::~AuditoryManager() {
    stopCapture();
}

bool AuditoryManager::initialise() {
    // Initialise ALSA or any other required components here
    return true;
}

bool AuditoryManager::selectYouTubeInput(const std::string& apiKey, const std::string& searchQuery) {
    if (apiKey.empty() || searchQuery.empty()) {
        std::cerr << "YouTube API key or search query is empty." << std::endl;
        return false;
    }

    youTubeApiKey = apiKey;
    youTubeSearchQuery = searchQuery;

    // Search for a video
    youTubeVideoUrl = searchYouTubeVideo(apiKey, searchQuery);
    if (youTubeVideoUrl.empty()) {
        std::cerr << "Failed to find a YouTube video for the search query." << std::endl;
        return false;
    }

    inputSource = InputSource::YouTubeStream;
    return true;
}

bool AuditoryManager::selectRTSPInput(const std::string& rtspUrl) {
    if (rtspUrl.empty()) {
        std::cerr << "RTSP URL is empty." << std::endl;
        return false;
    }

    this->rtspUrl = rtspUrl;
    inputSource = InputSource::RTSPStream;
    return true;
}

bool AuditoryManager::selectMicrophoneInput() {
    selectedCaptureDevice = autoSelectFirstAlsaCaptureDevice();
    if (selectedCaptureDevice.empty()) {
        std::cerr << "Failed to select an ALSA capture device." << std::endl;
        return false;
    }

    mic = std::make_shared<PulseAudioMic>(audioQueue);
    if (!mic) {
        std::cerr << "Failed to create PulseAudioMic!" << std::endl;
        return false;
    }

    inputSource = InputSource::Microphone;
    return true;
}

bool AuditoryManager::selectFileInput(const std::string& filePath) {
    // Check if file exists and is accessible
    FILE* file = fopen(filePath.c_str(), "r");
    if (!file) {
        std::cerr << "Failed to open audio file: " << filePath << std::endl;
        return false;
    }
    fclose(file);

    audioFilePath = filePath;
    inputSource = InputSource::AudioFile;
    return true;
}

void AuditoryManager::startCapture() {
    if (capturing.load()) {
        return;
    }
    capturing = true;

    if (inputSource == InputSource::Microphone) {
        micThread = std::thread(&AuditoryManager::captureFromMicrophone, this);
    } else if (inputSource == InputSource::AudioFile) {
        fileThread = std::thread(&AuditoryManager::captureFromFile, this);
    } else if (inputSource == InputSource::RTSPStream) {
        rtspThread = std::thread(&AuditoryManager::captureFromRTSP, this);
    } else if (inputSource == InputSource::YouTubeStream) {
        youTubeThread = std::thread(&AuditoryManager::captureFromYouTube, this);
    } else {
        std::cerr << "No input source selected." << std::endl;
        capturing = false;
    }
}

void AuditoryManager::stopCapture() {
    if (!capturing.load()) {
        return;
    }
    capturing = false;

    if (inputSource == InputSource::Microphone) {
        if (mic) {
            mic->micStop();
        }
        if (micThread.joinable()) {
            micThread.join();
        }
    } else if (inputSource == InputSource::AudioFile) {
        if (fileThread.joinable()) {
            fileThread.join();
        }
    } else if (inputSource == InputSource::RTSPStream) {
        if (rtspThread.joinable()) {
            rtspThread.join();
        }
    } else if (inputSource == InputSource::YouTubeStream) {
        if (youTubeThread.joinable()) {
            youTubeThread.join();
        }
    }
}

void AuditoryManager::setAudioDataCallback(const std::function<void(const std::vector<double>&)>& callback) {
    audioDataCallback = callback;
}

void AuditoryManager::captureFromRTSP() {
    GstElement* pipeline = nullptr;
    GstBus* bus = nullptr;
    GstMessage* msg = nullptr;

    std::string pipelineStr = "rtspsrc location=" + rtspUrl + " ! decodebin ! audioconvert ! audioresample ! appsink name=appsink";

    GError* error = nullptr;
    pipeline = gst_parse_launch(pipelineStr.c_str(), &error);

    if (!pipeline) {
        std::cerr << "Failed to create GStreamer pipeline: " << error->message << std::endl;
        g_error_free(error);
        capturing = false;
        return;
    }

    GstElement* appSink = gst_bin_get_by_name(GST_BIN(pipeline), "appsink");
    GstAppSink* appsink = GST_APP_SINK(appSink);

    gst_app_sink_set_emit_signals(appsink, TRUE);
    gst_app_sink_set_drop(appsink, TRUE);
    gst_app_sink_set_max_buffers(appsink, 1);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    while (capturing.load()) {
        GstSample* sample = gst_app_sink_try_pull_sample(appsink, GST_SECOND);
        if (sample) {
            GstBuffer* buffer = gst_sample_get_buffer(sample);
            GstMapInfo map;
            if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
                // Assuming 16-bit signed integer PCM audio
                const int16_t* data = reinterpret_cast<const int16_t*>(map.data);
                size_t numSamples = map.size / sizeof(int16_t);

                // Convert samples to double and normalize
                std::vector<double> audioData(numSamples);
                for (size_t i = 0; i < numSamples; ++i) {
                    audioData[i] = data[i] / 32768.0; // Normalize to [-1.0, 1.0]
                }

                // Pass the audio data to the callback
                if (audioDataCallback) {
                    audioDataCallback(audioData);
                }

                gst_buffer_unmap(buffer, &map);
            }
            gst_sample_unref(sample);
        } else {
            // No sample received within timeout
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appSink);
    gst_object_unref(pipeline);
}

void AuditoryManager::captureFromMicrophone() {
    if (!mic) {
        std::cerr << "Microphone not initialised." << std::endl;
        return;
    }

    // Wrap PulseAudioMic’s tuple<double,double> callback into our vector<double> callback
    mic->setAudioDataCallback(
            [this](const std::vector<std::tuple<double,double>>& freqMagPairs) {
                std::vector<double> mono;
                mono.reserve(freqMagPairs.size());
                for (auto& p : freqMagPairs) {
                    mono.push_back(std::get<1>(p));  // take the magnitude
                }
                if (audioDataCallback) {
                    audioDataCallback(mono);
                }
            }
    );

    mic->micRun();
}

void AuditoryManager::captureFromFile() {
    // Open the audio file using libsndfile
    SF_INFO sfInfo;
    memset(&sfInfo, 0, sizeof(SF_INFO));
    SNDFILE* sndFile = sf_open(audioFilePath.c_str(), SFM_READ, &sfInfo);
    if (!sndFile) {
        std::cerr << "Failed to open audio file: " << audioFilePath << std::endl;
        capturing = false;
        return;
    }

    const int bufferSize = 1024;
    std::vector<double> buffer(bufferSize * sfInfo.channels);

    while (capturing.load()) {
        sf_count_t numFrames = sf_readf_double(sndFile, buffer.data(), bufferSize);
        if (numFrames > 0) {
            // If the file is stereo, you may need to handle channels separately
            // For simplicity, we can average the channels or select one
            std::vector<double> monoBuffer(numFrames);
            if (sfInfo.channels == 1) {
                // Mono audio
                monoBuffer.assign(buffer.begin(), buffer.begin() + numFrames);
            } else {
                // Stereo or multi-channel audio
                for (sf_count_t i = 0; i < numFrames; ++i) {
                    double sample = 0.0;
                    for (int ch = 0; ch < sfInfo.channels; ++ch) {
                        sample += buffer[i * sfInfo.channels + ch];
                    }
                    sample /= sfInfo.channels;
                    monoBuffer[i] = sample;
                }
            }

            // Pass the audio data to the callback
            if (audioDataCallback) {
                audioDataCallback(monoBuffer);
            }
        } else {
            // End of file reached
            break;
        }
    }

    sf_close(sndFile);
}

std::string AuditoryManager::searchYouTubeVideo(
        const std::string& apiKey,
        const std::string& searchQuery
) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialise CURL.\n";
        return "";
    }

    // Build URL
    std::string base =
            "https://www.googleapis.com/youtube/v3/search?"
            "part=snippet&type=video&maxResults=1&q=";
    char* esc = curl_easy_escape(curl, searchQuery.c_str(), searchQuery.length());
    std::string url = base + esc + "&key=" + apiKey;
    curl_free(esc);

    // Perform request
    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,
                     +[](char* ptr, size_t size, size_t nmemb, std::string* out) -> size_t {
                         out->append(ptr, size * nmemb);
                         return size * nmemb;
                     }
    );
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    if (curl_easy_perform(curl) != CURLE_OK) {
        std::cerr << "CURL request failed: "
                  << curl_easy_strerror(curl_easy_perform(curl))
                  << "\n";
        curl_easy_cleanup(curl);
        return "";
    }
    curl_easy_cleanup(curl);

    // —— nlohmann/json parsing starts here —— //

    json root;
    try {
        // parse throws on error
        root = json::parse(response);
    }
    catch (const json::parse_error& e) {
        std::cerr << "Failed to parse JSON response: "
                  << e.what() << "\n";
        return "";
    }

    // drill into items[0].id.videoId
    if (root.contains("items") && root["items"].is_array() &&
        !root["items"].empty())
    {
        const auto& first = root["items"][0];
        if (first.contains("id") && first["id"].contains("videoId"))
        {
            // get<string>() will throw if it's not actually a string
            std::string videoId = first["id"]["videoId"].get<std::string>();
            if (!videoId.empty()) {
                return "https://www.youtube.com/watch?v=" + videoId;
            }
        }
    }

    // no valid result
    return "";
}

void AuditoryManager::captureFromYouTube() {
    // Use GStreamer to stream audio from YouTube URL
    GstElement* pipeline = nullptr;
    GstBus* bus = nullptr;
    GstMessage* msg = nullptr;

    std::string pipelineStr = "ytalkiengine location=\"" + youTubeVideoUrl + "\" ! decodebin ! audioconvert ! audioresample ! appsink name=appsink";

    GError* error = nullptr;
    pipeline = gst_parse_launch(pipelineStr.c_str(), &error);

    if (!pipeline) {
        std::cerr << "Failed to create GStreamer pipeline: " << error->message << std::endl;
        g_error_free(error);
        capturing = false;
        return;
    }

    GstElement* appSink = gst_bin_get_by_name(GST_BIN(pipeline), "appsink");
    GstAppSink* appsink = GST_APP_SINK(appSink);

    gst_app_sink_set_emit_signals(appsink, TRUE);
    gst_app_sink_set_drop(appsink, TRUE);
    gst_app_sink_set_max_buffers(appsink, 1);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    while (capturing.load()) {
        GstSample* sample = gst_app_sink_try_pull_sample(appsink, GST_SECOND);
        if (sample) {
            GstBuffer* buffer = gst_sample_get_buffer(sample);
            GstMapInfo map;
            if (gst_buffer_map(buffer, &map, GST_MAP_READ)) {
                // Assuming 16-bit signed integer PCM audio
                const int16_t* data = reinterpret_cast<const int16_t*>(map.data);
                size_t numSamples = map.size / sizeof(int16_t);

                // Convert samples to double and normalize
                std::vector<double> audioData(numSamples);
                for (size_t i = 0; i < numSamples; ++i) {
                    audioData[i] = data[i] / 32768.0; // Normalize to [-1.0, 1.0]
                }

                // Pass the audio data to the callback
                if (audioDataCallback) {
                    audioDataCallback(audioData);
                }

                gst_buffer_unmap(buffer, &map);
            }
            gst_sample_unref(sample);
        } else {
            // No sample received within timeout
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(appSink);
    gst_object_unref(pipeline);
}

std::vector<std::string> AuditoryManager::listAlsaCaptureDevices() {
    std::vector<std::string> captureDevices;
    int card = -1;
    int err;

    // Iterate through sound cards
    while ((err = snd_card_next(&card)) >= 0 && card >= 0) {
        char* cardName = nullptr;
        if ((err = snd_card_get_name(card, &cardName)) < 0) {
            std::cerr << "Cannot get name for card " << card << ": " << snd_strerror(err) << std::endl;
            continue;
        }

        // Open control interface for the card
        snd_ctl_t* ctl;
        std::string ctlName = "hw:" + std::to_string(card);
        if ((err = snd_ctl_open(&ctl, ctlName.c_str(), 0)) < 0) {
            std::cerr << "Cannot open control for card " << card << ": " << snd_strerror(err) << std::endl;
            free(cardName);
            continue;
        }

        int device = -1;
        // Iterate through PCM devices on this card
        while ((err = snd_ctl_pcm_next_device(ctl, &device)) >= 0 && device >= 0) {
            snd_pcm_info_t* pcmInfo;
            snd_pcm_info_alloca(&pcmInfo);
            snd_pcm_info_set_device(pcmInfo, device);
            snd_pcm_info_set_subdevice(pcmInfo, 0);
            snd_pcm_info_set_stream(pcmInfo, SND_PCM_STREAM_CAPTURE);

            if ((err = snd_ctl_pcm_info(ctl, pcmInfo)) >= 0) {
                const char* name = snd_pcm_info_get_name(pcmInfo);
                // Construct device identifier (e.g., "hw:0,0")
                std::string deviceId = "hw:" + std::to_string(card) + "," + std::to_string(device);
                captureDevices.emplace_back(deviceId);
                std::cout << "Found capture device: " << deviceId << " (" << name << ")" << std::endl;
            }
        }

        if (err < 0 && err != -ENOENT) {
            std::cerr << "Error iterating PCM devices for card " << card << ": " << snd_strerror(err) << std::endl;
        }

        snd_ctl_close(ctl);
        free(cardName);
    }

    if (err < 0 && err != -ENOENT) {
        std::cerr << "Error iterating sound cards: " << snd_strerror(err) << std::endl;
    }

    return captureDevices;
}

std::string AuditoryManager::autoSelectFirstAlsaCaptureDevice() {
    std::vector<std::string> devices = listAlsaCaptureDevices();

    char* pulse_sink = std::getenv("PULSE_SINK");
    if (pulse_sink != nullptr) {
        std::cout << "PULSE_SINK: " << pulse_sink << std::endl;
    } else {
        std::cout << "PULSE_SINK not set" << std::endl;
    }

    char* pulse_source = std::getenv("PULSE_SOURCE");
    if (pulse_source != nullptr) {
        std::cout << "PULSE_SOURCE: " << pulse_source << std::endl;
    } else {
        std::cout << "PULSE_SOURCE not set" << std::endl;
    }

    ThreadSafeQueue<std::vector<std::tuple<double, double>>> audioQueue;
    ThreadSafeQueue<std::vector<std::tuple<double, double>>> emptyAuditoryQueue;

    std::shared_ptr<PulseAudioMic> mic = std::make_shared<PulseAudioMic>(audioQueue);
    if (!mic) {
        std::cerr << "Failed to create PulseAudioMic!" << std::endl;
        return "";
    }
    std::thread micThread(&PulseAudioMic::micRun, mic);

    if (devices.empty()) {
        std::cerr << "Error: No ALSA capture devices found." << std::endl;
        return "";
    }
    std::cout << "Automatically selected ALSA capture device: " << devices[0] << std::endl;
    return devices[0];
}

void AuditoryManager::setSensoryReceptors(const std::vector<std::shared_ptr<SensoryReceptor>>& receptors) {
    sensoryReceptors = receptors;
}

void AuditoryManager::startProcessing() {
    if (!processing.load()) {
        processing = true;
        processingThread = std::thread(&AuditoryManager::processAuditoryData, this);
    }
}

void AuditoryManager::stopProcessing() {
    if (processing.load()) {
        processing = false;
        if (processingThread.joinable()) {
            processingThread.join();
        }
    }
}

void AuditoryManager::processAuditoryData() {
    const int FFT_SIZE = 1024; // Adjust as needed
    std::vector<double> audioBuffer;
    audioBuffer.reserve(FFT_SIZE);

    while (processing.load()) {
        std::vector<std::tuple<double,double>> audioData;
        if (!audioQueue.pop(audioData)) {
            // queue was empty or shutting down; back off slightly
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        // Extract the audio samples (assuming mono audio)
        for (const auto& sample : audioData) {
            double leftSample = std::get<0>(sample);
            audioBuffer.push_back(leftSample);
            if (audioBuffer.size() >= FFT_SIZE) {
                // Perform FFT on audioBuffer
                performFFTAndStimulate(audioBuffer);
                // Clear buffer for next batch
                audioBuffer.clear();
            }
        }
    }
}

void AuditoryManager::performFFTAndStimulate(const std::vector<double>& audioBuffer) {
    const int FFT_SIZE = audioBuffer.size();
    fftw_complex* out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (FFT_SIZE / 2 + 1));
    fftw_plan p = fftw_plan_dft_r2c_1d(FFT_SIZE, const_cast<double*>(audioBuffer.data()), out, FFTW_ESTIMATE);
    fftw_execute(p);

    // Stimulate SensoryReceptors based on frequency amplitudes
    for (size_t i = 0; i < sensoryReceptors.size(); ++i) {
        if (i < static_cast<size_t>(FFT_SIZE / 2 + 1)) {
            double magnitude = sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]);
            sensoryReceptors[i]->stimulate(magnitude);
        }
    }

    fftw_destroy_plan(p);
    fftw_free(out);
}
