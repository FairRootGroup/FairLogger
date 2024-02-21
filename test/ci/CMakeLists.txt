################################################################################
#   Copyright (C) 2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH     #
#                                                                              #
#              This software is distributed under the terms of the             #
#              GNU Lesser General Public Licence (LGPL) version 3,             #
#                  copied verbatim in the file "LICENSE"                       #
################################################################################

function(container)
  cmake_parse_arguments(ARGS "" "OS;VERSION" "" ${ARGN})

  set(container "${ARGS_OS}.${ARGS_VERSION}")
  set(def "${container}.def")
  set(log "${container}.log")
  set(target "${container}.sif")

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

container(OS fedora VERSION 32)
container(OS fedora VERSION 33)
container(OS fedora VERSION 34)
container(OS fedora VERSION 35)
container(OS fedora VERSION 36)
container(OS fedora VERSION 37)
container(OS fedora VERSION 38)
container(OS fedora VERSION 39)

add_custom_target(all-containers DEPENDS ${containers})
