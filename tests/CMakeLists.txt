# Sin libreria de tests: comprobar.hpp basta y no añade dependencias.
function(capturia_prueba nombre)
  add_executable(${nombre} ${nombre}.cpp)
  target_link_libraries(${nombre} PRIVATE capturia::core capturia_avisos)
  target_include_directories(${nombre} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(${nombre} PRIVATE
    CAPTURIA_DIR_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
    CAPTURIA_VOLCADO_REAL="${CMAKE_SOURCE_DIR}/docs/gsr-capabilities.txt"
    # La version de project() llega al test para compararla con
    # kVersionCapturia: estan duplicadas a pelo y sin esto se separan sin que
    # nadie lo note.
    CAPTURIA_VERSION_CMAKE="${PROJECT_VERSION}")
  add_test(NAME ${nombre} COMMAND ${nombre})
endfunction()

capturia_prueba(prueba_version)
capturia_prueba(prueba_proceso)
capturia_prueba(prueba_capacidades)
capturia_prueba(prueba_entorno)
capturia_prueba(prueba_json_ipc)
capturia_prueba(prueba_ajustes)
capturia_prueba(prueba_grabacion)
capturia_prueba(prueba_audio)
capturia_prueba(prueba_configuracion)

# El CLI tiene que responder a --version y --check sin reventar aunque no haya
# nada instalado. --check devuelve 1 en una maquina sin GSR, y eso es correcto:
# el test comprueba que imprime el diagnostico, no que este todo bien.
add_test(NAME cli_version COMMAND capturia_cli --version)
set_tests_properties(cli_version PROPERTIES PASS_REGULAR_EXPRESSION "capturia [0-9]+\\.[0-9]+\\.[0-9]+")

add_test(NAME cli_check COMMAND capturia_cli --check)
set_tests_properties(cli_check PROPERTIES
  PASS_REGULAR_EXPRESSION "Resultado: (LISTO|NO LISTO)")

add_test(NAME cli_orden_desconocida COMMAND capturia_cli --inventada)
set_tests_properties(cli_orden_desconocida PROPERTIES WILL_FAIL TRUE)
