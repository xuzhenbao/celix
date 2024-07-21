# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

option(ENABLE_ADDRESS_SANITIZER "Enabled building with address sanitizer. Note for gcc libasan must be installed," OFF)
option(ENABLE_UNDEFINED_SANITIZER "Enabled building with undefined behavior sanitizer." OFF)
option(ENABLE_THREAD_SANITIZER "Enabled building with thread sanitizer." OFF)

# Clear "Advanced" flag for sanitizer options
mark_as_advanced(CLEAR ENABLE_ADDRESS_SANITIZER)
mark_as_advanced(CLEAR ENABLE_UNDEFINED_SANITIZER)
mark_as_advanced(CLEAR ENABLE_THREAD_SANITIZER)

if (ENABLE_ADDRESS_SANITIZER)
    if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
        set(CMAKE_C_FLAGS "-fsanitize=address -fno-omit-frame-pointer ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-fsanitize=address -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS}")
    elseif ("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
        set(CMAKE_C_FLAGS "-lasan -fsanitize=address -fno-omit-frame-pointer ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-lasan -fsanitize=address -fno-omit-frame-pointer ${CMAKE_CXX_FLAGS}")
    else ()
        message(WARNING "Address sanitizer is not supported for ${CMAKE_C_COMPILER_ID}")
    endif ()

    if (ENABLE_TESTING)
        set(CMAKE_C_FLAGS "-DCPPUTEST_MEM_LEAK_DETECTION_DISABLED ${CMAKE_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "-DCPPUTEST_MEM_LEAK_DETECTION_DISABLED ${CMAKE_CXX_FLAGS}")
    endif ()
endif()

if (ENABLE_UNDEFINED_SANITIZER)
    set(CMAKE_C_FLAGS "-fsanitize=undefined ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-fsanitize=undefined ${CMAKE_CXX_FLAGS}")
endif()

if (ENABLE_THREAD_SANITIZER)
    set(CMAKE_C_FLAGS "-fsanitize=thread ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "-fsanitize=thread ${CMAKE_CXX_FLAGS}")
endif()

MACRO(celix_subproject)
    set(ARGS "${ARGN}")

    list(GET ARGS 0 OPTION_NAME)
    list(REMOVE_AT ARGS 0)

    list(GET ARGS 0 OPTION_DESCRIPTION)
    list(REMOVE_AT ARGS 0)

    list(GET ARGS 0 OPTION_DEFAULT)
    list(REMOVE_AT ARGS 0)

    set(OPTIONS )
    set(ONE_VAL_ARGS )
    set(MULTI_VAL_ARGS DEPS)
    cmake_parse_arguments(OPTION "${OPTIONS}" "${ONE_VAL_ARGS}" "${MULTI_VAL_ARGS}" ${ARGS})

    string(TOUPPER ${OPTION_NAME} UC_OPTION_NAME)
    set(NAME "BUILD_${UC_OPTION_NAME}")
    
    option(${NAME} "${OPTION_DESCRIPTION}" ${OPTION_DEFAULT})
    
    IF (${NAME})
        set(${OPTION_NAME} "ON")
        FOREACH (DEP ${OPTION_DEPS})
            string(TOUPPER ${DEP} UC_DEP)
            set(DEP_NAME "BUILD_${UC_DEP}")
            if (NOT ${DEP_NAME})
                message(FATAL_ERROR "${DEP} is required by ${OPTION_NAME}: please add -D${DEP_NAME}:BOOL=ON")
            endif ()
        ENDFOREACH (DEP)
    ENDIF (${NAME})
ENDMACRO(celix_subproject)

#[[
Internal function that converts a property string to a JSON field entry.
The result is stored in the OUTPUT_VAR_NAME variable.

In the key the char `=` is not allowed and should be escaped as `\=` (in CMake this is `\\=`, because \ is already an
escape char in CMake).
In the value the char `=` is allowed.

To handle \= string sequences the \= entries are replaced with a placeholder (__<CELIX_ESCAPED_EQUAL>__) and after the
split the placeholder is replaced with =.

```CMake
_celix_convert_keyval_to_json("prop1=val1" "=" OUTPUT_VAR_NAME) # OUTPUT_VAR_NAME will be set to "\"prop1\":\"val1\""
_celix_convert_keyval_to_json("prop1=va=l1" "=" OUTPUT_VAR_NAME) # OUTPUT_VAR_NAME will be set to "\"prop1\":\"va=l1\""
_celix_convert_keyval_to_json("prop\\=1=val1" "=" OUTPUT_VAR_NAME) # OUTPUT_VAR_NAME will be set to "\"prop=1\":\"val1\""

_celix_convert_keyval_to_json(" prop1 = val1 " "=" OUTPUT_VAR_NAME) # OUTPUT_VAR_NAME will be set to "\"prop1\"":\"val1\""
```
]]
function(_celix_convert_keyval_to_json INPUT_STR SEPERATOR_CHAR OUTPUT_VAR_NAME)
    set(PLACEHOLDER "__<CELIX_ESCAPED_EQUAL>__")
    string(REPLACE "\\${SEPERATOR_CHAR}" "${PLACEHOLDER}" TEMP_INPUT_STR "${INPUT_STR}")

    string(REGEX MATCH "([^${SEPERATOR_CHAR}]+)${SEPERATOR_CHAR}(.*)" _ ${TEMP_INPUT_STR})
    set(KEY ${CMAKE_MATCH_1})
    set(VALUE ${CMAKE_MATCH_2})

    #Replace replaced \= and \\ with = and \
    string(REPLACE "${PLACEHOLDER}" "${SEPERATOR_CHAR}" KEY "${KEY}")
    string(REPLACE "${PLACEHOLDER}" "${SEPERATOR_CHAR}" VALUE "${VALUE}")

    #Strip leading and trailing spaces
    string(STRIP "${KEY}" KEY)
    string(STRIP "${VALUE}" VALUE)

    set(${OUTPUT_VAR_NAME} "\"${KEY}\":\"${VALUE}\"" PARENT_SCOPE)
endfunction()

#[[
Custom target which list the Celix CMake targets that are still using deprecated headers.
]]
if (NOT TARGET celix-deprecated)
    add_custom_target(celix-deprecated
        COMMAND ${CMAKE_COMMAND} -E echo "Targets still using deprecated utils headers: $<JOIN:$<TARGET_PROPERTY:celix-deprecated,UTIL_TARGETS>, >"
        COMMAND ${CMAKE_COMMAND} -E echo "Targets still using deprecated framework headers: $<JOIN:$<TARGET_PROPERTY:celix-deprecated,FRAMEWORK_TARGETS>, >"
    )
    set_target_properties(celix-deprecated PROPERTIES "UTIL_TARGETS" "")
    set_target_properties(celix-deprecated PROPERTIES "FRAMEWORK_TARGETS" "")

