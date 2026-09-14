cmake_minimum_required(VERSION 3.25)

project(capturia
        VERSION 0.1.0
        DESCRIPTION "Grabador de pantalla para Linux/Wayland sobre gpu-screen-recorder"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

# La verificacion de cada tanda exige "compila sin warnings", asi que por
# defecto un warning es un error. Se puede bajar en una maquina con un compilador
# mas nuevo que saque avisos nuevos, pero entonces hay que arreglarlos.
option(CAPTURIA_WERROR "Tratar los warnings como errores" ON)

add_library(capturia_avisos INTERFACE)
target_compile_options(capturia_avisos INTERFACE
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
if(CAPTURIA_WERROR)
  target_compile_options(capturia_avisos INTERFACE -Werror)
endif()

add_subdirectory(src/core)
add_subdirectory(src/cli)
add_subdirectory(src/ui)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
