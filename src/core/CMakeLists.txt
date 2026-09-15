# libesr: nucleo sin GUI y sin Qt. Todo lo que la UI y el CLI necesitan
# saber del entorno vive aqui, para que la capa 3 nunca ofrezca algo que vaya
# a fallar.
add_library(libesr STATIC
  capacidades.cpp
  configuracion.cpp
  entorno.cpp
  grabacion.cpp
  ajustes.cpp
  audio.cpp
  ipc.cpp
  json_ipc.cpp
  proceso.cpp
  version.cpp
)

set_target_properties(libesr PROPERTIES OUTPUT_NAME esr)

target_include_directories(libesr PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(libesr PRIVATE esr_avisos)

add_library(esr::core ALIAS libesr)
