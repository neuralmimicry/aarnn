cmake_minimum_required(VERSION 3.0)
project(NeuralMimicry LANGUAGES C)

if(UNIX)
    add_definitions(-DUNIX)
endif(UNIX)

if(WIN32)
    add_definitions(-DWIN32)
    set(ADDITIONAL_LIBS ${ADDITIONAL_LIBS} ws2_32)
endif(WIN32)

include_directories(./include)

# the server

SET(server_SRCS
   src/server.c
   src/msock.c
)

add_executable(aarnnServer ${server_SRCS})

target_link_libraries(aarnnServer
                      ${ADDITIONAL_LIBS}
)

install(TARGETS aarnnServer DESTINATION bin)


# the client

SET(client_SRCS
   src/client.c
   src/msock.c
)

add_executable(aarnnClient ${client_SRCS})

target_link_libraries(aarnnClient
                      ${ADDITIONAL_LIBS}
)

install(TARGETS aarnnClient DESTINATION bin)


# aarnn integrated

SET(aarnn_SRCS
   src/aarnn.c
)

add_executable(aarnn ${aarnn_SRCS})

target_link_libraries(aarnn
                      ${ADDITIONAL_LIBS}
)

install(TARGETS aarnn DESTINATION bin)

