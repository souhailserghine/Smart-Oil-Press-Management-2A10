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
    agriculteur.cpp
    agriculteur.h
    agriculteurmodule.cpp
    agriculteurmodule.h
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

target_include_directories(smartoil PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
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
# OpenCV (optionnel — désactivé si non compatible avec le compilateur)
# =========================

set(OpenCV_DIR "C:/opencv/build_mingw")

find_package(OpenCV QUIET COMPONENTS
    core
    imgproc
    imgcodecs
    highgui
    dnn
    objdetect
    videoio
)

if(OpenCV_FOUND)
    message(STATUS "OpenCV trouvé : ${OpenCV_VERSION}")
    target_link_libraries(smartoil PRIVATE ${OpenCV_LIBS})
    target_compile_definitions(smartoil PRIVATE OPENCV_AVAILABLE)

    # Chercher le dossier des DLLs
    if(EXISTS "C:/opencv/build_mingw/bin")
        set(OPENCV_BIN_DIR "C:/opencv/build_mingw/bin")
    elseif(EXISTS "C:/opencv/build_mingw/x64/mingw/bin")
        set(OPENCV_BIN_DIR "C:/opencv/build_mingw/x64/mingw/bin")
    else()
        set(OPENCV_BIN_DIR "")
    endif()

    if(OPENCV_BIN_DIR)
        message(STATUS "Using OPENCV_BIN_DIR: ${OPENCV_BIN_DIR}")
        file(GLOB OPENCV_RUNTIME_DLLS
            "${OPENCV_BIN_DIR}/libopencv_*.dll"
            "${OPENCV_BIN_DIR}/opencv_videoio_ffmpeg*.dll"
        )
        foreach(_dll ${OPENCV_RUNTIME_DLLS})
            add_custom_command(TARGET smartoil POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_dll}"
                    "$<TARGET_FILE_DIR:smartoil>"
            )
        endforeach()
    endif()
else()
    message(WARNING "OpenCV non trouvé ou incompatible avec ce compilateur. La reconnaissance faciale sera désactivée.")
    target_compile_definitions(smartoil PRIVATE OPENCV_UNAVAILABLE)
endif()

# =========================
# Face recognition models (optionnel — seulement si OpenCV disponible)
# =========================

if(OpenCV_FOUND)
    set(FACE_DETECTION_MODEL
        "${CMAKE_SOURCE_DIR}/models/face_detection_yunet_2023mar.onnx"
    )

    set(FACE_RECOGNITION_MODEL
        "${CMAKE_SOURCE_DIR}/models/face_recognition_sface_2021dec.onnx"
    )

    if(NOT EXISTS "${FACE_DETECTION_MODEL}")
        message(WARNING "Modèle manquant : ${FACE_DETECTION_MODEL}")
    else()
        add_custom_command(TARGET smartoil POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FACE_DETECTION_MODEL}"
                "$<TARGET_FILE_DIR:smartoil>/face_detection_yunet_2023mar.onnx"
        )
    endif()

    if(NOT EXISTS "${FACE_RECOGNITION_MODEL}")
        message(WARNING "Modèle manquant : ${FACE_RECOGNITION_MODEL}")
    else()
        add_custom_command(TARGET smartoil POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${FACE_RECOGNITION_MODEL}"
                "$<TARGET_FILE_DIR:smartoil>/face_recognition_sface_2021dec.onnx"
        )
    endif()
endif()

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(smartoil)
endif()
