add_executable(AARNN aarnn.cpp
                avTimerCallback.cpp
                AxonBouton.cpp
                AxonBranch.cpp
                Axon.cpp
                AxonHillock.cpp
                BodyComponent.cpp
                BodyShapeComponent.cpp
                DendriteBouton.cpp
                DendriteBranch.cpp
                Dendrite.cpp
                Effector.cpp
                Logger.cpp
                Neuron.cpp
                NeuronParameters.cpp
                portaudioMicThread.cpp
                Position.cpp
                PulseAudioMic.cpp
                SensoryReceptor.cpp
                Soma.cpp
                SynapticGap.cpp)

target_link_libraries(AARNN PRIVATE Boost::headers Boost::system Boost::filesystem ${PQXX_LIB} ${PostgreSQL_LIBRARIES} ${VTK_LIBRARIES}
                             ${OpenSSL_LIBRARIES} ${FFmpeg_LIBRARIES} ${PORTAUDIO_LIBRARIES} ${PULSEAUDIO_LIBRARIES} ${FFTW3_LIBRARIES} OpenMP::OpenMP_CXX)  


 # Define the target for visualiser.cpp
add_executable(Visualiser visualiser.cpp)
target_link_libraries(Visualiser PRIVATE Boost::headers Boost::system Boost::filesystem ${PQXX_LIB} 
                            ${PostgreSQL_LIBRARIES} ${VTK_LIBRARIES} ${OpenSSL_LIBRARIES} ${FFmpeg_LIBRARIES} ${PORTAUDIO_LIBRARIES} 
                            ${PULSEAUDIO_LIBRARIES} ${FFTW3_LIBRARIES} OpenMP::OpenMP_CXX)
