# Project test executables use DKR*Tests names. Configure their assertion
# contract after all declarations, including optional targets from modules.
# Restrict discovery to this directory: dependency test targets in their own
# subdirectories must retain their upstream build settings.
include_guard(DIRECTORY)

function(dkr_configure_test_assertions)
    get_property(project_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(test_target IN LISTS project_targets)
        get_target_property(target_type "${test_target}" TYPE)
        if(test_target MATCHES "^DKR.+Tests$" AND target_type STREQUAL "EXECUTABLE")
            if(MSVC)
                target_compile_options("${test_target}" PRIVATE /UNDEBUG)
            else()
                target_compile_options("${test_target}" PRIVATE -UNDEBUG)
            endif()
        endif()
    endforeach()
endfunction()

# Private qualification executables can be built even with BUILD_TESTING=OFF.
cmake_language(DEFER CALL dkr_configure_test_assertions)
