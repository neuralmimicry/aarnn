add_library(brain
        axon_bouton.cpp
        axon_branch.cpp
        axon.cpp
        axon_hillock.cpp
        body_component.cpp
        brain.cpp
        dendrite_bouton.cpp
        dendrite_branch.cpp
        dendrite.cpp
        neuron.cpp
        sensory_receptor.cpp
        soma.cpp
        synaptic_gap.cpp
        effector.cpp
)

#add_executable(aarnn aarnn.cpp
#        av_timer_callback.cpp
#        axon_bouton.cpp
#        axon_branch.cpp
#        axon.cpp
#        axon_hillock.cpp
#        dendrite_bouton.cpp
#        dendrite_branch.cpp
#        dendrite.cpp
#        effector.cpp
#        neuron.cpp
#        portaudio_mic_thread.cpp
#        position.cpp
#        pulse_audioMic.cpp
#        sensory_receptor.cpp
#        soma.cpp
#        synaptic_gap.cpp
#        ../include/rand_utils.h)
#
#target_link_libraries(aarnn PRIVATE
#        Boost::headers
#        Boost::system
#        Boost::filesystem
#        ${PQXX_LIB}
#        ${PostgreSQL_LIBRARIES}
#        ${VTK_LIBRARIES}
#        ${OpenSSL_LIBRARIES}
#        ${FFmpeg_LIBRARIES}
#        ${PORTAUDIO_LIBRARIES}
#        ${PULSEAUDIO_LIBRARIES}
#        ${FFTW3_LIBRARIES}
#        OpenMP::OpenMP_CXX
#        spdlog::spdlog)
#
## Define the target for visualiser.cpp
#add_executable(visualiser visualiser.cpp)
#target_link_libraries(visualiser PRIVATE
#        Boost::headers
#        Boost::system
#        Boost::filesystem
#        ${PQXX_LIB}
#        ${PostgreSQL_LIBRARIES}
#        ${VTK_LIBRARIES}
#        ${OpenSSL_LIBRARIES}
#        ${FFmpeg_LIBRARIES}
#        ${PORTAUDIO_LIBRARIES}
#        ${PULSEAUDIO_LIBRARIES}
#        ${FFTW3_LIBRARIES}
#        OpenMP::OpenMP_CXX
#        spdlog::spdlog)
