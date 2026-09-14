# Capa 2. Regla dura del proyecto: si algo no funciona por CLI, no se toca la UI.
add_executable(capturia_cli main.cpp)
set_target_properties(capturia_cli PROPERTIES OUTPUT_NAME capturia)
target_link_libraries(capturia_cli PRIVATE capturia::core capturia_avisos)

include(GNUInstallDirs)
install(TARGETS capturia_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
