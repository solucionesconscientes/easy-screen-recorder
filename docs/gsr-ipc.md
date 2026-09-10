# El protocolo IPC de gpu-screen-recorder

**Estado: DOCUMENTADO Y COMPROBADO.** Bloqueo B1 cerrado.

Versión leída: GSR **6.0.0**, el árbol de `third_party/gpu-screen-recorder/`
(`project.conf:4`). Es la misma que está instalada en la máquina de desarrollo,
así que las citas apuntan al código que de verdad se ejecuta aquí.

Todas las citas son `fichero:línea` dentro de `third_party/gpu-screen-recorder/`.
Al final hay una sección con lo que se ejecutó para comprobarlo y otra con lo
que no queda claro.

## Resumen para quien tenga prisa

- Transporte: **socket de dominio unix**, `SOCK_STREAM`, permisos `0600`.
- Formato: **una petición JSON por línea**, terminada en `\n`. Hay respuesta
  siempre, también en JSON de una línea.
- La ruta del socket **la elegimos nosotros**: es el argumento de `-ipc`. GSR no
  la deriva de nada.
- Sin `-ipc` no hay IPC. GSR ni siquiera crea el socket.
- **La ruta del fichero guardado llega en la respuesta a `stop`**, en el campo
  `data`. Es una respuesta diferida: no llega hasta que el fichero está escrito.

Eso último es lo que más falta le hacía a la UI y está resuelto sin trucos: no
hay que adivinar el nombre ni leer stdout.

## Transporte

Un socket de dominio unix orientado a conexión. Lo dice el comentario del tipo
en `include/cli/ipc.h:71` y lo confirma la creación en `src/cli/ipc.c:873`:

```c
self->socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
```

Detalles que importan al escribir el cliente:

| Qué | Valor | Dónde |
|---|---|---|
| Permisos del socket | `0600`, puestos con `umask` alrededor del `bind` | `src/cli/ipc.c:30`, `src/cli/ipc.c:790-793` |
| Clientes a la vez | 8 | `include/cli/ipc.h:10`, `listen()` en `src/cli/ipc.c:890` |
| Petición máxima | 4096 bytes | `include/cli/ipc.h:11` |
| Longitud máxima de la ruta | la de `sun_path`, 107 caracteres en Linux | `src/cli/ipc.c:848-851` |

El socket se atiende en un hilo aparte con `epoll` en Linux y `kqueue` fuera de
él (`src/cli/ipc.c:67-150`). Para nosotros es transparente, pero explica por qué
las respuestas diferidas pueden llegar mientras la grabación sigue.

### Socket viejo de un GSR que murió

Antes de hacer `bind`, GSR mira si en esa ruta ya hay algo
(`src/cli/ipc.c:767-784`):

- Si existe y **no** es un socket, se niega y no arranca.
- Si es un socket y alguien está escuchando, se niega: "another program is
  already listening".
- Si es un socket y nadie escucha, lo borra y sigue.

Al salir limpiamente lo borra él (`src/cli/ipc.c:828`). Comprobado: tras el
`stop` el fichero de socket ya no estaba.

## Dónde sale la ruta del socket

De ningún sitio automático. **Es el argumento de `-ipc` y nada más.**

- La opción está declarada como opcional en `src/args_parser.c:574`.
- Se lee en `src/cli/main.c:523` y solo se inicializa el IPC si trae valor:
  `src/cli/main.c:545`.

```c
if(ipc_arg->num_values > 0 && gsr_ipc_init(&ipc, ipc_arg->values[0]) != GSR_ERROR_OK) {
```

No depende del usuario, ni del PID, ni de `XDG_RUNTIME_DIR`. Se buscó
`XDG_RUNTIME_DIR` en todo `src/` e `include/` y no aparece. Los ejemplos del
manual usan `$XDG_RUNTIME_DIR/gsr.sock` (`gsr-cli.1`, sección EXAMPLES) pero eso
es una costumbre del ejemplo, no del programa.

**Para Capturia esto es la mejor noticia del documento:** elegimos la ruta,
así que sabemos siempre cuál es y podemos tener varias grabaciones a la vez sin
pisarnos. Ver más abajo la trampa del flatpak.

## Formato de mensaje

### Petición

Un objeto JSON por línea, terminado en `\n`. El servidor va acumulando bytes
hasta ver el salto de línea y ahí procesa (`src/cli/ipc.c:526-539`).

Campos (`src/cli/ipc.c:271-329`):

| Campo | Tipo | Obligatorio | Nota |
|---|---|---|---|
| `id` | entero | sí | vuelve tal cual en la respuesta |
| `name` | cadena | sí | el nombre del comando |
| `data` | lo que pida el comando | según el comando | ausente en la mayoría |

Si falta `id` la respuesta lleva `"id":0`: el parser pone la petición a cero
nada más entrar (`src/cli/ipc.c:272`) y ese cero es el que se devuelve
(`src/cli/ipc.c:505`). Si el objeto no
es JSON válido, o `name` no es cadena, contesta con error y el motivo.

### Respuesta

