<!--
SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Hoja de ruta

Escrito el 15 de septiembre de 2026, ampliado el 16.

| Parte | Qué es |
|---|---|
| **1 · Frontera del backend** | Deuda técnica con nombre y apellidos |
| **2 · Pantalla y webcam juntas** | Funcionalidad pedida, con el diseño medido |
| **3 · Mensajes del núcleo en inglés** | Lo que quedó fuera de la traducción |
| **4 · Distribución** | Flathub, luego `.deb`, y **Windows descartado** con su motivo escrito para que no se reabra sin datos nuevos |

Las tres primeras comparten un prerrequisito y no es casualidad: **la matriz de
`verify-recording.sh`**. Las tres tocan los ficheros donde un error no rompe una
compilación, rompe una grabación.

---

# Parte 1 · La frontera del backend de captura

## Diagnóstico: el transporte está aislado, el vocabulario no

**La buena noticia.** La UI incluye exactamente cinco cabeceras de `libesr`:

```
esr/ajustes.hpp  esr/audio.hpp  esr/configuracion.hpp
esr/entorno.hpp  esr/grabacion.hpp
```

Y **no incluye** `esr/proceso.hpp`, `esr/ipc.hpp` ni `esr/json_ipc.hpp`.
Comprobado. O sea que el lanzamiento del proceso, el socket unix y el JSON del
protocolo están detrás de `libesr` y la UI no los ve. Eso es la mitad difícil y
ya está hecha.

**La mala.** El *vocabulario* de GSR sí se filtra hasta el QML. Siete fugas
localizadas:

| Fuga | Dónde | Qué significa |
|---|---|---|
| Identificadores de códec de GSR | `src/ui/qml/Main.qml:188` | El QML sabe que el sufijo `_software` significa CPU **porque GSR lo nombra así** |
| Índice del enum de calidad | `Main.qml:212` | `currentIndex: 2` es `very_high` porque es el tercer valor **de la lista de GSR** |
| Compatibilidad códec/formato | `Main.qml:195` | El filtrado replica qué respeta GSR en cada contenedor |
| Rareza de comportamiento | `Main.qml:318` | «GSR no pausa el modo audio-only»: una limitación suya, codificada en la UI |
| Formato de `-region` | `src/ui/qml/SelectorRegion.qml:88` | Las coordenadas se calculan en el formato de su bandera |
| Estado `"sinGsr"` | `src/ui/controlador.hpp:21`, `Main.qml:108` | El nombre del backend está en la máquina de estados de la UI |
| Campo `gsr` en la API pública | `esr/entorno.hpp:62-63` → `e.gsr.presente` | `libesr` expone dos herramientas llamadas `gsr` y `gsr_cli` |

**Veredicto: no está aislada.** Si mañana hubiera que cambiar de backend habría
que tocar QML, no solo `libesr`.

Con Windows descartado (ver Parte 4), esto **deja de ser urgente**: hoy hay un
solo backend y no se prevé un segundo. Pasa de deuda que bloquea a deuda que
conviene. Los dos motivos que la mantienen en la lista son que el plan de
pruebas que exige —la matriz de `verify-recording.sh`— es bueno por sí mismo, y
que si GSR cambiara de interfaz habría que tocar QML para adaptarse, lo cual es
absurdo.

**Y no se refactoriza ahora.** Es un cambio de API pública con los tests verdes
por delante, y el encargo de este renombrado era explícito: no cambiar
funcionalidad. Queda el plan.

## Plan: la interfaz `CaptureBackend`

### Ficheros afectados

| Fichero | Qué cambia |
|---|---|
| `src/core/include/esr/backend.hpp` | **Nuevo.** La interfaz |
| `src/core/backend_gsr.cpp` + `.hpp` | **Nuevo.** La implementación actual, movida |
| `src/core/include/esr/entorno.hpp` | `Herramienta gsr` / `gsr_cli` → un `Backend` con `nombre` y `disponible` |
| `src/core/entorno.cpp` | `localizar_gsr()` pasa a ser la detección del backend |
| `src/core/grabacion.cpp` | Deja de construir argumentos de GSR directamente |
| `src/core/capacidades.cpp` | El parser de `section=` pasa a `backend_gsr.cpp`; queda la traducción a tipos propios |
| `src/ui/controlador.cpp` | `e.gsr.presente` → `e.backend.disponible`; el estado `sinGsr` → `sinBackend` |
| `src/ui/controlador.hpp` | El contrato de estados |
| `src/ui/qml/Main.qml` | Los códecs y la calidad vienen de `libesr` como listas con etiqueta, no como identificadores del backend |
| `src/ui/qml/SelectorRegion.qml` | Devuelve una región en tipos propios; la traducción a `-region` baja a `backend_gsr.cpp` |
| `tests/prueba_capacidades.cpp`, `prueba_entorno.cpp`, `prueba_grabacion.cpp` | Se adaptan a los tipos nuevos |

