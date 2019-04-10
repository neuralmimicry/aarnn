cmake_minimum_required(VERSION 3.0)
project(NeuralMimicry LANGUAGES C)

if(UNIX)
    add_definitions(-DUNIX)
endif(UNIX)

if(WIN32)
    add_definitions(-DWIN32)
    set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ws2_32)
endif(WIN32)


# the server

SET(server_SRCS
   src/server.c
   src/msock.c
)

add_executable(aarnnServer ${server_SRCS})

target_link_libraries(server
                      ${ADDITIONAL_LIBS}
)

install(TARGETS aarnnServer DESTINATION bin)


# the client

SET(client_SRCS
   src/client.c
   src/msock.c
)

add_executable(aarnnClient ${client_SRCS})

target_link_libraries(client
                      ${ADDITIONAL_LIBS}
)

install(TARGETS aarnnClient DESTINATION bin)

