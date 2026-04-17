# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

#set(CMAKE_C_STANDARD 90)
#set(CMAKE_CXX_STANDARD 11)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("stacking")

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.
#
# In this top level CMakeLists.txt, ${CMAKE_PROJECT_NAME} is used to define
# the target library name; in the sub-module's CMakeLists.txt, ${PROJECT_NAME}
# is preferred for the same purpose.
#
# In order to load a library into your app from Java/Kotlin, you must call
# System.loadLibrary() and pass the name of the library defined here;
# for GameActivity/NativeActivity derived applications, the same library name must be
# used in the AndroidManifest.xml file.

# 1. Adding libs
set(LIBS_DIR ${CMAKE_SOURCE_DIR}/../libs/${ANDROID_ABI})

macro(add_imported_lib NAME FILENAME)
    add_library(${NAME} SHARED IMPORTED)
    set_target_properties(${NAME} PROPERTIES IMPORTED_LOCATION ${LIBS_DIR}/${FILENAME})
endmacro()

add_imported_lib(libturbojpeg libturbojpeg.so)
add_imported_lib(libpng       libpng16.so)
add_imported_lib(libtiff      libtiff.so)
add_imported_lib(libraw       libraw.so)
add_imported_lib(lcms2        liblcms2.so)
add_imported_lib(OpenCL       libOpenCL.so)

# Open MP
find_package(OpenMP REQUIRED)

# 2. Core lib
set(CORE_LIB_NAME core)
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/${CORE_LIB_NAME}/*.cpp")

add_library(${CORE_LIB_NAME} STATIC ${CORE_SOURCES})
target_include_directories(${CORE_LIB_NAME}
        PUBLIC
            "${LIBS_DIR}/include/"
            "${CMAKE_SOURCE_DIR}/${CORE_LIB_NAME}"
        INTERFACE
            "${CMAKE_SOURCE_DIR}")

target_link_libraries(${CORE_LIB_NAME} PUBLIC
        OpenCL
        libturbojpeg
        libpng
        libtiff
        libraw
        lcms2
        OpenMP::OpenMP_CXX)

# 3. Stacking lib
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/*.cpp")

add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES})
#target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/${CMAKE_PROJECT_NAME}")
target_link_libraries(${CMAKE_PROJECT_NAME} android log ${CORE_LIB_NAME})