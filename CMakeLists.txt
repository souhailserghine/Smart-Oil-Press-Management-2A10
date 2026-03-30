cmake_minimum_required(VERSION 3.16)

project(Atelier_Connexion LANGUAGES CXX)
set(TARGET_NAME Atelier_Connexion)

# Qt6 requires C++17 (and it's also fine for Qt5)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Sql Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Sql Charts)

add_executable(${TARGET_NAME}
    main.cpp
    connection.cpp
    connection.h
    mainwindow.ui
    mainwindow.h
    mainwindow.cpp
    style.qrc
    img.qrc
)

target_link_libraries(${TARGET_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Charts
)