### La interfaz

```cpp
// esr/backend.hpp — lo que la UI y el CLI pueden pedir de una captura, sin
// saber quien la hace.
namespace esr {

struct Codec {
    std::string id;        // identificador interno, opaco para la UI
    std::string etiqueta;  // lo que se ensena: "H.264 (GPU)"
    bool hardware;
};

struct Calidad {
    std::string id;
    std::string etiqueta;
};

struct Region { int ancho, alto, x, y; };

class CaptureBackend {
public:
    virtual ~CaptureBackend() = default;

    virtual std::string nombre() const = 0;
    virtual bool disponible(std::string* motivo) const = 0;

    virtual std::vector<Fuente>  fuentes()  const = 0;
    virtual std::vector<Codec>   codecs(const std::string& formato) const = 0;
    virtual std::vector<Calidad> calidades() const = 0;
    virtual Calidad              calidad_por_defecto() const = 0;

    virtual Resultado empezar(const Peticion&) = 0;
    virtual Resultado parar() = 0;
    virtual bool      admite_pausa(bool solo_audio) const = 0;
    virtual Resultado pausar(bool pausado) = 0;
};

}  // namespace esr
```

Las tres funciones que matan las fugas: `codecs(formato)` devuelve la lista ya
filtrada y **con etiqueta**, así que el QML no necesita saber qué es `_software`;
`calidad_por_defecto()` sustituye al `currentIndex: 2`; y `admite_pausa(bool)`
sustituye al comentario sobre lo que GSR no hace.

### Riesgo de regresión

**Alto**, y por dos motivos concretos:

1. **`capacidades.cpp` es el parser de la salida de GSR** y de él sale la lista
   de códecs y de fuentes de toda la aplicación. Tiene tests con fixtures reales
   (`tests/fixtures/`, `docs/gsr-capabilities.txt`), pero el riesgo no es que
   deje de parsear: es que la traducción a tipos propios **pierda un caso** —un
   códec que se filtra mal, una fuente que deja de aparecer— y eso no lo pilla un
   test que compara cadenas.
2. **`grabacion.cpp` construye la línea de comandos.** Un argumento mal movido no
   rompe el build ni los tests: rompe una grabación, y probablemente solo en la
   combinación que nadie prueba. Aquí la única red es
   `scripts/verify-recording.sh`, que graba de verdad, y hay que ampliarlo a más
   combinaciones **antes** de refactorizar, no después.

**Orden recomendado:** primero ampliar `verify-recording.sh` a una matriz
(monitor/región/portal × h264/hevc × con audio/sin audio × mkv/mp4), dejarla en
verde con el código actual, y solo entonces mover código. Si la red no está
antes, el refactor es a ciegas.

**Lo que no hay que hacer:** aprovechar el refactor para «mejorar» la UI. Un
cambio de API pública y un cambio de comportamiento a la vez no se pueden
bisecar.

---

# Parte 2 · Pantalla y webcam juntas

Pedido por el titular el 2026-09-16: grabar pantalla y webcam, **con el tamaño y
la esquina de la webcam elegibles**. Esto es el diseño, medido antes de escribir
código. No está implementado.

## Lo que ya funciona hoy y no hay que tocar

**Grabar solo la webcam.** Está en el desplegable como `/dev/video0` y graba;
verificado: 1280×720 h264. Lo único que le falta es cosmética, y está en la
lista de UX: se enseña con su nombre de dispositivo y aparece una vez por cada
modo que reporta GSR.

## La decisión de fondo: post-proceso, no plugin

GSR soporta superposición **en vivo** con su sistema de plugins: `-p plugin.so`,
con una función `draw()` que recibe el framebuffer y un contexto OpenGL. Su
manual lo llama «Plugin system for custom graphics overlay», así que
técnicamente es la vía prevista.

