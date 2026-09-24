# Licencias: nuestra GPL-3.0-or-later y por qué la de GSR no nos alcanza

Este documento explica el razonamiento y **dónde está el riesgo**. No es para
abrir la puerta a enlazar el código de GSR: es para que quien venga después sepa
qué línea no se cruza y por qué.

Aviso: esto es ingeniería, no asesoría legal. Si algún día Easy Screen Recorder se
distribuye en serio, esto se lo mira un abogado. Lo que hay aquí sirve para
tomar decisiones técnicas con los ojos abiertos.

## Los hechos

**gpu-screen-recorder es GPL-3.0-only.** No "or later":

- `el LICENSE de GSR`: texto íntegro de la GNU GPL versión
  3, 29 de junio de 2007.
- `el README de GSR, línea 246`: "This software is licensed
  under GPL-3.0-only".

**Easy Screen Recorder todavía no tiene licencia propia.** No hay fichero `LICENSE` en la
raíz del repo. Es una decisión pendiente y no es menor: ver el último apartado.

## Cómo usamos GSR, exactamente

Esto es lo que decide todo, así que conviene ser literal. Easy Screen Recorder:

- **Lanza `gpu-screen-recorder` como proceso aparte**, con `fork` y `execv`
  (`src/core/proceso.cpp`).
- **Le pasa opciones por línea de comandos**: la fuente, el códec, el fichero.
- **Le habla por un socket de dominio unix**, con mensajes JSON de una línea, el
  mismo protocolo que usa `gsr-cli` (ver `docs/gsr-ipc.md`).
- **Lee su salida estándar y su código de salida.**

Y lo que **no** hace, que importa igual o más:

- No incluye ni una cabecera de GSR. `libesr` no tiene un solo `#include`
  que apunte a la copia de referencia.
- No enlaza nada suyo, ni estática ni dinámicamente. `src/core/CMakeLists.txt`
  no menciona GSR.
- No copia código suyo. Ni fragmentos, ni algoritmos traducidos.
- No compila su árbol. la copia de referencia de GSR es solo lectura y está
  fuera del build.
- No lo redistribuye. Lo instala el usuario, por su cuenta, con su gestor de
  paquetes o su flatpak.

la copia de referencia de GSR ni siquiera va al remoto: lo excluye
`.gitignore`. Está en la máquina de desarrollo para poder leerlo y citarlo, que
es justo lo que se ha hecho en `docs/gsr-ipc.md`.

## El razonamiento

La GPL-3.0 obliga a licenciar bajo GPL "la obra basada en el Programa": lo que
en términos de copyright sería una obra derivada. La pregunta es si nuestra UI
lo es.

Dos programas que se comunican **a distancia** (procesos separados, tuberías,
sockets, argumentos de línea de comandos) se consideran normalmente obras
separadas, aunque se usen juntos. Es la posición que la propia FSF sostiene en
su FAQ de la GPL, y es exactamente el caso de Easy Screen Recorder: procesos distintos,
espacios de memoria distintos, un protocolo de texto en medio.

El argumento de más peso no es teórico, es que **ya existe el precedente dentro
del propio GSR**: `gsr-cli` es un binario aparte que controla al grabador por
ese mismo socket. El upstream diseñó esa frontera a propósito, para que un
programa externo pudiera mandarle órdenes. Nosotros ocupamos el sitio de
`gsr-cli`, no el de una parte de GSR.

Lo que hace fuerte nuestra posición, en orden de importancia:

1. **Ningún enlace.** Sin código de GSR en nuestro binario, el argumento de obra
   derivada se queda casi sin base.
2. **Protocolo público y documentado.** JSON por un socket, con su página de
   manual. No es una interfaz interna que hayamos destripado.
3. **Easy Screen Recorder funciona sin GSR.** Arranca, hace `--check` y explica qué falta.
   El modo audio-only, cuando exista, irá por ffmpeg y no tocará GSR
   (`docs/gsr-audio-only.md`).
