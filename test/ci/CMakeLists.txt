################################################################################
#   Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH     #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

function(container)
  cmake_parse_arguments(ARGS "" "OS;VERSION;DOCKER_TAG;PACKAGE_SETUP_SCRIPT" "" ${ARGN})

  if(ARGS_DOCKER_TAG)
    set(DOCKER_TAG ${ARGS_DOCKER_TAG})
  else()
    set(DOCKER_TAG "${ARGS_OS}:${ARGS_VERSION}")
  endif()

  if(ARGS_PACKAGE_SETUP_SCRIPT)
    set(PACKAGE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/${ARGS_PACKAGE_SETUP_SCRIPT}")
  else()
    set(PACKAGE_SETUP_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/setup-${ARGS_OS}.sh")
  endif()

  set(container "${ARGS_OS}.${ARGS_VERSION}")
  set(def "${container}.def")
  set(log "${container}.log")
  set(target "${container}.fairlogger.sif")

  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/linux.def.in" ${def} @ONLY)

  add_custom_command(OUTPUT ${target}
    COMMAND ${CMAKE_COMMAND} -E env ${BASH} -c
      "${SINGULARITY} build -f -F ${target} ${def} > ${log} 2>&1"
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building fairlogger test container ${target}, logging to ${CMAKE_CURRENT_BINARY_DIR}/${log}"
    DEPENDS ${PACKAGE_SETUP_SCRIPT} ${bootstrap_cmake_script}
  )

  list(APPEND containers ${target})
  set(containers ${containers} PARENT_SCOPE)
endfunction()

container(OS fedora VERSION 33)

add_custom_target(all-containers DEPENDS ${containers})
