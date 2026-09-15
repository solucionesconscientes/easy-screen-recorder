<!--
SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)

SPDX-License-Identifier: GPL-3.0-or-later
-->

# Hoja de ruta

Escrito el 15 de septiembre de 2026. Dos partes: la frontera del backend de
captura, que es deuda técnica con nombre y apellidos, y las tres fases de
distribución.

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

**Veredicto: no está aislada.** Si mañana hubiera que cambiar de backend —y la
fase 3 dice que en Windows habrá que hacerlo— habría que tocar QML, no solo
`libesr`.

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

# Parte 2 · Distribución

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

## Fase 3 · Windows

**Hoy no hay ni una línea de código de Windows, y así se queda** hasta que las
fases 1 y 2 estén cerradas.

### Interfaz

Portable con **KDE Craft**, que compila Qt y KF6 para Windows. La UI es
Qt/QML/Kirigami y no toca el display directamente, así que es la parte que menos
duele.

### El backend de captura, que es el problema

GSR es Linux: usa KMS/DRM, VAAPI y NVFBC. En Windows hay dos caminos:

| Vía | Qué implica |
|---|---|
| **ffmpeg como proceso externo** con `ddagrab` (Desktop Duplication) más NVENC, AMF o QSV | Mismo patrón que hoy: proceso aparte, argumentos, sin enlazar. Es lo que encaja con la arquitectura y con la licencia |
| **Portar GSR a Windows** | Solo si el proyecto upstream madura en esa dirección. No depende de nosotros |

La primera es la apuesta, y **es exactamente lo que la Parte 1 desbloquea**:
sin la interfaz `CaptureBackend`, esto significa reescribir QML.

### El audio del sistema es un problema aparte

En Linux el audio del escritorio sale de un monitor de PipeWire. En Windows eso
**no existe igual**: hace falta **WASAPI en modo loopback**, y eso es código
nativo, no un argumento de ffmpeg. Es trabajo propio y hay que presupuestarlo
como tal, no como «lo mismo pero en Windows».

### Licencia al redistribuir en Windows

Si el instalador lleva binarios de **ffmpeg** o de **GSR**, pasamos a ser
distribuidores de software GPL y **hay que ofrecer sus fuentes** —con sus parches
si los hay, por el mismo medio o con oferta escrita válida tres años—. Eso **no
afecta a nuestra licencia**: seguirían siendo programas separados distribuidos
juntos. Ver `docs/LICENSING.md`, «Si algún día se empaqueta GSR».

Y Qt: en Windows, con LGPL, sigue haciendo falta enlace dinámico, los avisos y
que el usuario pueda sustituir las bibliotecas. Un instalador monolítico con Qt
estático necesita licencia comercial de Qt.

### El nombre puede estar cogido

**Hay que comprobarlo antes de invertir en la Microsoft Store.** «Easy Screen
Recorder» es un nombre descriptivo y por eso mismo es probable que esté usado:
consta al menos una aplicación homónima en el ecosistema de Apple. Que sea
descriptivo juega a dos bandas — es difícil que alguien tenga un derecho fuerte
sobre él, y también es difícil que nosotros lo tengamos —.

Lo que hay que hacer, y en este orden:

1. Buscar el nombre en la Microsoft Store y en la App Store.
2. Consultar la OEPM y la EUIPO por si hay marca registrada.
3. Decidir: o se convive con el homónimo, o se registra, o se añade un
   distintivo para la Store.

**No es una tarea de la fase 3, es de antes:** si el nombre tuviera que cambiar,
cuanto antes se sepa, menos cuesta. Hoy ya está en el app-id, el `.desktop`, el
metainfo, el nombre de los binarios y toda la marca.