4. **No lo distribuimos.** Nada de GSR viaja en nuestro paquete.

## Qué cambiaría si enlazáramos su código

Esta es la parte que hay que tener presente. Si algún día alguien decide que
sale más a cuenta llamar a una función de GSR que hablarle por el socket, esto
es lo que pasa:

| Si hiciéramos... | Consecuencia |
|---|---|
| Incluir sus cabeceras y llamar a sus funciones | La UI pasa a ser obra derivada. **Todo Easy Screen Recorder tendría que ser GPL-3.0** |
| Enlazar una biblioteca suya, estática o dinámica | Igual. La FSF considera el enlace dinámico tan derivado como el estático |
| Copiar código suyo, aunque sean veinte líneas traducidas | Igual, y además con un problema de atribución |
| Meter su código en nuestro árbol de compilación | Igual, aunque no se llame a nada: se estaría distribuyendo una obra combinada |

"Todo Easy Screen Recorder tendría que ser GPL-3.0" quiere decir: código fuente disponible
para cualquiera que reciba el binario, licencia compatible en toda dependencia
nuestra, y la puerta cerrada a cualquier plan futuro que no sea GPL. Puede que
eso acabe pareciendo bien; lo que no puede es pasar **sin decidirlo**, por
comodidad, en un `#include` de un martes.

Por eso `CLAUDE.md` lo prohíbe y por eso la copia de referencia es solo lectura.

## Las zonas grises

Ser honestos con el riesgo es el objetivo de este documento, así que:

- **La frontera no es absoluta.** Lo que cuenta no es solo "¿hay enlace?", sino
  cuán íntima es la comunicación y si los dos programas forman en la práctica
  uno solo. Un socket con un protocolo documentado está muy lejos de esa línea.
  Un protocolo privado, con estructuras binarias compartidas y estados
  entrelazados, estaría más cerca.
- **Es interpretación, no jurisprudencia.** La postura de la FSF es la del autor
  de la licencia, no la de un tribunal. Hay abogados que la discuten. Para un
  proyecto de escritorio con esta separación el riesgo es bajo, pero no es cero.
- **Empaquetar sí es distribuir.** Si algún día Easy Screen Recorder saliera como flatpak
  con GSR dentro, estaríamos distribuyendo una obra GPL-3.0 y habría que cumplir
  la GPL **para esa copia**: oferta de fuentes, licencia incluida, avisos. Eso
  no relicencia nuestro código (sigue siendo agregación), pero sí añade
  obligaciones que hoy no tenemos. Recomendación: **depender de GSR instalado,
  no empaquetarlo.** Es además lo que ya hace `libesr`, que lo busca en
  PATH y en el flatpak del sistema.
- **`gsr-cli` es un binario GPL.** Si algún día llamáramos a `gsr-cli` en vez de
  hablar el protocolo nosotros, seguiría siendo ejecutar un programa aparte, que
  no contagia nada. Ejecutar nunca ha sido el problema; enlazar sí.

## Reglas para este proyecto

Lo accionable, en cuatro líneas:

1. GSR se usa **solo** como proceso externo, por línea de comandos y por el
   socket IPC. Nada más.
2. la copia de referencia de GSR es de lectura. No se compila, no se copia,
   no se modifica.
3. Ni un `#include` que apunte ahí desde `src/`.
4. Antes de empaquetar GSR con Easy Screen Recorder, se vuelve a leer este documento.

## Decisión

Tomada el 15 de septiembre de 2026 por el titular, **Dalmau Romaní (Soluciones
Conscientes)**.

**Easy Screen Recorder es GPL-3.0-or-later.** Todo: la aplicación, el CLI y
`libesr`. El texto íntegro está en `LICENSE`, descargado de gnu.org y sin
modificar. Cada fichero lleva su cabecera SPDX y lo que no admite comentario
está cubierto por `REUSE.toml`; `reuse lint` pasa sin errores.

### Modelo dual

