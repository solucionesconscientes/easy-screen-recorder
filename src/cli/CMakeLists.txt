# SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Capa 2. Regla dura del proyecto: si algo no funciona por CLI, no se toca la UI.
add_executable(esr_cli main.cpp)
set_target_properties(esr_cli PROPERTIES OUTPUT_NAME easy-screen-recorder-cli)
target_link_libraries(esr_cli PRIVATE esr::core esr_avisos)

include(GNUInstallDirs)
install(TARGETS esr_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
