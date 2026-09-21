# SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Capa 3: la UI Qt6/QML con Kirigami. Enlaza libesr directamente: es la
# capa 3 sobre la capa 1 del diagrama de CLAUDE.md; el CLI es un hermano, no
# un intermediario.
#
# La UI es opcional en el build a proposito: el CI no tiene Qt y el CLI y los
# tests tienen que seguir compilando alli. Si falta Qt se avisa y se sigue.
# Multimedia va en OPTIONAL_COMPONENTS y no en una llamada aparte, y eso no es
# estilo: `find_package(Qt6 COMPONENTS X)` con X ausente pone Qt6_FOUND a FALSE.
# Puesto en una segunda llamada, el `if(NOT Qt6_FOUND)` de abajo se disparaba y
# la interfaz entera dejaba de compilarse en silencio, con el binario viejo
# todavia en build/ y pareciendo que todo iba bien.
find_package(Qt6 6.4 QUIET
  COMPONENTS Core DBus Gui Qml Quick QuickControls2 Widgets Concurrent
  OPTIONAL_COMPONENTS Multimedia)

# Multimedia es OPCIONAL, y esa decision tiene motivo. De ella dependen dos
# cosas —la vista previa de la camara y el vumetro del microfono— y ninguna de
# las dos graba nada: son ayudas para acertar antes de empezar. Exigirla dejaria
# sin interfaz a una maquina que solo tiene el Qt basico, que es peor trato que
# quedarse sin dos ayudas.
#
# El paquete esta en el SDK de Flatpak (org.kde.Sdk 6.11), que es donde se
# compila lo que se publica, asi que el usuario final las tiene.

if(NOT Qt6_FOUND)
  message(STATUS "Easy Screen Recorder: sin Qt6 de desarrollo; la UI no se compila (el CLI y los tests si)")
  return()
endif()

qt_standard_project_setup(REQUIRES 6.4)
# Politicas nuevas de Qt: prefijo de recursos moderno y qmldir por carpeta.
qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

qt_add_executable(esr_ui
  atajos.cpp
  main.cpp
  medidor.cpp
  controlador.cpp
  reloj_bandeja.cpp
)
set_target_properties(esr_ui PROPERTIES OUTPUT_NAME easy-screen-recorder)

# Sistema.qml es un singleton: los numeros del diseño se leen desde cualquier
# fichero QML sin instanciarlo, que es justo lo que se quiere de una escala.
set_source_files_properties(qml/Sistema.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(esr_ui
  URI es.solucionesconscientes.esr
  VERSION 1.0
  # Sin escaneo de imports: solo sirve para enlazar plugins QML estaticos y
  # este Qt es compartido. Con el puesto, la configuracion escupia veinte
  # avisos de plugins que no existen como objetivos.
  NO_IMPORT_SCAN
  QML_FILES
    qml/Main.qml
    qml/Sistema.qml
    qml/FilaAjuste.qml
    qml/CabeceraSeccion.qml
    qml/SelectorRegion.qml
    qml/ColocarCamara.qml
)

# La vista previa entra en el modulo SOLO si hay Multimedia. Asi, sin el
# paquete, el fichero ni existe para QML y no hay un import que falle ni un
# aviso por consola: simplemente no se ofrece el control.
if(Qt6Multimedia_FOUND)
  qt_target_qml_sources(esr_ui QML_FILES qml/VistaPreviaCamara.qml)
  target_compile_definitions(esr_ui PRIVATE ESR_CON_MULTIMEDIA)
  target_link_libraries(esr_ui PRIVATE Qt6::Multimedia)
  message(STATUS "Easy Screen Recorder: con Qt6Multimedia; vista previa de camara y vumetro")
else()
  message(STATUS "Easy Screen Recorder: sin Qt6Multimedia; sin vista previa de camara ni vumetro")
endif()

# Traduccion al ingles.
#
# Las cadenas fuente estan en castellano —es la regla del repositorio,
# CLAUDE.md— asi que el catalogo traduce castellano a ingles, al reves de la
# costumbre de Qt. Funciona igual y evita reescribir las 45 cadenas del codigo.
#
# Se usa `lrelease` a pelo y NO `qt_add_translations()`, que seria lo idiomatico.
# El motivo es una dependencia: qt_add_translations viene del paquete de CMake
# Qt6LinguistTools, o sea de qt6-tools-dev, que no esta en esta maquina y es un
# paquete gordo. `lrelease` lo trae qt6-l10n-tools, que es lo minimo y ya
# estaba. Asi el proyecto compila con la traduccion en mas sitios, que es lo
# que importa en algo que se empaqueta para varias distribuciones.
#
# El .qm se EMPOTRA en el binario. Un fichero de traduccion suelto en el disco
# es algo que se pierde al empaquetar, y entonces la interfaz sale en castellano
# sin que nadie sepa por que.
find_program(ESR_LRELEASE
  NAMES lrelease lrelease-qt6
  HINTS /usr/lib/qt6/bin /usr/lib/x86_64-linux-gnu/qt6/bin
  PATH_SUFFIXES bin)

if(ESR_LRELEASE)
  set(ESR_QM "${CMAKE_CURRENT_BINARY_DIR}/easy-screen-recorder_en.qm")
  add_custom_command(
    OUTPUT "${ESR_QM}"
    COMMAND "${ESR_LRELEASE}" -silent
            "${CMAKE_CURRENT_SOURCE_DIR}/i18n/easy-screen-recorder_en.ts"
            -qm "${ESR_QM}"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/i18n/easy-screen-recorder_en.ts"
    COMMENT "Compilando la traduccion al ingles"
    VERBATIM)
  set_source_files_properties("${ESR_QM}" PROPERTIES
    QT_RESOURCE_ALIAS easy-screen-recorder_en.qm)
  qt_add_resources(esr_ui "traducciones"
    PREFIX "/i18n"
    BASE "${CMAKE_CURRENT_BINARY_DIR}"
    FILES "${ESR_QM}")
else()
  message(WARNING
    "Easy Screen Recorder: no se encuentra lrelease (paquete qt6-l10n-tools). "
    "La interfaz solo saldra en castellano.")
endif()

target_link_libraries(esr_ui PRIVATE
  esr::core
  Qt6::Core Qt6::DBus Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Widgets Qt6::Concurrent
)
# Los avisos duros del proyecto se aplican a NUESTRO codigo. El codigo que
# genera moc/qmltc no es nuestro y sus avisos no se arreglan aqui, asi que el
# objetivo de la UI lleva los flags directamente y no el INTERFACE global.
target_compile_options(esr_ui PRIVATE -Wall -Wextra)

include(GNUInstallDirs)
install(TARGETS esr_ui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES datos/es.solucionesconscientes.EasyScreenRecorder.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES datos/es.solucionesconscientes.EasyScreenRecorder.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
# El icono de color va a scalable/apps y los monocromos a symbolic/apps, que es
# donde los busca el tema de iconos cuando se pide un nombre con sufijo
# -symbolic. Mezclarlos deja la bandeja con el icono a color, que sobre un panel
# oscuro se ve como una pegatina.
install(FILES datos/es.solucionesconscientes.EasyScreenRecorder.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES datos/es.solucionesconscientes.EasyScreenRecorder-symbolic.svg
              datos/es.solucionesconscientes.EasyScreenRecorder-recording-symbolic.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/symbolic/apps)