El titular se reserva la posibilidad de **conceder licencias comerciales** del
mismo código a clientes que necesiten una versión cerrada. Eso no es una
excepción a la GPL: es el derecho de quien tiene el copyright a licenciar su
obra tantas veces como quiera y en los términos que quiera. La versión pública
es y seguirá siendo GPL-3.0-or-later.

Para que esa opción siga viva hay que cumplir cuatro cosas, y las cuatro son
verificables:

1. **Todo el código es del titular o está cubierto por un CLA.** Ninguna
   contribución externa entra sin firmarlo. Ver `CONTRIBUTING.md` y
   `docs/CLA.md`.
2. **GSR solo como proceso externo.** Se invoca por argumentos y se le habla por
   un socket unix. No se enlaza, no se incluye, no se copia y no se
   redistribuye. Es lo que sostiene todo el razonamiento de las secciones
   anteriores.
3. **Qt y KF6 con enlace dinámico, bajo LGPL.** Una versión cerrada puede usar
   Qt LGPL siempre que enlace dinámico, incluya los avisos de copyright y de
   licencia de Qt, y **permita al usuario sustituir las bibliotecas** por otra
   versión compatible. Si eso no se puede garantizar en el empaquetado de un
   cliente concreto, ese cliente necesita licencia comercial de Qt, y es su
   coste, no el nuestro.
4. **Nada de copiar código de terceros**, ni de GSR ni de ningún otro proyecto,
   ni siquiera traducido a otro lenguaje.

### Que GSR sea "only" da igual mientras sea proceso externo

`gpu-screen-recorder` es **GPL-3.0-only**, sin la cláusula "or later". Eso sería
un problema de compatibilidad si lo enlazáramos: una obra combinada tendría que
distribuirse bajo GPL-3.0-only exactamente, y nuestro "or later" no podría
aplicarse.

Pero no hay obra combinada. Son dos programas que se hablan por una interfaz, y
esa es la frontera que la propia FSF usa para separar obras. Nuestra licencia no
depende de la suya, y la suya no nos alcanza.

### Si algún día se empaqueta GSR con la aplicación

Cambia el cuadro y hay que hacer dos cosas, las dos obligatorias:

- **Incluir su licencia** con la copia distribuida.
- **Ofrecer el código fuente exacto de esa copia**, con sus parches si los hay,
  por el mismo medio o con una oferta escrita válida durante tres años.

Y una tercera que no es obligación legal pero sí de ingeniería: dejar escrito
qué versión exacta se empaquetó, porque "el código fuente correspondiente" es el
de esa versión y no el de la rama principal de hoy.

Nada de esto convierte nuestro código en GPL-3.0-only: seguirían siendo dos
programas, uno distribuido junto al otro. Lo que cambia es que pasamos a ser
distribuidores de GSR y asumimos sus obligaciones como tal.

### GSR se consulta fuera del repositorio

La copia de referencia vive en `~/Documentos/PROJECTES/app-audio/referencia/gpu-screen-recorder/`,
**fuera del repositorio**, y es de solo lectura. No está versionada, no lo ha
estado nunca y `third_party/` sigue en `.gitignore` como red. Se lee para
entender el protocolo y se citan `fichero:línea` como referencia; no se copia
su código ni se pega en la documentación.

### Resultado de la auditoría

Auditoría del 15 de septiembre de 2026, comparando `json_ipc.cpp`, `ipc.cpp`,
`proceso.cpp`, `capacidades.cpp`, `audio.cpp` y `grabacion.cpp` contra
`src/cli/ipc.c`, `src/cli/commands.c`, `src/json.c`, `include/cli/ipc.h`,
`tools/gsr-cli/main.c` y `src/args_parser.c` de GSR 6.0.0.

| Fichero | Veredicto |
|---|---|
| `src/core/json_ipc.cpp` | **Independiente** |
| `src/core/ipc.cpp` | **Independiente** |
| `src/core/proceso.cpp` | **Independiente** |
| `src/core/capacidades.cpp` | **Independiente** |
| `src/core/audio.cpp` | **Independiente** |
| `src/core/grabacion.cpp` | **Independiente** |

