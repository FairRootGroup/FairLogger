################################################################################
# Copyright (C) 2018-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

cmake_minimum_required(VERSION 3.9.4 FATAL_ERROR)
cmake_policy(VERSION 3.9...3.14)

# Project ######################################################################
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(FairLoggerLib)

get_git_version()

project(FairLogger VERSION ${PROJECT_VERSION} LANGUAGES C CXX)
message(STATUS "${BWhite}${PROJECT_NAME}${CR} ${PROJECT_GIT_VERSION} from ${PROJECT_DATE}")

set_fairlogger_defaults()

include(CTest)

option(USE_BOOST_PRETTY_FUNCTION "Use Boost BOOST_PRETTY_FUNCTION macro" OFF)
################################################################################


# Dependency ###################################################################
if(USE_BOOST_PRETTY_FUNCTION)
  if(NOT DEFINED Boost_NO_BOOST_CMAKE AND CMAKE_VERSION VERSION_LESS 3.15)
    # Since Boost 1.70 a CMake package is shipped by default. Unfortunately, it has a number
    # of problems that are only fixed in Boost 1.71 or CMake 3.15. By default we skip the
    # BoostConfig lookup. This can be overridden on the command line via -DBoost_NO_BOOST_CMAKE=OFF
    set(Boost_NO_BOOST_CMAKE ON)
  endif()
  find_package2(PUBLIC Boost REQUIRED)
endif()
################################################################################


# Targets ######################################################################
# Configure Version.h
configure_file(logger/Version.h.in
  ${CMAKE_BINARY_DIR}/logger/Version.h
  @ONLY
)

add_library(FairLogger
  logger/Logger.cxx
  logger/Logger.h
)

if(USE_BOOST_PRETTY_FUNCTION)
  target_link_libraries(FairLogger PUBLIC Boost::boost)
  target_compile_definitions(FairLogger PUBLIC FAIRLOGGER_USE_BOOST_PRETTY_FUNCTION)
endif()

target_include_directories(FairLogger
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/logger>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_target_properties(FairLogger PROPERTIES
  VERSION ${PROJECT_GIT_VERSION}
  SOVERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
)

if(BUILD_TESTING)
  add_executable(loggerTest test/loggerTest.cxx)
  target_link_libraries(loggerTest FairLogger pthread)
endif()
################################################################################


# Installation #################################################################
if(BUILD_TESTING)
  set(test_targets ${targets} loggerTest)
endif()
install(TARGETS
  FairLogger
  ${test_targets}

  EXPORT ${PROJECT_EXPORT_SET}
  LIBRARY DESTINATION ${PROJECT_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${PROJECT_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${PROJECT_INSTALL_BINDIR}
)

install(FILES
  logger/Logger.h
  ${CMAKE_BINARY_DIR}/logger/Version.h

  DESTINATION ${PROJECT_INSTALL_INCDIR}
)

install_cmake_package()
################################################################################


# Testing ######################################################################
if(BUILD_TESTING)
  add_test(NAME loggerTest
    COMMAND $<TARGET_FILE:loggerTest>
  )
endif()
################################################################################


# Summary ######################################################################
message(STATUS "  ")
message(STATUS "  ${Cyan}CXX STANDARD${CR}       ${BGreen}C++${CMAKE_CXX_STANDARD}${CR} (>= C++${PROJECT_MIN_CXX_STANDARD}, change with ${BMagenta}-DCMAKE_CXX_STANDARD=17${CR})")
message(STATUS "  ")
message(STATUS "  ${Cyan}COMPONENT  BUILT?  INFO${CR}")
message(STATUS "  ${BWhite}library${CR}     ${BGreen}YES${CR}    (default, always built)")
if(BUILD_TESTING)
  set(testing_summary "${BGreen}YES${CR}    (default, disable with ${BMagenta}-DBUILD_TESTING=OFF${CR})")
else()
  set(testing_summary "${BRed} NO${CR}    (enable with ${BMagenta}-DBUILD_TESTING=ON${CR})")
endif()
message(STATUS "  ${BWhite}tests${CR}       ${testing_summary}")
message(STATUS "  ")
################################################################################
