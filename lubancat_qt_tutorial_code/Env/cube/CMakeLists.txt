# Generated from cube.pro.

cmake_minimum_required(VERSION 3.16)
project(cube LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/opengl/cube")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS OpenGL)
find_package(Qt6 COMPONENTS OpenGLWidgets)

qt_add_executable(cube
    geometryengine.cpp geometryengine.h
    main.cpp
    mainwidget.cpp mainwidget.h
)
set_target_properties(cube PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(cube PUBLIC
    Qt::Core
    Qt::Gui
    Qt::OpenGL
    Qt::OpenGLWidgets
    Qt::Widgets
)


# Resources:
set(shaders_resource_files
    "fshader.glsl"
    "vshader.glsl"
)

qt6_add_resources(cube "shaders"
    PREFIX
        "/"
    FILES
        ${shaders_resource_files}
)
set(textures_resource_files
    "cube.png"
)

qt6_add_resources(cube "textures"
    PREFIX
        "/"
    FILES
        ${textures_resource_files}
)

install(TARGETS cube
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

option(DEPLOYED_REMOTELY "Turn on for remote deployment" OFF)

macro(add_deployment_file SRC DEST)
    file(RELATIVE_PATH path ${CMAKE_SOURCE_DIR}
         ${CMAKE_CURRENT_SOURCE_DIR})
    file(APPEND "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
         "${path}/${SRC}:${DEST}\n")
endmacro()

set(CMAKE_INSTALL_PREFIX "/home/cat/qt/opengl/cube")
if(DEPLOYED_REMOTELY)
    # Write base installation path as first line.
    file(WRITE "${CMAKE_BINARY_DIR}/QtCreatorDeployment.txt"
         "${CMAKE_INSTALL_PREFIX}\n")

    # Append mapping for executable.
    file(RELATIVE_PATH relative_exe_path
         "${CMAKE_CURRENT_SOURCE_DIR}"
         "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}")

    add_deployment_file(relative_exe_path cube)

endif()


