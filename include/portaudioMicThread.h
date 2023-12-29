#ifndef PORTAUDIOMICTHREAD_H
#define PORTAUDIOMICTHREAD_H

class portaudioMicThread {
public:
    portaudioMicThread();
    ~portaudioMicThread();
    int run() ;
    void stop();
private:
    PaStream *stream;
    PaStreamInfo stream_info{};
};

#endif