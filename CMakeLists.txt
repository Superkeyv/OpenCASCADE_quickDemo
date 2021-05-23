cmake_minimum_required(VERSION 3.5)

project(OpenCascade_Learn LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


###### 增加单元测试内容

if(${CMAKE_BUILD_TYPE} MATCHES "Debug")
add_definitions(-DDEBUG)
add_definitions(-DLOG4CPP_FIX_ERROR_COLLISION)
endif()

enable_testing(true)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check https://doc.qt.io/qt/deployment-android.html for more information.
# They need to be set before the find_package( ...) calls below.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(LIBS Qt${QT_VERSION_MAJOR}::Widgets)

find_package(VTK REQUIRED)
if(${VTK_FOUND})
    message(STATUS "VTK found!")
    message(STATUS "    version: ${VTK_VERSION}")
#    message(STATUS "    LIBS: ${VTK_LIBRARIES}")

    include_directories(${VTK_INCLUDE_DIRS})

    list(REMOVE_ITEM VTK_LIBRARIES
        "vtkIOODBC" "vtkIOPostgreSQL" "vtkIOMySQL" "vtkIOPDAL" "vtkGeovisGDAL" "vtkIOGDAL" "vtkViewsContext2D"
        "vtkIOGeoJSON" "vtkTestingIOSQL")

    list(APPEND LIBS ${VTK_LIBRARIES})
#    list(REMOVE_ITEM LIBS "vtkIOODBC")
endif(${VTK_FOUND})


set(OpenCASCADE_DIR "${THIRD_LIBS}/opencascade-7.5.0/build/release/lib/cmake/opencascade")
#set(CMAKE_PREFIX_PATH "${THIRD_LIBS}/opencascade-7.5.0/build/release")
find_package(OpenCASCADE REQUIRED)
#message(STATUS "OpenCASCADE_INSTALL_PREFIX: ${OpenCASCADE_INSTALL_PREFIX}")
### 添加 OpenCASCADE支持
if(${OpenCASCADE_FOUND})
    message(STATUS "OpenCASCADE found!")
    message(STATUS "    version: ${OpenCASCADE_VERSION}")
    message(STATUS "    OpenCASCADE_INSTALL_PREFIX: ${OpenCASCADE_INSTALL_PREFIX}")
#    message(STATUS "    INCLUDE: ${OpenCASCADE_INCLUDE_DIR}")
#    message(STATUS "    LIBS: ${OpenCASCADE_LIBRARIES}")

    list(APPEND LIBS ${OpenCASCADE_LIBRARIES})
    include_directories(${OpenCASCADE_INCLUDE_DIR})
endif(${OpenCASCADE_FOUND})

qt5_add_resources(RESOURCE_FILES image.qrc)

set(BASE_SRC
    Gglobal.h
    mainwindow.cpp
    mainwindow.h
    ModelView.cpp
    ModelView.h
    OcctWindow.cpp
    OcctWindow.h
    ${RESOURCE_FILES}
)

set(PROJECT_SOURCES
    main.cpp
    ${BASE_SRC}
)



if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(OpenCascade_Learn
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(OpenCascade_Learn SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(OpenCascade_Learn
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(OpenCascade_Learn PRIVATE ${LIBS})



############## 单元测试内容 ################
# 配置测试二进制文件存放目录
set(EXECUTABLE_OUTPUT_PATH "${PROJECT_BINARY_DIR}/bin/test")

set(TEST_SRC
    test_geom.cpp
)

source_group("Tests" FILES ${TEST_SRC})

add_executable(test_geom
    ${BASE_SRC}
    ${TEST_SRC}
)
target_link_libraries(test_geom ${LIBS} cppunit)
add_test(NAME test_geom COMMAND "${PROJECT_BINARY_DIR}/bin/test/test_geom")
set_tests_properties(test_geom PROPERTIES FAIL_REGULAR_EXPRESSION "failed")
