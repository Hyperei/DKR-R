# All dependency modifications use checked isolated copies, never the checkout.
find_package(Git REQUIRED)
function(dkr_stage_patch target source stage_name patch)
    set(stage "${CMAKE_BINARY_DIR}/generated/${stage_name}")
    set(files ${ARGN})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${patch}")
    foreach(file IN LISTS files)
        get_filename_component(parent "${file}" DIRECTORY)
        file(MAKE_DIRECTORY "${stage}/${parent}")
        configure_file("${source}/${file}" "${stage}/${file}" COPYONLY)
    endforeach()
    execute_process(COMMAND "${GIT_EXECUTABLE}" init --quiet "${stage}"
        COMMAND_ERROR_IS_FATAL ANY)
    foreach(mode IN ITEMS check apply)
        # Checkout EOLs differ between Windows and Linux. Ignore context-only
        # whitespace; added lines still pass the strict whitespace check.
        set(flags --recount --whitespace=error --ignore-space-change)
        if(mode STREQUAL "check")
            list(APPEND flags --check)
        endif()
        execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${stage}" apply ${flags} "${patch}"
            RESULT_VARIABLE result ERROR_VARIABLE error)
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "${stage_name} ${mode} failed: ${error}")
        endif()
    endforeach()
    get_target_property(sources ${target} SOURCES)
    foreach(file IN LISTS files)
        get_filename_component(name "${file}" NAME)
        set(matches ${sources})
        list(FILTER matches INCLUDE REGEX "(^|/)${name}$")
        list(LENGTH matches count)
        if(NOT count EQUAL 1)
            message(FATAL_ERROR "${stage_name}: expected one ${name}, found ${count}")
        endif()
        list(REMOVE_ITEM sources ${matches})
        list(APPEND sources "${stage}/${file}")
        get_filename_component(parent "${file}" DIRECTORY)
        set_source_files_properties("${stage}/${file}" TARGET_DIRECTORY ${target}
            PROPERTIES INCLUDE_DIRECTORIES "${source}/${parent};${DKRPORT_ROOT}/runtime-recomp/src/game")
    endforeach()
    set_property(TARGET ${target} PROPERTY SOURCES "${sources}")
endfunction()

dkr_stage_patch(ultramodern "${DKR_MODERN_RUNTIME_SOURCE}" performance-runtime
    "${DKRPORT_ROOT}/patches/performance/runtime-snapshot-pool.patch" ultramodern/src/events.cpp)
dkr_stage_patch(ultramodern "${CMAKE_BINARY_DIR}/generated/performance-runtime" host-task-lifetime
    "${DKRPORT_ROOT}/patches/performance/runtime-host-task-lifetime.patch" ultramodern/src/events.cpp)
dkr_stage_patch(ultramodern "${CMAKE_BINARY_DIR}/generated/host-task-lifetime" vi-bootstrap
    "${DKRPORT_ROOT}/patches/performance/runtime-vi-bootstrap.patch" ultramodern/src/events.cpp)
dkr_stage_patch(librecomp "${DKR_MODERN_RUNTIME_SOURCE}" session-bootstrap
    "${DKRPORT_ROOT}/patches/performance/runtime-session-bootstrap.patch" librecomp/src/recomp.cpp)
dkr_stage_patch(ultramodern "${DKR_MODERN_RUNTIME_SOURCE}" guest-lifetime
    "${DKRPORT_ROOT}/patches/performance/runtime-guest-lifetime.patch"
    ultramodern/src/threads.cpp ultramodern/src/threadqueue.cpp ultramodern/src/scheduling.cpp)
dkr_stage_patch(librecomp "${CMAKE_BINARY_DIR}/generated/session-bootstrap" guest-cancellation
    "${DKRPORT_ROOT}/patches/performance/runtime-guest-cancellation.patch" librecomp/src/recomp.cpp)
if(DKR_REPLAY_QUALIFICATION OR DKR_EXPERIMENTAL_RACE_TEST)
    dkr_stage_patch(librecomp "${CMAKE_BINARY_DIR}/generated/guest-cancellation" experimental-quiesced-bootstrap
        "${DKRPORT_ROOT}/patches/netplay/experimental-quiesced-bootstrap.patch" librecomp/src/recomp.cpp)
