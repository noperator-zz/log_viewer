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

set(CMAKE_TOOLCHAIN_FILE "C:/development/.vcpkg-clion/vcpkg/scripts/buildsystems/vcpkg.cmake")

find_package(OpenGL REQUIRED)
#find_package(PkgConfig REQUIRED)
#pkg_search_module(GLFW REQUIRED glfw3)
find_package(glfw3 CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

# Path to your shaders
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
file(GLOB SHADER_FILES "${SHADER_DIR}/*.glsl")

set(GENERATED_SHADER_HEADERS)
foreach(SHADER ${SHADER_FILES})
    get_filename_component(FILE_NAME ${SHADER} NAME)
    string(REPLACE "." "_" VAR_NAME ${FILE_NAME})
    set(HEADER "${CMAKE_CURRENT_BINARY_DIR}/shaders/${FILE_NAME}.h")
    list(APPEND GENERATED_SHADER_HEADERS ${HEADER})

    add_custom_command(
        OUTPUT ${HEADER}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/shaders
        COMMAND ${CMAKE_COMMAND} -DINPUT=${SHADER} -DOUTPUT=${HEADER} -DVAR_NAME=${VAR_NAME}
                -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/embed_shader.cmake
        DEPENDS ${SHADER}
        COMMENT "Embedding shader ${FILE_NAME}"
    )
endforeach()

# Add a custom target to generate all shaders
add_custom_target(compile_shaders DEPENDS ${GENERATED_SHADER_HEADERS})

add_executable(${PROJECT_NAME}
    ${GENERATED_SHADER_HEADERS}
    shaders/text_shader.cpp
    shaders/gp_shader.cpp
    src/main.cpp
    src/app.cpp
    src/font.cpp
    src/file.cpp
    src/shader.h
    src/file_view.cpp
    src/parser.cpp
    src/widget.cpp
    src/scrollbar.cpp
)
add_dependencies(${PROJECT_NAME} compile_shaders)

target_include_directories(${PROJECT_NAME} PRIVATE
#    ${GLFW_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}/shaders
    shaders
    src
)

target_compile_options(${PROJECT_NAME} PRIVATE -march=native)

#target_link_directories(${PROJECT_NAME} PRIVATE
#    lib
#)

target_link_libraries(${PROJECT_NAME} PRIVATE
    OpenGL::GL
    glfw
    freetype
    GLEW::GLEW
    glm::glm
)