Siempre hay respuesta, y siempre es una línea (`src/cli/ipc.c:247-268`):

```
{"id":N,"result":"ok"}
{"id":N,"result":"ok","data":"<cadena>"}
{"id":N,"result":"error","data":"<motivo>"}
```

El `data` de un error es el texto del fallo, ya escapado. El de un `ok` solo
aparece en los comandos que guardan un fichero, y entonces **es su ruta**.

## Comandos

Los reconoce `ipc_handle_request` en `src/cli/ipc.c:455-491`. Cualquier otro
nombre da `unknown request name '<x>'`.

| Comando | `data` que acepta | Respuesta | Cita |
|---|---|---|---|
| `stop` | ninguno | **diferida**, con la ruta del fichero | `src/cli/ipc.c:456-457` |
| `toggle-pause` | ninguno | inmediata | `src/cli/ipc.c:459-460` |
| `set-paused` | `true` o `false` (booleano JSON suelto, no objeto) | inmediata | `src/cli/ipc.c:462-468`, `385-393` |
| `toggle-replay-recording` | ninguno | inmediata | `src/cli/ipc.c:470-471` |
| `start-replay-recording` | ninguno | inmediata | `src/cli/ipc.c:473-474` |
| `stop-replay-recording` | ninguno | **diferida**, con la ruta | `src/cli/ipc.c:476-477` |
| `save-replay` | objeto opcional con `seconds` y `restart-replay` | **diferida**, con la ruta | `src/cli/ipc.c:479-487`, `342-383` |

Dos cosas que no son comandos del protocolo:

- **`status` no existe en el servidor.** Es cosa del cliente: `gsr-cli` se
  conecta al socket y, si la conexión entra, imprime `running` y sale con 0
  (`tools/gsr-cli/main.c:235-247`). No se manda nada. Nosotros haremos lo mismo:
  un `connect()` que funciona es la señal.
- **No hay comando para arrancar.** El IPC controla un GSR que ya está grabando.
  Arrancar es lanzar el proceso con sus argumentos. Lo mismo vale para elegir
  fuente, códec o fichero: todo eso son argumentos de línea de comandos, no
  mensajes.

### `set-paused` frente a `toggle-pause`

`toggle-pause` falla si ya estaba en el estado contrario. `set-paused` no
(`gsr-cli.1`, sección COMMANDS). Para una UI con un botón de estado, `set-paused`
es la buena: el resultado no depende de lo que creamos que está pasando.

### `save-replay`

El `data` es un objeto con dos campos opcionales (`src/cli/ipc.c:342-383`):

- `seconds`: entero mayor que 0. Sin él se guarda el buffer entero.
- `restart-replay`: booleano. Pisa la opción `-restart-replay-on-save` solo para
  esta vez.

Requiere que GSR se haya lanzado con `-r`. Sin eso contesta
`option -r is required to save a replay`, comprobado.

## Cómo llega la ruta del fichero guardado

Es la parte que más importa, así que va entera.

Tres comandos no contestan cuando se ejecutan, sino cuando el fichero está
escrito: `stop`, `save-replay` y `stop-replay-recording`. El código los llama
**peticiones diferidas** (`include/cli/ipc.h:39-46`).

El recorrido completo:

1. Llega la petición. Antes de tocar nada se apunta como pendiente, con el
   descriptor del cliente y el `id` (`src/cli/ipc.c:434-447`). El comentario de
   `src/cli/ipc.c:507-508` explica por qué en ese orden: si se hiciera después,
   una grabación corta podría terminar antes de que hubiera a quién contestar.
2. El handler arranca la operación y devuelve. **No se contesta todavía**
   (`src/cli/ipc.c:520-521`).
3. Cuando el fichero está cerrado, el grabador llama a `gsr_ipc_complete_request`
   con la ruta (`src/cli/ipc.c:944-962`). Los tres sitios:
   - `src/cli/main.c:501` para `stop`, pasando `settings.filename`.
   - `src/cli/main.c:359` para `save-replay`, con la ruta del callback.
   - `src/cli/main.c:384` para `stop-replay-recording`.
4. Eso despierta al hilo del IPC, que manda la respuesta con la ruta en `data`
   (`src/cli/ipc.c:621-641`).

O sea: **se pide `stop` y se espera. La respuesta que llega ya trae la ruta y la
garantía de que el fichero está escrito.** No hay que sondear el directorio ni
adivinar el nombre.

Detalle de `src/cli/main.c:501`: en modo replay se pasa `NULL` en vez de la
ruta, porque un `stop` en replay no guarda nada. Ahí la respuesta es
`{"id":N,"result":"ok"}` sin `data`. La UI tiene que aguantar ese caso.

### Cuándo no llega

- Si el guardado falla, la respuesta es `result:"error"` con un motivo fijo por
  tipo (`src/cli/ipc.c:424-432`), por ejemplo `failed to save the recording`.
- Si el cliente se desconecta antes, la petición pendiente se tira
  (`src/cli/ipc.c:596-611`). **Hay que mantener la conexión abierta hasta
  recibir la respuesta.**