endif()
if(BUILD_TESTING AND NOT ANDROID)
    add_executable(DKRNativeGuestLifetimeTests "${DKRPORT_ROOT}/runtime-recomp/tests/native_guest_lifetime_tests.cpp")
    target_compile_definitions(DKRNativeGuestLifetimeTests PRIVATE
        DKR_THREADS_SOURCE="${CMAKE_BINARY_DIR}/generated/guest-lifetime/ultramodern/src/threads.cpp"
        DKR_QUEUE_SOURCE="${CMAKE_BINARY_DIR}/generated/guest-lifetime/ultramodern/src/threadqueue.cpp"
        DKR_SCHEDULING_SOURCE="${CMAKE_BINARY_DIR}/generated/guest-lifetime/ultramodern/src/scheduling.cpp")
    target_include_directories(DKRNativeGuestLifetimeTests PRIVATE
        "${DKR_MODERN_RUNTIME_SOURCE}/ultramodern/include"
        "${DKR_MODERN_RUNTIME_SOURCE}/thirdparty"
        "${DKR_MODERN_RUNTIME_SOURCE}/thirdparty/concurrentqueue"
        "${DKRPORT_ROOT}/runtime-recomp/src/game")
    if(NOT MSVC)
        target_link_libraries(DKRNativeGuestLifetimeTests PRIVATE pthread)
    endif()
    add_test(NAME DKRNativeGuestLifetime COMMAND DKRNativeGuestLifetimeTests)
    set_tests_properties(DKRNativeGuestLifetime PROPERTIES TIMEOUT 30)
    add_executable(DKRRuntimeLogArchiveTests "${DKRPORT_ROOT}/runtime-recomp/tests/runtime_log_archive_tests.cpp")
    target_include_directories(DKRRuntimeLogArchiveTests PRIVATE "${DKRPORT_ROOT}/runtime-recomp/src/game")
    add_test(NAME DKRRuntimeLogArchive COMMAND DKRRuntimeLogArchiveTests)
    add_executable(DKRViBootstrapTests "${DKRPORT_ROOT}/runtime-recomp/tests/vi_bootstrap_tests.cpp")
    target_compile_definitions(DKRViBootstrapTests PRIVATE
        DKR_VI_SOURCE="${CMAKE_BINARY_DIR}/generated/vi-bootstrap/ultramodern/src/events.cpp")
    target_include_directories(DKRViBootstrapTests PRIVATE
        "${DKR_MODERN_RUNTIME_SOURCE}/ultramodern/include"
        "${DKR_MODERN_RUNTIME_SOURCE}/thirdparty"
        "${DKR_MODERN_RUNTIME_SOURCE}/thirdparty/concurrentqueue"
        "${DKRPORT_ROOT}/runtime-recomp/src/game")
    if(MSVC)
        target_compile_options(DKRViBootstrapTests PRIVATE /Gy /Gw)
        target_link_options(DKRViBootstrapTests PRIVATE /OPT:REF)
    else()
        target_compile_options(DKRViBootstrapTests PRIVATE -ffunction-sections -fdata-sections)
        if(APPLE)
            target_link_options(DKRViBootstrapTests PRIVATE -Wl,-dead_strip)
        else()
            target_link_options(DKRViBootstrapTests PRIVATE -Wl,--gc-sections)
        endif()
        target_link_libraries(DKRViBootstrapTests PRIVATE pthread)
    endif()
    add_test(NAME DKRViBootstrap COMMAND DKRViBootstrapTests)
    set_tests_properties(DKRViBootstrap PROPERTIES TIMEOUT 30)
endif()
# Fault injection is deliberately absent from distributed binaries.
option(DKR_TASK_QUALIFICATION "Private host-task delay qualification" OFF)
option(DKR_WATER_QUALIFICATION "Private offline water preview qualification; never package" OFF)
if(DKR_WATER_QUALIFICATION)
    add_compile_definitions(DKR_WATER_QUALIFICATION=1)
endif()
if(DKR_TASK_QUALIFICATION)
    add_compile_definitions(DKR_TASK_QUALIFICATION=1)
    # ultramodern's directory was configured before this file was included.
    target_compile_definitions(ultramodern PRIVATE DKR_TASK_QUALIFICATION=1)
endif()
dkr_stage_patch(ultramodern "${DKR_MODERN_RUNTIME_SOURCE}" performance-idle
    "${DKRPORT_ROOT}/patches/performance/runtime-idle-diagnostics.patch" ultramodern/src/mesgqueue.cpp)
dkr_stage_patch(ultramodern "${CMAKE_BINARY_DIR}/generated/performance-idle" performance-events
    "${DKRPORT_ROOT}/patches/performance/runtime-deferred-events.patch" ultramodern/src/mesgqueue.cpp)