Lo que se midió, no lo que parece:

- **Cero líneas idénticas** normalizadas (sin comentarios ni espacios) entre los
  dos árboles.
- La **única cadena literal de más de seis caracteres** que comparten es
  `gpu-screen-recorder`, que es el nombre del programa que hay que invocar.
- Los identificadores comunes son palabras clave de C++, nombres de la API POSIX
  (`AF_UNIX`, `sockaddr_un`, `waitpid`, `pollfd`) y el vocabulario del protocolo
  —`section=`, `video_codecs`, `capture_options`, `supports_app_audio`—, que son
  **literales que GSR imprime por su salida estándar**: están en
  `docs/gsr-capabilities.txt`, que es esa salida capturada. Leer la interfaz de
  un programa no es copiar su código.
- El caso más claro es el IPC: el `include/cli/ipc.h` de GSR es el **servidor**
  —ocho clientes, un hilo, tabla de callbacks, peticiones diferidas con mutex— y
  nuestro `ipc.hpp` es el **cliente**: una clase RAII con un descriptor y un
  contador de peticiones. Son los dos lados del mismo protocolo, no una
  traducción.

**Lo que sí hubo que corregir, y se corrigió:** los documentos. `gsr-ipc.md`
citaba dos líneas de C literales y `gsr-audio-only.md` siete, incluido un bloque
contiguo de cinco con el bucle de validación de argumentos. Para un uso normal
serían citas cortas de análisis y probablemente amparadas, pero el modelo dual
exige que cada byte del repositorio sea del titular o esté cubierto por un CLA,
y GPL-3.0-only no se puede relicenciar. Las nueve líneas se reescribieron como
prosa. Los hechos y las citas `fichero:línea` se quedaron.

### Autoría del historial

**Los 24 commits de este repositorio tienen como autor al titular**, Dalmau
Romaní (Soluciones Conscientes). Comprobable:

```
$ git log --all --format='%an <%ae>' | sort | uniq -c
     24 solucionesconscientes <contacto@solucionesconscientes.es>
```

No siempre fue así. Un commit —«Tanda 1», el segundo del proyecto— quedó con
`Claude <noreply@anthropic.com>` en el campo `Author`. **El 15 de septiembre de
2026 se reescribió el historial** con `git filter-repo --mailmap` y se
reemplazaron las dos ramas en el remoto.

Se hizo entonces porque entonces se podía y después no: el repositorio era
**privado, con cero forks, cero clones y cero colaboradores**, así que el force
push no le rompía el historial a nadie. En cuanto un repositorio es público y
alguien lo clona, esa ventana se cierra y la única salida es convivir con el
problema.

**Por qué molestarse, si era solo metadato.** Lo era: el campo `Author` de git no
transfiere ni crea derechos, un modelo de lenguaje no es persona física, no puede
ser autor en el sentido del TRLPI, no puede ceder derechos y no puede firmar un
CLA. Lo que produce una herramienta bajo la dirección del titular es del titular,
con independencia de lo que diga el campo.

Pero este proyecto se va a enseñar como escaparate y su código se va a ofrecer
bajo licencia comercial. Lo primero que hace la revisión de un comprador es
`git shortlog -sne`, y ahí aparecía un tercero que no puede firmar nada. No era
un problema legal: era un problema que había que explicar cada vez.

**Lo que sí se conservó a propósito: los 23 `Co-Authored-By:` de los mensajes.**
No se borran y no se van a borrar. El trabajo se hizo con una herramienta y eso
es el registro honesto de cómo se hizo; lo que no puede figurar es como autoría,
porque la autoría tiene consecuencias que una herramienta no puede asumir.

**Norma de aquí en adelante:** el autor de los commits es el titular y la
herramienta va en `Co-Authored-By:`. Ya no hay `.mailmap`, porque no queda nada
que mapear.
