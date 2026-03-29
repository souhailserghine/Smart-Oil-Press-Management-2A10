# Enable verbose build and AutoUIC trace to surface the exact uic command and errors
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_AUTOUIC_TRACE ON)

# Ensure Qt's automatic processing is enabled and avoid compiler predefines step that fails on some environments
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC_COMPILER_PREDEFINES OFF)
cmake_minimum_required(VERSION 3.16)

project(smartoil VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Charts)
find_package(Qt6 REQUIRED COMPONENTS Widgets Sql)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
    face_recognition_service.h
    face_recognition_service.cpp
    face_recognition_dialog.h
    face_recognition_dialog.cpp
    face_capture_dialog.h
    face_capture_dialog.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(smartoil
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}



        style.qrc
        img.qrc
        connection.h
        connection.cpp
        employe.h employe.cpp

    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET smartoil APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(smartoil SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(smartoil
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(smartoil PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts)
target_link_libraries(smartoil PRIVATE Qt6::Core Qt6::Sql)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.smartoil)
endif()
set_target_properties(smartoil PROPERTIES
    ${BUNDLE_ID_OPTION}
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

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(smartoil)
endif()


set(OpenCV_DIR "C:/opencv/build_mingw")
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui dnn objdetect videoio)
target_link_libraries(smartoil PRIVATE ${OpenCV_LIBS})

# Copy face recognition model files next to the executable at build time
add_custom_command(TARGET smartoil POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/models/face_detection_yunet_2023mar.onnx"
        "$<TARGET_FILE_DIR:smartoil>/face_detection_yunet_2023mar.onnx"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/models/face_recognition_sface_2021dec.onnx"
        "$<TARGET_FILE_DIR:smartoil>/face_recognition_sface_2021dec.onnx"
)

# Copy OpenCV MinGW DLLs next to the executable at build time
foreach(_dll
    libopencv_core4120
    libopencv_imgproc4120
    libopencv_imgcodecs4120
    libopencv_highgui4120
    libopencv_dnn4120
    libopencv_objdetect4120
    libopencv_videoio4120
    libopencv_calib3d4120
    libopencv_features2d4120
    libopencv_flann4120
)
    add_custom_command(TARGET smartoil POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "C:/opencv/build_mingw/bin/${_dll}.dll"
            "$<TARGET_FILE_DIR:smartoil>/${_dll}.dll"
    )
endforeach()
# ffmpeg helper DLL (different naming)
add_custom_command(TARGET smartoil POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/opencv/build_mingw/bin/opencv_videoio_ffmpeg4120_64.dll"
        "$<TARGET_FILE_DIR:smartoil>/opencv_videoio_ffmpeg4120_64.dll"
)