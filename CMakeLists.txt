cmake_minimum_required(VERSION 3.16...3.28)

project(smartoil
    VERSION 0.1
    DESCRIPTION "Smart Oil Management Application"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql Charts PrintSupport SerialPort)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql Charts PrintSupport SerialPort)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    qualite.cpp
    qualite.h
    qualite.ui
    citernes.cpp
    citernes.h
    citernes.ui
    stocks.cpp
    stocks.h
    stocks.ui
    employe.cpp
    employe.h
    connection.cpp
    connection.h
    face_recognition_service.cpp
    face_recognition_service.h
    face_recognition_dialog.cpp
    face_recognition_dialog.h
    face_capture_dialog.cpp
    face_capture_dialog.h
    fingerprintservice.cpp
    fingerprintservice.h
    arduino.cpp
    arduino.h
    style.qrc
    img.qrc
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(smartoil
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(smartoil ${PROJECT_SOURCES})
endif()

target_link_libraries(smartoil PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Charts
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::SerialPort
)

set_target_properties(smartoil PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.smartoil"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS smartoil
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# =========================
# OpenCV
# =========================

set(OpenCV_DIR "C:/opencv/build_mingw")

find_package(OpenCV REQUIRED COMPONENTS
    core
    imgproc
    imgcodecs
    highgui
    dnn
    objdetect
    videoio
)

target_link_libraries(smartoil PRIVATE ${OpenCV_LIBS})

# Prefer the old working OpenCV bin path.
# If it does not exist, fallback to your friend's path.
if(EXISTS "C:/opencv/build_mingw/bin")
    set(OPENCV_BIN_DIR "C:/opencv/build_mingw/bin")
elseif(EXISTS "C:/opencv/build_mingw/x64/mingw/bin")
    set(OPENCV_BIN_DIR "C:/opencv/build_mingw/x64/mingw/bin")
else()
    message(FATAL_ERROR "OpenCV DLL folder not found. Check C:/opencv/build_mingw/bin or C:/opencv/build_mingw/x64/mingw/bin")
endif()

message(STATUS "Using OpenCV_DIR: ${OpenCV_DIR}")
message(STATUS "Using OPENCV_BIN_DIR: ${OPENCV_BIN_DIR}")

# Copy all OpenCV runtime DLLs next to smartoil.exe.
# This avoids hardcoding libopencv_core455.dll or libopencv_core4120.dll.
file(GLOB OPENCV_RUNTIME_DLLS
    "${OPENCV_BIN_DIR}/libopencv_*.dll"
    "${OPENCV_BIN_DIR}/opencv_videoio_ffmpeg*.dll"
)

if(NOT OPENCV_RUNTIME_DLLS)
    message(FATAL_ERROR "No OpenCV DLLs found in ${OPENCV_BIN_DIR}")
endif()

foreach(_dll ${OPENCV_RUNTIME_DLLS})
    add_custom_command(TARGET smartoil POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_dll}"
            "$<TARGET_FILE_DIR:smartoil>"
    )
endforeach()

# =========================
# Face recognition models
# =========================

set(FACE_DETECTION_MODEL
    "${CMAKE_SOURCE_DIR}/models/face_detection_yunet_2023mar.onnx"
)

set(FACE_RECOGNITION_MODEL
    "${CMAKE_SOURCE_DIR}/models/face_recognition_sface_2021dec.onnx"
)

if(NOT EXISTS "${FACE_DETECTION_MODEL}")
    message(FATAL_ERROR "Missing model file: ${FACE_DETECTION_MODEL}")
endif()

if(NOT EXISTS "${FACE_RECOGNITION_MODEL}")
    message(FATAL_ERROR "Missing model file: ${FACE_RECOGNITION_MODEL}")
endif()

add_custom_command(TARGET smartoil POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FACE_DETECTION_MODEL}"
        "$<TARGET_FILE_DIR:smartoil>/face_detection_yunet_2023mar.onnx"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${FACE_RECOGNITION_MODEL}"
        "$<TARGET_FILE_DIR:smartoil>/face_recognition_sface_2021dec.onnx"
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(smartoil)
endif()