**Se descarta, y no por dificultad.** Un plugin se carga *dentro del proceso de
GSR*, y eso es obra combinada: GSR es GPL-3.0-**only**, así que ese plugin no se
podría licenciar comercialmente. Sería el primer componente fuera del modelo
dual, y el razonamiento entero de `docs/LICENSING.md` —que se apoya en que GSR
es un proceso externo sin enlazar— dejaría de ser cierto para él.

Si algún día un cliente paga la superposición en vivo, el cálculo es otro y es
suyo. Hasta entonces: dos procesos y composición después.

## Lo medido, que es lo que hace viable el diseño

### Dos grabaciones a la vez: funciona

Dos procesos de GSR simultáneos, cámara y pantalla, **cero errores** en los
logs: 769 paquetes de vídeo en la cámara (1280×720) y 540 en la pantalla
(1366×768), las dos codificando en la misma GPU sin contención.

### Composición: tres vías, y solo una sirve

Medido sobre 20 s de 1080p30 sintético, en el i5-6200U con HD Graphics 520:

| Vía | Tiempo | Factor sobre lo grabado |
|---|---|---|
| Todo en CPU (`libx264 -preset veryfast`) | 9,55 s | **0,48×** |
| Todo en GPU (`scale_vaapi` + `overlay_vaapi`) | **falla** | `overlay_vaapi` da «Function not implemented» en esta GPU, incluso en el caso mínimo |
| **Mezcla en CPU + codificación en GPU** | **3,37 s** | **0,17×** |

**La tercera es la que va**, y el motivo importa: la mezcla de dos fotogramas es
barata, lo caro es codificar. Dejando el `overlay` en CPU y el `h264_vaapi` en
GPU se gana casi el triple. Verificado visualmente: la webcam aparece a su
tamaño y en su esquina.

Que la primera vía cueste el 48 % del tiempo grabado **no es un detalle**: para
un tutorial de media hora son catorce minutos de CPU a tope, que es exactamente
lo que esta aplicación promete evitar. Si algún día la mezcla en GPU funciona en
otra máquina, se usa; el código debe poder elegir.

### La receta

```bash
ffmpeg -init_hw_device vaapi=va:/dev/dri/renderD128 -filter_hw_device va \
  -i pantalla.mkv -i camara.mkv \
  -filter_complex "[1:v]scale=384:-2[pip];\
                   [0:v][pip]overlay=W-w-32:H-h-32,format=nv12,hwupload[v]" \
  -map "[v]" -map 0:a? -c:v h264_vaapi -qp 23 compuesto.mkv
```

Dos detalles que cuestan una tarde si se descubren tarde:

- **`scale=384:-2` y no `-1`.** El `-2` fuerza altura par, que es lo que H.264
  necesita. Con `-1` sale una altura impar y el codificador la rechaza.
- **`format=nv12,hwupload` antes del codificador.** Sin eso, la mezcla se queda
  en memoria de CPU y `h264_vaapi` no la acepta.

Tamaño y esquina son parámetros: el `384` del `scale` y las cuatro expresiones
de `overlay` —`24:24`, `W-w-24:24`, `24:H-h-24`, `W-w-24:H-h-24`—.

## El cambio de modelo, que es la parte de diseño

Hoy hay **una** sesión de pantalla (`~/.cache/easy-screen-recorder/sesion`) y
**una** de audio (`sesion-audio`), con cerrojos separados. La webcam añade una
tercera.

**Y la clave para no complicarlo: no son dos grabaciones, es UNA con dos
fuentes.** Empiezan juntas, paran juntas y se pausan juntas, porque es lo que el
usuario pide. Con eso, el estado de la interfaz sigue siendo un solo valor y no
hay que rehacer la máquina de estados: `grabando` cubre las dos.

Lo que sí cambia:

| Qué | Cómo |
|---|---|
| Sesión de la cámara | `sesion-camara/`, con su socket IPC, su pid y su log. Mismo patrón que `sesion-audio`, que ya existe |
| Arranque | Los dos procesos, y **si el segundo falla, se para el primero**. Media grabación es peor que ninguna |
| Parada | Los dos por IPC, y esperar los dos ficheros antes de dar por terminado |
| Ajustes | `AjustesGrabacion` gana `camara` (dispositivo o vacío), `pip_ancho` y `pip_esquina` |
| Estado nuevo | `componiendo`, porque al parar ya no se acaba: empieza un ffmpeg que tarda |

### Los tres problemas que ya aparecieron en las pruebas