endif ()


#[[
Add include path for the Celix utils deprecated headers to the provided target (as PRIVATE)

```CMake
celix_deprecated_utils_headers(<target_name>))
```
]]
function(celix_deprecated_utils_headers)
    list(GET ARGN 0 TARGET_NAME)
    get_target_property(UTILS_SOURCE_DIR Celix::utils "SOURCE_DIR")
    target_include_directories(${TARGET_NAME} PRIVATE ${UTILS_SOURCE_DIR}/include_deprecated)
    set_property(TARGET celix-deprecated APPEND PROPERTY "UTIL_TARGETS" "${TARGET_NAME}")
endfunction()


#[[
Add include path for the Celix framework deprecated headers to the provided target (as PRIVATE)

```CMake
celix_deprecated_framework_headers(<target_name>))
```
]]
function(celix_deprecated_framework_headers)
    list(GET ARGN 0 TARGET_NAME)
    get_target_property(FRAMEWORK_SOURCE_DIR Celix::framework "SOURCE_DIR")
    target_include_directories(${TARGET_NAME} PRIVATE ${FRAMEWORK_SOURCE_DIR}/include_deprecated)
    set_property(TARGET celix-deprecated APPEND PROPERTY "FRAMEWORK_TARGETS" "${TARGET_NAME}")
endfunction()

include(${CMAKE_CURRENT_LIST_DIR}/ApacheRat.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/CodeCoverage.cmake)
