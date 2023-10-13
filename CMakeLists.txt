cmake_minimum_required(VERSION 3.5)

#   ____       _   _   _                 
#  / ___|  ___| |_| |_(_)_ __   __ _ ___ 
#  \___ \ / _ \ __| __| | '_ \ / _` / __|
#   ___) |  __/ |_| |_| | | | | (_| \__ \
#  |____/ \___|\__|\__|_|_| |_|\__, |___/
#                              |___/     
set(APP_NAME telemetry_dev)
set(APP_DOMAIN it.unitn)
project(${APP_NAME} VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QT-related
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets PrintSupport)
find_package(Qt6 REQUIRED COMPONENTS Widgets PrintSupport)


#  __     __             
#  \ \   / /_ _ _ __ ___ 
#   \ \ / / _` | '__/ __|
#    \ V / (_| | |  \__ \
#     \_/ \__,_|_|  |___/
                       
set(EXTERNAL_DIR ${CMAKE_CURRENT_LIST_DIR}/external)
set(USR_DIR ${EXTERNAL_DIR}/usr)
set(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(MAIN_DIR ${SOURCE_DIR}/main)
file(GLOB LIB_SOURCES ${SOURCE_DIR}/*(.h|.hpp|.cpp))
file(GLOB UI_SOURCES ${SOURCE_DIR}/*(.ui))
set(PROJECT_SOURCES
  ${LIB_SOURCES}
  ${UI_SOURCES}
  ${MAIN_DIR}/main.cpp
  ${MAIN_DIR}/pub.cpp
  ${MAIN_DIR}/sub.cpp
)

configure_file(
  ${SOURCE_DIR}/defines.h.in 
  ${SOURCE_DIR}/defines.h
)


#   _____                    _       
#  |_   _|_ _ _ __ __ _  ___| |_ ___ 
#    | |/ _` | '__/ _` |/ _ \ __/ __|
#    | | (_| | | | (_| |  __/ |_\__ \
#    |_|\__,_|_|  \__, |\___|\__|___/
#                 |___/              

include_directories(${USR_DIR}/include)
link_directories(${USR_DIR}/lib)

# List of required libraries
if(WIN32)
  set(LIB_LIST snappy.lib zmqpp-static.lib libzmq-static-mt-s-4_3_5.lib Ws2_32 Iphlpapi)
else()
  set(LIB_LIST libzmqpp-static.a ${PTHREAD} libzmq.a libsnappy.a)
endif()

# CLI targets
add_executable(pub ${MAIN_DIR}/pub.cpp)
target_link_libraries(pub ${LIB_LIST})


add_executable(sub ${MAIN_DIR}/sub.cpp)
target_link_libraries(sub ${LIB_LIST})


# QT Target
qt_add_executable(${APP_NAME}
  MANUAL_FINALIZATION
  ${LIB_SOURCES} ${MAIN_DIR}/main.cpp
)

target_link_libraries(${APP_NAME} PRIVATE 
  Qt${QT_VERSION_MAJOR}::Widgets 
  Qt${QT_VERSION_MAJOR}::PrintSupport
  ${LIB_LIST}
)

set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER ${APP_DOMAIN}.${APP_NAME})

set_target_properties(${APP_NAME} PROPERTIES
  ${BUNDLE_ID_OPTION}
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
  AUTOMOC TRUE
  AUTOUIC TRUE
  AUTORCC TRUE
)

qt_finalize_executable(${APP_NAME})


#   ___           _        _ _ 
#  |_ _|_ __  ___| |_ __ _| | |
#   | || '_ \/ __| __/ _` | | |
#   | || | | \__ \ || (_| | | |
#  |___|_| |_|___/\__\__,_|_|_|
                             
set(TARGET_LIST pub sub ${APP_NAME})


set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_LIST_DIR}/products)
install(TARGETS ${TARGET_LIST}
    BUNDLE DESTINATION bin
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

#   _____      _                        _     
#  | ____|_  _| |_ ___ _ __ _ __   __ _| |___ 
#  |  _| \ \/ / __/ _ \ '__| '_ \ / _` | / __|
#  | |___ >  <| ||  __/ |  | | | | (_| | \__ \
#  |_____/_/\_\\__\___|_|  |_| |_|\__,_|_|___/
                                            
if (NOT EXISTS ${USR_DIR})
  message(STATUS "Compiling third party libraries")
  include(external.cmake)
  foreach(t IN ITEMS ${TARGET_LIST})
    add_dependencies(${t} libzmq zmqpp snappy)
  endforeach()
endif()



