cmake_minimum_required(VERSION 3.10)
project(resources_compiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

execute_process(COMMAND echo "Run conan install...")
execute_process(COMMAND conan install . --build missing WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})

include(${PROJECT_SOURCE_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(TARGET_SRC "resources_compiler.cpp")
add_executable(${PROJECT_NAME} ${TARGET_SRC})
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})
