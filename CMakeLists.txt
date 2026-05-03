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

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(smartoil)
endif()
