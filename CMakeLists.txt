cmake_minimum_required(VERSION 3.5)

project(telemetry_dev VERSION 0.1 LANGUAGES CXX)

#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets PrintSupport)
find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport)

set(EXTERNAL_DIR ${PROJECT_SOURCE_DIR}/external)
set(USR_DIR ${EXTERNAL_DIR}/usr)
set(SRC_DIR ${EXTERNAL_DIR}/src)
set(SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(MAIN_DIR ${SOURCE_DIR}/main)
include_directories(${USR_DIR}/include)
link_directories(${USR_DIR}/lib)

file(GLOB LIB_SOURCES ${SOURCE_DIR}/*(.h|.hpp|.cpp|.ui))
set(PROJECT_SOURCES
  ${LIB_SOURCES}
  ${MAIN_DIR}/main.cpp
  ${MAIN_DIR}/pub.cpp
  ${MAIN_DIR}/sub.cpp
)


# TARGETS


qt_add_executable(telemetry_dev
  MANUAL_FINALIZATION
  ${LIB_SOURCES} ${MAIN_DIR}/main.cpp
)


target_link_libraries(telemetry_dev PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::PrintSupport)

set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER it.unitn.telemetry)

set_target_properties(telemetry_dev PROPERTIES
  ${BUNDLE_ID_OPTION}
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
  AUTOMOC TRUE
  AUTOUIC TRUE
  AUTORCC TRUE
)

qt_finalize_executable(telemetry_dev)

if(WIN32)
  set(LIB_LIST snappy.lib zmqpp-static.lib libzmq-static-mt-s-4_3_5.lib Ws2_32 Iphlpapi)
else()
  set(LIB_LIST libzmqpp-static.a ${PTHREAD} libzmq.a libsnappy.a)
endif()


add_executable(pub ${MAIN_DIR}/pub.cpp)
target_link_libraries(pub ${LIB_LIST})


add_executable(sub ${MAIN_DIR}/sub.cpp)
target_link_libraries(sub ${LIB_LIST})

target_link_libraries(telemetry_dev PRIVATE ${LIB_LIST})


set(TARGET_LIST pub sub telemetry_dev)


set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/products)
install(TARGETS ${TARGET_LIST}
    BUNDLE DESTINATION bin
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# External libraries
if (NOT EXISTS ${USR_DIR})
  message(STATUS "Compiling third party libraries")
  include(external.cmake)
  foreach(t IN ITEMS ${TARGET_LIST})
    add_dependencies(${t} libzmq zmqpp snappy)
  endforeach()
endif()



