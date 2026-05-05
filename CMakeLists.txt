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

# ============================================================
# Qt — priorise Qt6, fallback Qt5
# ============================================================
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS
    Widgets Sql Charts PrintSupport SerialPort
)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Widgets Sql Charts PrintSupport SerialPort
)

# ============================================================
# Sources du projet
# ============================================================
set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    connection.cpp
    connection.h
    citernes.cpp
    citernes.h
    citernes.ui
    qualite.cpp
    qualite.h
    qualite.ui
    stocks.cpp
    stocks.h
    stocks.ui
    agriculteur.cpp
    agriculteur.h
    agriculteurmodule.cpp
    agriculteurmodule.h
    employe.cpp
    employe.h
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

# ============================================================
# Création de l'exécutable
# ============================================================
if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(smartoil
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(smartoil ${PROJECT_SOURCES})
endif()

# ============================================================
# Liens Qt
# ============================================================
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

# ============================================================
# Propriétés bundle (macOS / Windows)
# ============================================================
set_target_properties(smartoil PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.smartoil"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ============================================================
# Installation
# ============================================================
include(GNUInstallDirs)
install(TARGETS smartoil
    BUNDLE  DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# ============================================================
# OpenCV — OPTIONNEL
# Activé UNIQUEMENT si OpenCV_DIR est fourni explicitement
# Ex: -DOpenCV_DIR=C:/opencv_mingw/build/x64/mingw/lib
# ============================================================

# ON active OpenCV seulement si l'utilisateur a fourni OpenCV_DIR
if(DEFINED OpenCV_DIR AND NOT OpenCV_DIR STREQUAL "")

    message(STATUS "OpenCV_DIR fourni : ${OpenCV_DIR}")

    # Vérification que OpenCVConfig.cmake existe bien à cet endroit
    if(NOT EXISTS "${OpenCV_DIR}/OpenCVConfig.cmake")
        message(WARNING
            "OpenCVConfig.cmake introuvable dans : ${OpenCV_DIR}\n"
            "Verifie que OpenCV_DIR pointe vers le dossier contenant OpenCVConfig.cmake\n"
            "Exemple : C:/opencv_mingw/build/x64/mingw/lib"
        )
    else()
        find_package(OpenCV REQUIRED COMPONENTS
            core imgproc imgcodecs highgui dnn objdetect videoio
        )

        if(OpenCV_FOUND)
            message(STATUS "OpenCV ${OpenCV_VERSION} trouve et active.")
            target_compile_definitions(smartoil PRIVATE WITH_OPENCV)
            target_include_directories(smartoil PRIVATE ${OpenCV_INCLUDE_DIRS})
            target_link_libraries(smartoil PRIVATE ${OpenCV_LIBS})

            # -- Copie automatique des DLLs dans le dossier de build --
            get_filename_component(_opencv_bin "${OpenCV_DIR}/../bin" ABSOLUTE)
            if(EXISTS "${_opencv_bin}")
                file(GLOB _opencv_dlls
                    "${_opencv_bin}/libopencv_*.dll"
                    "${_opencv_bin}/opencv_*.dll"
                    "${_opencv_bin}/opencv_videoio_ffmpeg*.dll"
                )
                foreach(_dll ${_opencv_dlls})
                    add_custom_command(TARGET smartoil POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${_dll}" "$<TARGET_FILE_DIR:smartoil>"
                        COMMENT "Copie: ${_dll}"
                    )
                endforeach()
            else()
                message(WARNING "Dossier bin OpenCV introuvable : ${_opencv_bin}")
            endif()

            # -- Copie des modèles ONNX --
            set(_face_det  "${CMAKE_SOURCE_DIR}/models/face_detection_yunet_2023mar.onnx")
            set(_face_rec  "${CMAKE_SOURCE_DIR}/models/face_recognition_sface_2021dec.onnx")
            if(EXISTS "${_face_det}" AND EXISTS "${_face_rec}")
                add_custom_command(TARGET smartoil POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_face_det}"
                        "$<TARGET_FILE_DIR:smartoil>/face_detection_yunet_2023mar.onnx"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${_face_rec}"
                        "$<TARGET_FILE_DIR:smartoil>/face_recognition_sface_2021dec.onnx"
                    COMMENT "Copie des modeles ONNX"
                )
            else()
                message(WARNING
                    "Modeles ONNX manquants dans ${CMAKE_SOURCE_DIR}/models/\n"
                    "  face_detection_yunet_2023mar.onnx\n"
                    "  face_recognition_sface_2021dec.onnx"
                )
            endif()
        endif()
    endif()

else()
    # OpenCV_DIR non fourni → on compile sans OpenCV, sans erreur
    message(STATUS "")
    message(STATUS "====================================================")
    message(STATUS "  OpenCV DESACTIVE (OpenCV_DIR non defini)")
    message(STATUS "  La reconnaissance faciale ne sera pas disponible.")
    message(STATUS "----------------------------------------------------")
    message(STATUS "  Pour activer OpenCV :")
    message(STATUS "  1. Telechargez OpenCV MinGW :")
    message(STATUS "     https://github.com/huihut/OpenCV-MinGW-Build/releases")
    message(STATUS "  2. Extrayez dans ex: C:/opencv_mingw")
    message(STATUS "  3. Dans Qt Creator > Projects > Build > CMake,")
    message(STATUS "     ajoutez dans 'Additional CMake options' :")
    message(STATUS "     -DOpenCV_DIR=C:/opencv_mingw/build/x64/mingw/lib")
    message(STATUS "====================================================")
    message(STATUS "")
endif()

# ============================================================
# Finalisation Qt 6
# ============================================================
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(smartoil)
endif()
