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
project("stackingcamera")

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

# Adding jniLibs
include_directories(${CMAKE_SOURCE_DIR}/include/${ANDROID_ABI})

set(JNI_LIBS ${CMAKE_SOURCE_DIR}/../jniLibs/${ANDROID_ABI})

add_library(libturbojpeg SHARED IMPORTED)
set_target_properties(libturbojpeg PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS}/libturbojpeg.so)

add_library(libpng SHARED IMPORTED)
set_target_properties(libpng PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS}/libpng16.so)

add_library(libtiff SHARED IMPORTED)
set_target_properties(libtiff PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS}/libtiff.so)

add_library(libraw SHARED IMPORTED)
set_target_properties(libraw PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS}/libraw.so)

add_library(lcms2 SHARED IMPORTED)
set_target_properties(lcms2 PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS}/liblcms2.so)

add_library(OpenCL SHARED IMPORTED)
set_target_properties(OpenCL PROPERTIES IMPORTED_LOCATION
        ${JNI_LIBS}/libOpenCL.so)

# Open MP
find_package(OpenMP REQUIRED)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

# Including project lib
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/*.cpp")
add_library(${CMAKE_PROJECT_NAME} SHARED ${SOURCES}
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        ${SOURCES})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/${CMAKE_PROJECT_NAME}/")

# Specifies libraries CMake should link to your target library. You
# can link libraries from various origins, such as libraries defined in this
# build script, prebuilt third-party libraries, or Android system libraries.
target_link_libraries(${CMAKE_PROJECT_NAME}
        # List libraries link to the target library
        OpenCL
        libturbojpeg
        libpng
        libtiff
        libraw
        lcms2
        android
        log
        OpenMP::OpenMP_CXX)