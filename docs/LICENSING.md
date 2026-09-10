# Licencias: por qué GSR no nos arrastra a la GPL-3.0

Este documento explica el razonamiento y **dónde está el riesgo**. No es para
abrir la puerta a enlazar el código de GSR: es para que quien venga después sepa
qué línea no se cruza y por qué.

Aviso: esto es ingeniería, no asesoría legal. Si algún día Capturia se
distribuye en serio, esto se lo mira un abogado. Lo que hay aquí sirve para
tomar decisiones técnicas con los ojos abiertos.

## Los hechos

**gpu-screen-recorder es GPL-3.0-only.** No "or later":

- `third_party/gpu-screen-recorder/LICENSE`: texto íntegro de la GNU GPL versión
  3, 29 de junio de 2007.
- `third_party/gpu-screen-recorder/README.md:246`: "This software is licensed
  under GPL-3.0-only".

**Capturia todavía no tiene licencia propia.** No hay fichero `LICENSE` en la
raíz del repo. Es una decisión pendiente y no es menor: ver el último apartado.

## Cómo usamos GSR, exactamente

Esto es lo que decide todo, así que conviene ser literal. Capturia:

- **Lanza `gpu-screen-recorder` como proceso aparte**, con `fork` y `execv`
  (`src/core/proceso.cpp`).
- **Le pasa opciones por línea de comandos**: la fuente, el códec, el fichero.
- **Le habla por un socket de dominio unix**, con mensajes JSON de una línea, el
  mismo protocolo que usa `gsr-cli` (ver `docs/gsr-ipc.md`).
- **Lee su salida estándar y su código de salida.**

Y lo que **no** hace, que importa igual o más:

- No incluye ni una cabecera de GSR. `libcapturia` no tiene un solo `#include`
  que apunte a `third_party/`.
- No enlaza nada suyo, ni estática ni dinámicamente. `src/core/CMakeLists.txt`
  no menciona GSR.
- No copia código suyo. Ni fragmentos, ni algoritmos traducidos.
- No compila su árbol. `third_party/gpu-screen-recorder/` es solo lectura y está
  fuera del build.
- No lo redistribuye. Lo instala el usuario, por su cuenta, con su gestor de
  paquetes o su flatpak.

`third_party/gpu-screen-recorder/` ni siquiera va al remoto: lo excluye
`.gitignore`. Está en la máquina de desarrollo para poder leerlo y citarlo, que
es justo lo que se ha hecho en `docs/gsr-ipc.md`.

## El razonamiento

La GPL-3.0 obliga a licenciar bajo GPL "la obra basada en el Programa": lo que
en términos de copyright sería una obra derivada. La pregunta es si nuestra UI
lo es.

Dos programas que se comunican **a distancia** (procesos separados, tuberías,
sockets, argumentos de línea de comandos) se consideran normalmente obras
separadas, aunque se usen juntos. Es la posición que la propia FSF sostiene en
su FAQ de la GPL, y es exactamente el caso de Capturia: procesos distintos,
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
3. **Capturia funciona sin GSR.** Arranca, hace `--check` y explica qué falta.
   El modo audio-only, cuando exista, irá por ffmpeg y no tocará GSR
   (`docs/gsr-audio-only.md`).
4. **No lo distribuimos.** Nada de GSR viaja en nuestro paquete.

## Qué cambiaría si enlazáramos su código

Esta es la parte que hay que tener presente. Si algún día alguien decide que
sale más a cuenta llamar a una función de GSR que hablarle por el socket, esto
es lo que pasa:

| Si hiciéramos... | Consecuencia |
|---|---|
| Incluir sus cabeceras y llamar a sus funciones | La UI pasa a ser obra derivada. **Todo Capturia tendría que ser GPL-3.0** |
| Enlazar una biblioteca suya, estática o dinámica | Igual. La FSF considera el enlace dinámico tan derivado como el estático |
| Copiar código suyo, aunque sean veinte líneas traducidas | Igual, y además con un problema de atribución |
| Meter su código en nuestro árbol de compilación | Igual, aunque no se llame a nada: se estaría distribuyendo una obra combinada |

"Todo Capturia tendría que ser GPL-3.0" quiere decir: código fuente disponible
para cualquiera que reciba el binario, licencia compatible en toda dependencia
nuestra, y la puerta cerrada a cualquier plan futuro que no sea GPL. Puede que
eso acabe pareciendo bien; lo que no puede es pasar **sin decidirlo**, por
comodidad, en un `#include` de un martes.

Por eso `CLAUDE.md` lo prohíbe y por eso `third_party/` es solo lectura.

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
- **Empaquetar sí es distribuir.** Si algún día Capturia saliera como flatpak
  con GSR dentro, estaríamos distribuyendo una obra GPL-3.0 y habría que cumplir
  la GPL **para esa copia**: oferta de fuentes, licencia incluida, avisos. Eso
  no relicencia nuestro código (sigue siendo agregación), pero sí añade
  obligaciones que hoy no tenemos. Recomendación: **depender de GSR instalado,
  no empaquetarlo.** Es además lo que ya hace `libcapturia`, que lo busca en
  PATH y en el flatpak del sistema.
- **`gsr-cli` es un binario GPL.** Si algún día llamáramos a `gsr-cli` en vez de
  hablar el protocolo nosotros, seguiría siendo ejecutar un programa aparte, que
  no contagia nada. Ejecutar nunca ha sido el problema; enlazar sí.

## Reglas para este proyecto

Lo accionable, en cuatro líneas:

1. GSR se usa **solo** como proceso externo, por línea de comandos y por el
   socket IPC. Nada más.
2. `third_party/gpu-screen-recorder/` es de lectura. No se compila, no se copia,
   no se modifica.
3. Ni un `#include` que apunte ahí desde `src/`.
4. Antes de empaquetar GSR con Capturia, se vuelve a leer este documento.

## Pendiente: la licencia de Capturia

Sigue sin elegirse y conviene hacerlo pronto, porque condiciona lo de arriba.
El razonamiento entero de este documento sirve para **mantener abierta la
opción** de que Capturia no sea GPL. Si al final se elige GPL-3.0 igualmente,
gran parte del cuidado deja de hacer falta, aunque las reglas seguirían siendo
buena ingeniería: no enlazar la captura es lo que nos deja cambiar de backend
sin reescribir la UI.

Es decisión del titular. Hasta que la tome, el proyecto se comporta como si la
respuesta fuera "no GPL", que es la opción que no cierra puertas.
