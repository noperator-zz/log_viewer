cmake_minimum_required(VERSION 3.22)
project(log_viewer)

set(CMAKE_CXX_STANDARD 23)

#find_program(CCACHE ccache)
#if (CCACHE)
#    set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
#else()
#    message(WARNING "Consider installing ccache to speed up builds")
#endif()

#if (NOT CMAKE_BUILD_TYPE MATCHES Debug)
#    set(CMAKE_CXX_FLAGS "-static-libgcc -static-libstdc++ -Werror -Wunused")
#endif()

add_executable(log_viewer
    src/main.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
#    include/skia
    include/SDL2
)
#
#add_library(skia STATIC IMPORTED)
#set_target_properties(skia PROPERTIES
#    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/skia.lib"
#    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/skia/"
#)


if (WIN32)
    # Copy DLLs from bin folder to cmake build directory after build
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/bin $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )

    # Copy the release executable from cmake build directory to python package directory after build
    if (CMAKE_BUILD_TYPE MATCHES Release AND DEFINED PYTHON_PACKAGE_DIR)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:${PROJECT_NAME}> ${PYTHON_PACKAGE_DIR}
        )
    endif()

endif()

target_link_directories(${PROJECT_NAME} PUBLIC lib)

if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
    #    C:/development/skia/skia/out/StaticRelease/skia.lib
        SDL2.dll.a
        SDL2_ttf.dll.a
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL2
        SDL2_ttf
    )
endif()

