function(target_resource_pak target_name)
  set(res_file ${CMAKE_CURRENT_BINARY_DIR}/${target_name}.pak)

  add_custom_target(${target_name}-pak
    COMMAND python3 ${CMAKE_SOURCE_DIR}/scripts/respak.py ${res_file} ${ARGN}
    BYPRODUCTS ${res_file}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building resource pak file '${res_file}'"
    VERBATIM
    COMMAND_EXPAND_LISTS
    SOURCES ${ARGN}
  )

  add_dependencies(${target_name} ${target_name}-pak)
endfunction()