- Si GSR muere antes de terminar, las pendientes reciben
  `GPU Screen Recorder exited before the request finished`
  (`src/cli/ipc.c:643-660`).
- Solo cabe una pendiente de cada tipo. Un segundo `stop` mientras el primero
  sigue da `GPU Screen Recorder is already stopping` (`src/cli/ipc.c:414-422`).

### Sin tiempo límite

`gsr-cli` usa un tiempo de espera de 10 segundos para los comandos normales y
**ninguno** para los diferidos (`tools/gsr-cli/main.c:18-19`, y los
`GSR_CLI_NO_REPLY_TIMEOUT` de `tools/gsr-cli/main.c:323` y `358`). Tiene
sentido: guardar un replay largo puede tardar. Nuestra capa debe hacer lo mismo
y no imponer un límite corto, o cortará guardados legítimos.

## La alternativa: señales

GSR también se controla por señales (`src/cli/main.c:205-215`): `SIGINT` y
`SIGTERM` paran, `SIGUSR1` guarda el replay, `SIGUSR2` pausa, y `SIGRTMIN+N`
guarda tramos de duración fija.

**No las vamos a usar.** El manual lo dice y el código lo confirma: por IPC se
sabe si el comando funcionó y se recibe la ruta del fichero, y con señales no
(`gsr-cli.1:21-25`). Una señal se manda a ciegas.

## La trampa del flatpak

En esta máquina GSR está instalado como flatpak
(`com.dec05eba.gpu_screen_recorder`, 6.0.0). El sandbox tiene
`filesystems=host` y `shared=ipc`, así que ve el disco del usuario y los
sockets cruzan. Con una excepción que cuesta media tarde si no se sabe:

**El `/tmp` del flatpak es privado.** Comprobado: se creó un fichero en el
`/tmp` del sistema y desde dentro del sandbox no se ve; el socket y el vídeo que
GSR creó en `/tmp` existían solo dentro del sandbox.

Consecuencia para Capturia, cuando GSR venga en flatpak:

- El socket de `-ipc` **no puede ir en `/tmp`**. Ni el fichero de salida.
- Sirve cualquier ruta bajo el home del usuario. La prueba de este documento usó
  `~/.cache/`.
- Con GSR nativo en PATH da igual. Por eso `libcapturia` guarda de qué vía viene
  (`Invocacion::origen` en `src/core/include/capturia/entorno.hpp`) y `--check`
  lo enseña.

## Qué se ejecutó para comprobarlo

No es lectura sola. Se lanzó GSR 6.0.0 con `-ipc`, se le hablaron los comandos
por un socket a pelo y se miró qué contestaba. Literal:

```
--> {"id": 7, "name": "toggle-pause"}
<-- {"id":7,"result":"ok"}
--> {"id": 8, "name": "set-paused", "data": false}
<-- {"id":8,"result":"ok"}
--> {"id": 9, "name": "no-existe"}
<-- {"id":9,"result":"error","data":"unknown request name 'no-existe'"}
--> {"id": 10, "name": "save-replay"}
<-- {"id":10,"result":"error","data":"option -r is required to save a replay"}
--> {"nombre": "sin-id"}
<-- {"id":0,"result":"error","data":"the request is missing the 'id' field"}
--> {"id": 11, "name": "stop"}
<-- {"id":11,"result":"ok","data":"/home/pc/.cache/capturia-prueba/prueba.mkv"}
```

Y el fichero estaba: 4741 bytes, 2,907 s, un stream de vídeo h264 y uno de audio
opus según `ffprobe`. Los permisos del socket salieron `0600`, como dice el
código, y desapareció al salir GSR.

Fecha de la comprobación: 2026-09-10. Máquina: Ubuntu 26.04, KDE Plasma sobre
Wayland, GPU Intel, GSR 6.0.0 en flatpak.

## Lo que no queda claro

Honestamente, no mucho, pero conviene anotarlo:

- **Cuántas peticiones caben en una conexión.** El servidor procesa por línea y
  no cierra tras contestar, así que aparentemente varias. No se ha probado a
  encadenar dos por la misma conexión: la prueba abrió una conexión por
  comando, que es lo que hace `gsr-cli`.
- **Qué pasa si dos clientes piden `stop` a la vez.** El código apunta una sola
  pendiente por tipo y al segundo le contesta que ya se está parando, pero no se
  ha provocado la carrera de verdad.
- **Si `settings.filename` puede diferir del fichero final.** `src/cli/main.c:501`
  devuelve el nombre pedido por `-o`, no uno consultado al muxer. Con `-o` a un
  fichero concreto coinciden, y así salió en la prueba. Con `-ro` a un
  directorio el nombre lo pone GSR, y ese camino no se ha probado.
- **Versión mínima real.** Aquí solo se ha leído 6.0.0. La página de manual
  `gsr-cli.1:1` lleva sellado `5.15.3`, así que `gsr-cli` ya existía antes, pero
  sin ese árbol delante no se puede afirmar que su IPC estuviera completo. Por
  eso la mínima se fija en 6.0.0 y no en 5.15.3. Ver CLAUDE.md.
