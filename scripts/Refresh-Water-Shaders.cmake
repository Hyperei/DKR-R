# Shader-only iteration through the authored patch pipeline. Run only after
# any build using this directory has completed. No dependency or generated
# recompiled function sources are modified; clean builds use EnhancedWater.cmake.
if(NOT DEFINED DKR_SOURCE_ROOT OR NOT DEFINED DKR_BUILD_ROOT)
    message(FATAL_ERROR "Supply absolute DKR_SOURCE_ROOT and DKR_BUILD_ROOT")
endif()
set(stage "${DKR_BUILD_ROOT}/generated/enhanced-water")
if(NOT EXISTS "${stage}/.git" OR NOT EXISTS "${stage}/src/render/WaterWorkPolicy.h")
    message(FATAL_ERROR "Expected an existing enhanced-water performance stage")
endif()
find_program(GIT_EXECUTABLE git REQUIRED)
set(patch "${DKR_SOURCE_ROOT}/patches/enhanced-water/dkr-reflection-search.patch")
execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${stage}" apply --check --recount --ignore-space-change --whitespace=error "${patch}" RESULT_VARIABLE forward ERROR_VARIABLE error)
if(forward EQUAL 0)
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${stage}" apply --recount --ignore-space-change --whitespace=error "${patch}" COMMAND_ERROR_IS_FATAL ANY)
else()
    execute_process(COMMAND "${GIT_EXECUTABLE}" -C "${stage}" apply --reverse --check --recount --ignore-space-change --whitespace=error "${patch}" RESULT_VARIABLE reverse ERROR_VARIABLE error)
    if(NOT reverse EQUAL 0)
        # The first iteration had the same expressions in the performance
        # layer, without the explanatory comment. Verify all four expressions
        # before accepting that already-composed iteration.
        file(READ "${stage}/src/shaders/WaterPS.hlsl" pixels)
        foreach(expression IN ITEMS
            "const float4 rayOrigin=mul(p.viewProjection,float4(origin,1));"
            "waterPixelFromClip(rayOrigin+rayDirection*distance,p)"
            "waterPixelFromClip(rayOrigin+rayDirection*mid,p)"
            "waterPixelFromClip(rayOrigin+rayDirection*hi,p)")
            string(FIND "${pixels}" "${expression}" found)
            if(found LESS 0)
                message(FATAL_ERROR "Unrecognised reflection stage: ${error}")
            endif()
        endforeach()
    endif()
endif()
foreach(file IN ITEMS WaterProjection.hlsli WaterProjectionProbeCS.hlsl)
    configure_file("${DKR_SOURCE_ROOT}/patches/enhanced-water/native/${file}" "${stage}/src/shaders/${file}" COPYONLY)
endforeach()
message(STATUS "Authored reflection shader layer and projection probe refreshed")
