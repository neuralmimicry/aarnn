#include "PulseAudioMic.h"

#include "utils.h"

PulseAudioMic::PulseAudioMic(ThreadSafeQueue<std::vector<std::tuple<double, double>>> &audioQueue)
: sample_spec({PA_SAMPLE_S16NE, 16000, 2})
, audioQueue(audioQueue)
{
    initializeContext();
    initializeCallbacks();
}

void PulseAudioMic::sourceSelection()
{
    std::vector<std::string> sourceList = this->getSources();

    // List sources
    for(size_t i = 0; i < sourceList.size(); ++i)
    {
        std::cout << i << ": " << sourceList[i] << "\n";
    }

    // Select source
    int selected_source;
    selected_source = 5;
    std::cout << "Selected source: " << selected_source << "\n";
    // std::cin >> selected_source;
    this->setSource(sourceList[selected_source]);
}

int PulseAudioMic::micRun()
{
    if(!context)
    {
        std::cerr << "Context was not initialized correctly.\n";
        return 1;
    }
    pa_threaded_mainloop_start(mainloop);
    std::this_thread::sleep_for(std::chrono::seconds(1));  // Wait for a while to let the mic populate the sources list
    sourceSelection();
    return 0;
}

void PulseAudioMic::micStop()
{
    pa_threaded_mainloop_stop(mainloop);
}

std::vector<std::string> PulseAudioMic::getSources()
{
    std::lock_guard<std::mutex> lock(capturedAudio_mtx);
    return sources;
}

std::vector<std::tuple<double, double>> PulseAudioMic::getCapturedAudio()
{
    std::lock_guard<std::mutex> lock(capturedAudio_mtx);
    return capturedAudio;
}

void PulseAudioMic::setSource(const std::string &source)
{
    initializeStream(source.c_str());
}

void PulseAudioMic::readStream()
{
    const void *data;
    size_t      length;
    if(pa_stream_peek(stream, &data, &length) < 0)
    {
        std::cerr << "Read failed: " << pa_strerror(pa_context_errno(pa_stream_get_context(stream))) << "\n";
        return;
    }

    processStream(data, length);
    pa_stream_drop(stream);
}

void PulseAudioMic::initializeContext()
{
    mainloop     = pa_threaded_mainloop_new();
    mainloop_api = pa_threaded_mainloop_get_api(mainloop);

    pa_proplist *proplist = pa_proplist_new();
    pa_proplist_sets(proplist, PA_PROP_APPLICATION_NAME, "AARNN");

    context = pa_context_new_with_proplist(mainloop_api, nullptr, proplist);
    pa_proplist_free(proplist);

    if(pa_context_connect(context, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0)
    {
        std::cerr << "pa_context_connect() failed: " << pa_strerror(pa_context_errno(context)) << "\n";
    }
}

void PulseAudioMic::initializeCallbacks()
{
    pa_context_set_state_callback(context, context_state_callback_static, this);
    // pa_context_set_source_info_callback(context, source_info_callback_static, this);
}

void PulseAudioMic::initializeStream(const char *source)
{
    if(stream)
    {
        pa_stream_disconnect(stream);
        pa_stream_unref(stream);
    }
    stream = pa_stream_new(context, "Playback", &sample_spec, nullptr);
    if(stream)
    {
        pa_stream_set_state_callback(stream, stream_state_callback_static, this);
        pa_stream_set_read_callback(stream, stream_read_callback_static, this);
        if(pa_stream_connect_record(stream, source, nullptr, PA_STREAM_NOFLAGS))
            std::cerr << "pa_stream_connect_record() failed: " << pa_strerror(pa_context_errno(context)) << "\n";
    }
    else
    {
        std::cerr << "pa_stream_new() failed: " << pa_strerror(pa_context_errno(context)) << "\n";
        return;
    }
}

void PulseAudioMic::cleanUp()
{
    if(stream)
    {
        pa_stream_disconnect(stream);
        pa_stream_unref(stream);
    }
    if(context)
    {
        pa_context_disconnect(context);
        pa_context_unref(context);
    }
    if(mainloop)
    {
        pa_threaded_mainloop_free(mainloop);
    }
}

void PulseAudioMic::processStream(const void *inputBuffer, size_t framesPerBuffer)
{
    // Capture audio data

    std::lock_guard<std::mutex> lock(capturedAudio_mtx);

    // auto executeFunc = [&]() {
    //     std::lock_guard<std::mutex> lock(capturedAudio_mtx);
    // };

    capturedAudio.clear();
    // perform FFT
    auto *out = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * framesPerBuffer);
    for(size_t i = 0; i < framesPerBuffer; ++i)
    {
        out[i][0] = 0.0;
        out[i][1] = 0.0;
    }
    // for(int i = 0; i < framesPerBuffer; ++i) {
    //     std::cout << ((double*) inputBuffer)[i] << " ";
    // }
    fftw_plan p = fftw_plan_dft_r2c_1d(framesPerBuffer, (double *)inputBuffer, out, FFTW_ESTIMATE);
    fftw_execute(p);
    // calculate magnitude
    std::vector<double> magnitude;
    magnitude.reserve(framesPerBuffer);
    for(size_t i = 0; i < framesPerBuffer; ++i)
    {
        magnitude.emplace_back(std::sqrt(out[i][0] * out[i][0] + out[i][1] * out[i][1]));
    }
    // std::cout << "o";

    // clean up
    fftw_destroy_plan(p);
    fftw_free(out);
    for(unsigned int i = 0; i < framesPerBuffer; i++)
    {
        if(!std::isnan(magnitude[i]))
        {
            capturedAudio.emplace_back((double)i * SAMPLE_RATE / framesPerBuffer, magnitude[i]);
            // std::cout << std::get<0>(capturedAudio.back()) << " " << std::get<1>(capturedAudio.back()) << ", ";
        }
    }
    audioQueue.push(capturedAudio);
}

void PulseAudioMic::context_state_callback(pa_context *c)
{
    switch(pa_context_get_state(c))
    {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
        default:
            break;

        case PA_CONTEXT_READY:
            pa_operation_unref(pa_context_get_source_info_list(c, source_info_callback_static, this));
            break;

        case PA_CONTEXT_FAILED:
            std::cerr << "Connection failed\n";
            break;

        case PA_CONTEXT_TERMINATED:
            std::cerr << "Connection terminated\n";
            break;
    }
}

void PulseAudioMic::source_info_callback(pa_context *c, const pa_source_info *i, int eol)
{
    if(eol > 0)
    {
        return;
    }
    sources.emplace_back(i->name);
}

void PulseAudioMic::stream_state_callback(pa_stream *s)
{
    assert(s);
    switch(pa_stream_get_state(s))
    {
        case PA_STREAM_CREATING:
            std::cout << "Stream is being created...\n";
            break;
        case PA_STREAM_READY:
            std::cout << "Stream is ready to be used.\n";
            break;
        case PA_STREAM_FAILED:
            std::cerr << "Stream has failed.\n";
            break;
        case PA_STREAM_TERMINATED:
            std::cout << "Stream has been terminated.\n";
            break;
        case PA_STREAM_UNCONNECTED:
            std::cerr << "Stream is unconnected.\n";
            break;
        default:
            std::cerr << "Unknown stream state.\n";
            break;
    }
}