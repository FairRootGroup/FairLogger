################################################################################
# Copyright (C) 2018-2019 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

cmake_minimum_required(VERSION 3.9.4 FATAL_ERROR)
cmake_policy(VERSION 3.9...3.15)

# Project ######################################################################
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
include(FairLoggerLib)

get_git_version()

project(FairLogger VERSION ${PROJECT_VERSION} LANGUAGES C CXX)
message(STATUS "${BWhite}${PROJECT_NAME}${CR} ${PROJECT_GIT_VERSION} from ${PROJECT_DATE}")

set_fairlogger_defaults()

include(CTest)

option(USE_BOOST_PRETTY_FUNCTION "Use Boost BOOST_PRETTY_FUNCTION macro" OFF)
option(USE_EXTERNAL_FMT "Use external fmt library instead of the bundled one" OFF)
################################################################################

# Dependencies #################################################################
if(USE_BOOST_PRETTY_FUNCTION)
  if(NOT DEFINED Boost_NO_BOOST_CMAKE AND CMAKE_VERSION VERSION_LESS 3.15)
    # Since Boost 1.70 a CMake package is shipped by default. Unfortunately, it has a number
    # of problems that are only fixed in Boost 1.71 or CMake 3.15. By default we skip the
    # BoostConfig lookup. This can be overridden on the command line via -DBoost_NO_BOOST_CMAKE=OFF
    set(Boost_NO_BOOST_CMAKE ON)
  endif()
  find_package2(PUBLIC Boost REQUIRED)
endif()

if(USE_EXTERNAL_FMT)
  find_package2(PUBLIC fmt REQUIRED VERSION 5.3.0)
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

# legacy paths
list(APPEND FAIRLOGGER_INSTALL_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR})

if(USE_EXTERNAL_FMT)
  target_link_libraries(FairLogger PUBLIC fmt::fmt)
else()
  add_library(fmt INTERFACE)
  target_include_directories(fmt INTERFACE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/logger/bundled>
    $<INSTALL_INTERFACE:${PROJECT_INSTALL_INCDIR}/bundled>
  )
  target_compile_definitions(fmt INTERFACE FMT_HEADER_ONLY)
  target_link_libraries(FairLogger PUBLIC fmt)
  list(APPEND FAIRLOGGER_INSTALL_INCLUDE_DIRS ${CMAKE_INSTALL_PREFIX}/${PROJECT_INSTALL_INCDIR}/bundled)
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
if(NOT USE_EXTERNAL_FMT)
  set(fmt_target fmt)
endif()
install(TARGETS
  FairLogger
  ${fmt_target}

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

if(NOT USE_EXTERNAL_FMT)
  install(DIRECTORY
    logger/bundled
    DESTINATION ${PROJECT_INSTALL_INCDIR}
  )
endif()

install_cmake_package()
################################################################################

# Testing ######################################################################
if(BUILD_TESTING)
  add_test(NAME loggerTest COMMAND $<TARGET_FILE:loggerTest>)
endif()
################################################################################

# Summary ######################################################################
message(STATUS "  ")
message(STATUS "  ${Cyan}CXX STANDARD${CR}       ${BGreen}C++${CMAKE_CXX_STANDARD}${CR} (>= C++${PROJECT_MIN_CXX_STANDARD}, change with ${BMagenta}-DCMAKE_CXX_STANDARD=17${CR})")
if(CMAKE_CXX_FLAGS)
  message(STATUS "  ")
  message(STATUS "  ${Cyan}GLOBAL CXX FLAGS${CR}  ${BGreen}${CMAKE_CXX_FLAGS}${CR}")
endif()
if(CMAKE_CONFIGURATION_TYPES)
  message(STATUS "  ")
  message(STATUS "  ${Cyan}BUILD TYPE         CXX FLAGS${CR}")
  string(TOUPPER "${CMAKE_BUILD_TYPE}" selected_type)
  foreach(type IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER "${type}" type_upper)
    if(type_upper STREQUAL selected_type)
      pad("${type}" 18 " " type_padded)
      message(STATUS "${BGreen}* ${type_padded}${CMAKE_CXX_FLAGS_${type_upper}}${CR}")
    else()
      pad("${type}" 18 " " type_padded)
      message(STATUS "  ${BWhite}${type_padded}${CR}${CMAKE_CXX_FLAGS_${type_upper}}")
    endif()
    unset(type_padded)
    unset(type_upper)
  endforeach()
  message(STATUS "  ")
  message(STATUS "  (Change the build type with ${BMagenta}-DCMAKE_BUILD_TYPE=...${CR})")
endif()
if(PROJECT_PACKAGE_DEPENDENCIES)
  message(STATUS "  ")
  message(STATUS "  ${Cyan}DEPENDENCY FOUND     VERSION                   PREFIX${CR}")
  foreach(dep IN LISTS PROJECT_PACKAGE_DEPENDENCIES)
    if(${dep}_VERSION)
      if(${dep} STREQUAL Boost)
        if(Boost_VERSION_MAJOR)
          set(version_str "${BGreen}${${dep}_VERSION_MAJOR}.${${dep}_VERSION_MINOR}${CR}")
        else()
          set(version_str "${BGreen}${${dep}_MAJOR_VERSION}.${${dep}_MINOR_VERSION}${CR}")
        endif()
      else()
        set(version_str "${BGreen}${${dep}_VERSION}${CR}")
      endif()
    else()
      set(version_str "${BYellow}unknown${CR}")
    endif()
    if(PROJECT_${dep}_VERSION)
      set(version_req_str " (>= ${PROJECT_${dep}_VERSION})")
    endif()
    pad(${dep} 20 " " dep_padded)
    if(DISABLE_COLOR)
      pad("${version_str}${version_req_str}" 25 " " version_padded)
    else()
      pad("${version_str}${version_req_str}" 25 " " version_padded COLOR 1)
    endif()
    if(${dep} STREQUAL Boost)
      if(TARGET Boost::headers)
        get_target_property(boost_include Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
      else()
        get_target_property(boost_include Boost::boost INTERFACE_INCLUDE_DIRECTORIES)
      endif()
      get_filename_component(prefix ${boost_include}/.. ABSOLUTE)
    elseif(${dep} STREQUAL fmt)
      get_target_property(fmt_include fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
      get_filename_component(prefix ${fmt_include}/.. ABSOLUTE)
    else()
      get_filename_component(prefix ${${dep}_INCLUDE_DIR}/.. ABSOLUTE)
    endif()
    message(STATUS "  ${BWhite}${dep_padded}${CR}${version_padded}${prefix}")
    unset(version_str)
    unset(version_padded)
    unset(version_req_str)
  endforeach()
endif()
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
message(STATUS "  ${Cyan}INSTALL PREFIX${CR}     ${BGreen}${CMAKE_INSTALL_PREFIX}${CR} (change with ${BMagenta}-DCMAKE_INSTALL_PREFIX=...${CR})")
message(STATUS "  ")
################################################################################
