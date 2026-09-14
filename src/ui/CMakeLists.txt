# Capa 3: la UI Qt6/QML con Kirigami. Enlaza libcapturia directamente: es la
# capa 3 sobre la capa 1 del diagrama de CLAUDE.md; el CLI es un hermano, no
# un intermediario.
#
# La UI es opcional en el build a proposito: el CI no tiene Qt y el CLI y los
# tests tienen que seguir compilando alli. Si falta Qt se avisa y se sigue.
find_package(Qt6 6.4 QUIET COMPONENTS Core Gui Qml Quick QuickControls2 Widgets Concurrent)

if(NOT Qt6_FOUND)
  message(STATUS "Capturia: sin Qt6 de desarrollo; la UI no se compila (el CLI y los tests si)")
  return()
endif()

qt_standard_project_setup(REQUIRES 6.4)
# Politicas nuevas de Qt: prefijo de recursos moderno y qmldir por carpeta.
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(capturia_ui
  main.cpp
  controlador.cpp
)
set_target_properties(capturia_ui PROPERTIES OUTPUT_NAME capturia-ui)

qt_add_qml_module(capturia_ui
  URI org.capturia
  VERSION 1.0
  # Sin escaneo de imports: solo sirve para enlazar plugins QML estaticos y
  # este Qt es compartido. Con el puesto, la configuracion escupia veinte
  # avisos de plugins que no existen como objetivos.
  NO_IMPORT_SCAN
  QML_FILES
    qml/Main.qml
)

target_link_libraries(capturia_ui PRIVATE
  capturia::core
  Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Concurrent
)
# Los avisos duros del proyecto se aplican a NUESTRO codigo. El codigo que
# genera moc/qmltc no es nuestro y sus avisos no se arreglan aqui, asi que el
# objetivo de la UI lleva los flags directamente y no el INTERFACE global.
target_compile_options(capturia_ui PRIVATE -Wall -Wextra)

include(GNUInstallDirs)
install(TARGETS capturia_ui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES datos/org.capturia.Capturia.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES datos/org.capturia.Capturia.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES datos/org.capturia.Capturia.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