**1. La cámara se queda ocupada.** Lo topé tres veces:
`VIDIOC_S_FMT failed, error: Device or resource busy`. La causa está localizada:
**el envoltorio de `flatpak run` no propaga el SIGINT** al GSR de dentro, y un
proceso huérfano con `/dev/video0` abierto bloquea la siguiente grabación. El
usuario solo vería «busy».

No afecta a la pantalla, que se para por IPC. Para la cámara hay que pararla por
IPC igual, y además detectar el caso: si el dispositivo está ocupado, decir *qué*
lo tiene en vez de repetir el error del driver.

**2. La composición puede fallar, y los originales tienen que sobrevivir.** Si
el ffmpeg revienta, el usuario se queda con `pantalla.mkv` y `camara.mkv`, que
es una salida perfectamente válida. **Nunca borrar los originales antes de
confirmar que el compuesto existe y tiene duración.** Y decírselo: «no se pudo
componer, tienes los dos ficheros en …».

**3. Componer no es gratis.** 0,17× del tiempo grabado con la GPU codificando.
Para media hora de grabación son cinco minutos. Eso se avisa **antes** de
empezar, no después: la casilla de componer debería decir el coste aproximado.

## Las dos tandas

**Tanda A · Dos ficheros.** Pantalla y webcam a la vez, cada una a su fichero,
sin composición. Es la mitad del valor por una fracción del coste, y es lo que
quiere quien va a editar: dos pistas dan más libertad que un vídeo ya compuesto.
Aquí está todo el cambio de modelo.

**Tanda B · Composición opcional.** La casilla de «componer al terminar», con
tamaño y esquina. El patrón de post-proceso ya está pensado en
`docs/post-proceso.md`, de la Tanda 6.

**Orden con lo demás:** la Tanda A toca `grabacion.cpp`, que es donde la Parte 1
avisa de que un error no rompe una compilación sino una grabación. **Antes va la
matriz de `verify-recording.sh`.** Es la misma red que piden la Parte 1 y la
Parte 3, y sirve para las tres.

# Parte 3 · Traducir los mensajes del núcleo

La interfaz ya es bilingüe: 45 cadenas traducidas, con el idioma elegido según
el del sistema. Lo que sigue en castellano son **~47 mensajes de error de
`libesr`** —en `audio.cpp` (16), `grabacion.cpp` (14), `entorno.cpp` (7),
`ipc.cpp` (6) y `proceso.cpp` (4)— y la línea de comandos entera.

## Por qué no se hizo ya

`libesr` **no tiene Qt a propósito**: es la capa 1 y el CLI la usa sin interfaz
gráfica. Así que sus mensajes son `std::string` en castellano, y no hay `tr()`
que los cubra. Enlazar Qt en el núcleo para traducirlos sería romper la
separación que hace que el CLI funcione en una máquina sin Qt.

## La vía correcta

Devolver **códigos de error con sus parámetros** en vez de texto, y que la capa
que habla con el usuario —la UI o el CLI— los redacte:

```cpp
struct Fallo {
    enum class Clase { FormatoDesconocido, BitrateFueraDeRango, … };
    Clase clase;
    std::vector<std::string> datos;  // el formato pedido, la cifra, la ruta
};
```

## Riesgo, y por eso no es trivial

**Alto, y en el mismo sitio que la Parte 1:** los mensajes de `grabacion.cpp` y
`audio.cpp` salen de las funciones que construyen la línea de comandos del
grabador y validan los ajustes. Ahí un error no rompe la compilación ni los
tests: rompe una grabación, y probablemente en la combinación que nadie prueba.

**Se hace después de la matriz de `verify-recording.sh`**, no antes. Es la misma
red que pide la Parte 1, y sirve para las dos.

# Parte 4 · Distribución

## Fase 1 · Flathub

Es la siguiente. El detalle está en `empaquetado/flathub/NOTAS.md`; en corto:

- **Licencia:** resuelta, GPL-3.0-or-later.
- **Bloqueo real:** lanzar GSR desde dentro del sandbox por `flatpak-spawn --host`,
  con `FLATPAK_ID` como señal. No se escribe hasta poder ejecutarlo.
- **Pendiente además:** las tres capturas, el tag de la primera versión, el
  cambio de `type: dir` a `type: git`, la verificación de dominio en
  `https://solucionesconscientes.es/.well-known/org.flathub.VerifiedApps.txt`, y
  justificar el permiso `--talk-name=org.freedesktop.Flatpak` en el envío.
