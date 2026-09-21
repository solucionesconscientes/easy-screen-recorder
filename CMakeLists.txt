# SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.25)

project(easy-screen-recorder
        VERSION 0.10.0
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
option(ESR_WERROR "Tratar los warnings como errores" ON)

add_library(esr_avisos INTERFACE)
target_compile_options(esr_avisos INTERFACE
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion)
if(ESR_WERROR)
  target_compile_options(esr_avisos INTERFACE -Werror)
endif()

add_subdirectory(src/core)
add_subdirectory(src/cli)
add_subdirectory(src/ui)

# Las paginas de manual van con el proyecto y no con el empaquetado de Debian:
# documentan el comportamiento del programa, no como se empaqueta. Asi las tiene
# cualquier distribucion y tambien quien compila desde el codigo, y `lintian`
# deja de avisar de no-manual-page sin tener que duplicarlas en debian/.
include(GNUInstallDirs)
install(FILES docs/man/easy-screen-recorder.1
              docs/man/easy-screen-recorder-cli.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
