add_executable(run_aarnn_tests
        run_aarnn_tests.cpp
        axon_bouton_tests.cpp
        axon_branch_tests.cpp
        axon_tests.cpp
        axon_hillock_tests.cpp
        body_component_tests.cpp
        brain_tests.cpp
        dendrite_bouton_tests.cpp
        dendrite_branch_tests.cpp
        dendrite_tests.cpp
        effector_tests.cpp
        neuron_tests.cpp
        position_tests.cpp
        sensory_receptor_tests.cpp
        soma_tests.cpp
        synaptic_gap_tests.cpp
        thread_safe_queue_tests.cpp
        parameters_tests.cpp)


target_link_libraries(run_aarnn_tests PRIVATE gtest gtest_main brain fmt)

add_test(NAME aarnn_tests COMMAND run_aarnn_tests)
include(Dart)