- **Sin verificar:** el runtime se subió de 6.9 (EOL) a 6.11 sin construir.

## Fase 2 · `.deb`

Detalle en `empaquetado/debian/NOTAS.md`.

- `copyright` en DEP-5: **hecho**.
- `gpu-screen-recorder` **no está en los repositorios** de Ubuntu 26.04
  (medido), así que va en `Recommends` y no en `Depends`, con la instalación
  manual documentada.
- Falta `control`, `rules`, `changelog` y la primera versión etiquetada.
- X11 verificado por XWayland; sesión X11 pura sin probar.

## Windows: descartado

**Decidido el 15 de septiembre de 2026.** No se porta. No es un «más adelante»:
es una decisión tomada con un motivo, y para reabrirla hace falta que cambie ese
motivo, no que pase el tiempo.

### El motivo: la propuesta de valor no sobrevive al puerto

En Linux esta aplicación existe porque **Wayland rompió la captura de pantalla y
la única herramienta buena es de línea de comandos**. Ese hueco es real y es
nuestro. Las tres patas que lo sostienen se caen las tres en Windows:

| | Linux/Wayland | Windows |
|---|---|---|
| ¿Trae el sistema un grabador? | No | **Dos**, los dos preinstalados: Xbox Game Bar y la Herramienta de Recortes |
| ¿Codificación en GPU al alcance? | Hay que buscarla | La traen NVIDIA, AMD e Intel en su propio software, gratis y con atajo |
| ¿Está roto el camino de captura? | Sí, y el ecosistema va detrás | No. Desktop Duplication API es estable desde Windows 8 |
| Open source maduro con GPU | GSR, y poco más | OBS Studio y ShareX, los dos establecidos |

En Windows el hueco lo tapan Microsoft y los tres fabricantes de GPU antes de que
lleguemos nosotros. Seríamos la décima opción de una categoría resuelta, en vez
de la mejor de un caso concreto.

### Y el coste no era pequeño

- **WASAPI en modo loopback** para el audio del sistema: código nativo de
  verdad, no un argumento de ffmpeg. En Linux eso sale de un monitor de
  PipeWire; en Windows hay que escribirlo.
- **ffmpeg con `ddagrab`** como proceso externo era la vía correcta, pero
  redistribuir sus binarios nos convierte en distribuidores de software GPL, con
  la obligación de ofrecer sus fuentes.
- **Qt y KF6 por KDE Craft**: mantenimiento continuo de una cadena de
  construcción entera.
- **Kirigami fuera de Plasma** se ve como lo que es, una interfaz de KDE
  prestada.
- **Firma de código** para no comer avisos de SmartScreen, con su coste anual.
- Y en la Microsoft Store ya hay un **«HD Easy Screen Recorder»**, así que
  además habría lío de nombre.

### Lo único que sí tenía hueco, y no es esta aplicación

Grabar **solo el audio del sistema** a un fichero sigue siendo incómodo en
Windows: o Audacity con WASAPI loopback, o OBS sin vídeo. Ese agujero es real.

Pero es una utilidad pequeña y distinta, no esta aplicación portada. Si alguna
vez apetece, se hace como producto aparte y se decide por sus propios méritos.

### Qué haría falta para reabrirlo

- Que un cliente lo pague. Entonces el cálculo es otro y es el suyo.
- O que Microsoft retire Game Bar y la grabación de la Herramienta de Recortes,
  que no va a pasar.
- O que la aplicación de Linux esté tan establecida que el nombre valga algo por
  sí mismo, y entonces la Store sea escaparate y no producto.

### Consecuencia inmediata: el nombre deja de ser urgente

Estaba en la lista de «comprobar ya» **porque Windows implicaba la Microsoft
Store**. Sin Store:

- **Flathub está libre**, comprobado. Es donde vamos.
- «HD Easy Screen Recorder» en la Microsoft Store no nos afecta.
- El nombre es descriptivo, así que es difícil que alguien tenga un derecho
  fuerte sobre él —y difícil que lo tengamos nosotros—.

Queda como conveniente, no como bloqueante: una consulta a la OEPM y a la EUIPO
antes de gastar en marca, si algún día se gasta. No antes de publicar en
Flathub.

## Fase 3 · (libre)

Sin asignar. Lo que salga del uso real de las fases 1 y 2 manda sobre cualquier
plan escrito hoy.
