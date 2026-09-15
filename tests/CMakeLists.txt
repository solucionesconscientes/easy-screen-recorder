# Sin libreria de tests: comprobar.hpp basta y no añade dependencias.
function(esr_prueba nombre)
  add_executable(${nombre} ${nombre}.cpp)
  target_link_libraries(${nombre} PRIVATE esr::core esr_avisos)
  target_include_directories(${nombre} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(${nombre} PRIVATE
    ESR_DIR_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    ESR_VOLCADO_REAL="${CMAKE_SOURCE_DIR}/docs/gsr-capabilities.txt"
    # La version de project() llega al test para compararla con
    # kVersionEsr: estan duplicadas a pelo y sin esto se separan sin que
    # nadie lo note.
    ESR_VERSION_CMAKE="${PROJECT_VERSION}")
  add_test(NAME ${nombre} COMMAND ${nombre})
endfunction()

esr_prueba(prueba_version)
esr_prueba(prueba_proceso)
esr_prueba(prueba_capacidades)
esr_prueba(prueba_entorno)
esr_prueba(prueba_json_ipc)
esr_prueba(prueba_ajustes)
esr_prueba(prueba_grabacion)
esr_prueba(prueba_audio)
esr_prueba(prueba_configuracion)

# El CLI tiene que responder a --version y --check sin reventar aunque no haya
# nada instalado. --check devuelve 1 en una maquina sin GSR, y eso es correcto:
# el test comprueba que imprime el diagnostico, no que este todo bien.
add_test(NAME cli_version COMMAND esr_cli --version)
set_tests_properties(cli_version PROPERTIES PASS_REGULAR_EXPRESSION "easy-screen-recorder-cli [0-9]+\\.[0-9]+\\.[0-9]+")

add_test(NAME cli_check COMMAND esr_cli --check)
set_tests_properties(cli_check PROPERTIES
  PASS_REGULAR_EXPRESSION "Resultado: (LISTO|NO LISTO)")

add_test(NAME cli_orden_desconocida COMMAND esr_cli --inventada)
set_tests_properties(cli_orden_desconocida PROPERTIES WILL_FAIL TRUE)
