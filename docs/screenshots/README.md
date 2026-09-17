# Capturas: qué falta

**Flathub las exige:** «All graphical applications must have one or more
screenshots in the MetaInfo». No es una recomendación, así que sin ellas no hay
envío. La primera, la `type="default"`, es la que sale en la ficha.

El metainfo las referencia por URL cruda del repositorio y **apuntando al tag
`v0.1.0`, no a `main`**. Eso también lo pide Flathub: «the link should be from a
tag or a commit and not a branch». Una rama se mueve, y la ficha publicada
enseñaría capturas de otra versión.

**Consecuencia en el orden de los pasos, y es la parte que se olvida:** las
capturas tienen que estar DENTRO del commit etiquetado. Así que:

1. Tomar las capturas y commitearlas.
2. **Mover el tag** a ese commit: `git tag -f -a v0.1.0` y `git push --force origin v0.1.0`.
3. Poner en el manifiesto de Flathub el commit nuevo del tag.

Si se etiqueta antes de las capturas, las URLs dan 404 para siempre en esa
versión.

| Fichero | Qué tiene que enseñar | Estado |
|---|---|---|
| `principal.png` | La ventana principal en reposo: fuente y botón de grabar. Es la `type="default"` | **Hecha** (2026-09-17, v0.5.0) |
| `grabando.png` | Grabando: el tiempo y los botones de pausa y parada | **Hecha** (2026-09-17) |
| `avanzado.png` | El panel Avanzado abierto: formato, códecs, calidad y carpeta | **Hecha** (2026-09-17) |
| `demo.gif` | El GIF del README: abrir, elegir región, grabar, parar. 10–15 s | **Falta** |

## Cómo se rehacen ahora: `scripts/capturas-ficha.sh`

Un comando, sin un solo clic:

```bash
scripts/capturas-ficha.sh
```

Toma las tres, en inglés, sin maximizar, las compone a 1100×790 en PNG sin
transparencia y las deja en esta carpeta.

**La clave que lo hizo posible es KWin.** Lo que bloqueaba esto no era pereza:
en Wayland la aplicación **no puede ponerse delante sola**, porque
`requestActivate()` necesita un token de activación que solo concede un clic de
verdad. Pero la interfaz de scripting de KWin sí puede activar una ventana y
quitarle el maximizado:

```javascript
const lista = workspace.windowList();
for (let i = 0; i < lista.length; ++i) {
    const w = lista[i];
    if (String(w.resourceName) !== "easy-screen-recorder") continue;
    if (w.minimized) w.minimized = false;
    w.setMaximize(false, false);
    workspace.activeWindow = w;
}
```

Se carga con `qdbus6 org.kde.KWin /Scripting loadScript <fichero> <nombre>` y
luego `start`.

**Un detalle que cuesta un rato:** el identificador es `resourceName` y **no**
`resourceClass`. La clase es `es.solucionesconscientes.EasyScreenRecorder`, así
que buscar ahí «easy-screen-recorder» no casa por las mayúsculas, el script no
encuentra nada y no dice por qué. `resourceName` sí es `easy-screen-recorder`.

Y la de «grabando» se toma arrancando la grabación **antes** de abrir la
ventana: así se abre ya en ese estado y no se minimiza, porque minimizarse es su
reacción a que el estado *cambie*.

## Historia: por qué esto costó tanto

Lo que sigue se conserva porque el problema tardó cuatro tandas en resolverse y
las trampas siguen siendo reales.

### Pendiente desde la 0.2.0 (resuelto en la 0.5.0)

Las tres siguen siendo las de la v0.1.0 y **ya no se parecen a la aplicación**:
enseñan «eDP-1» en vez de «Laptop screen · 1366×768», la ventana estirada y el
panel Avanzado cortado por abajo. Además son RGBA y de dos tamaños distintos
(1000×502 y 1000×515), y Flathub pide PNG sin transparencia y todas iguales.

No se rehicieron en esa tanda por un motivo concreto, y conviene saberlo antes
de intentarlo: **en Wayland la aplicación no puede ponerse ella sola delante.**
`requestActivate()` necesita un token de activación que solo concede un clic de
verdad, así que lanzarla desde una terminal la deja detrás de la ventana que
tuviera el foco. Hace falta un clic humano en la ventana justo antes de
disparar la captura.

Se intentó esquivarlo haciendo que la propia ventana se dibujara a un PNG
(`QQuickWindow::grabWindow`). Funciona y sale limpio, pero **no es fiel**: una
ventana que nunca se ha mostrado no asienta el layout igual, y el resultado
salió con los botones alineados a la izquierda donde en pantalla están
centrados. No sirve para una ficha de tienda.

## Un error de los caros, que pasó de verdad

`spectacle -a` captura **la ventana activa, sea cual sea**. Al lanzar la
aplicación desde una terminal sin que tome el foco, lo que se guardó fue el
navegador del usuario con su sesión abierta. Se descartó sin llegar al
repositorio, pero de ahí sale una regla:

**Comprueba el tamaño de la imagen antes de mirarla y antes de moverla al
repositorio.** La ventana de esta aplicación mide unos 430-570 px de ancho; si
la captura sale con el ancho de la pantalla, no es la ventana, es el escritorio
de alguien.

## Dos errores que ya se cometieron una vez

La primera tanda salió inservible por dos motivos, y los dos son fáciles de
repetir:

**1. En castellano.** La ficha de Flathub la lee todo el mundo, así que las
capturas van en inglés. Hay que arrancar la aplicación con el idioma forzado:

```bash
LANG=C.UTF-8 ./build/src/ui/easy-screen-recorder
```

Si se arranca sin eso, sale en el idioma del sistema. Se ve enseguida: el botón
dice «Grabar» en vez de «Record».

**2. Con la ventana maximizada.** Salieron a 1366×702 con el contenido ocupando
la cuarta parte de arriba y el resto gris. En una ficha de tienda eso parece una
aplicación a medio hacer.

La ventana declara su tamaño natural en `Main.qml` —24×21 unidades de rejilla,
unos 432×378 px— y **a ese tamaño hay que capturarla**. No maximizar, no
redimensionar: abrirla y capturar. Si el gestor de ventanas la abre maximizada,
devolverla a su tamaño antes de disparar.

## Cómo tomarlas

Requisitos que pide Flathub: **PNG**, sin transparencia, y el mismo tamaño en
todas. 1600×900 va bien para una ventana de escritorio.

La aplicación se graba a sí misma, que además es la mejor prueba de que
funciona:

```bash
./build/src/ui/easy-screen-recorder &
./build/src/cli/easy-screen-recorder-cli grabar --fuente focused \
  --salida /tmp/demo.mkv
# ...hacer lo que se quiera enseñar...
./build/src/cli/easy-screen-recorder-cli parar
```

Para los PNG sueltos, `spectacle -a -b -n -o docs/screenshots/principal.png`
captura la ventana activa sin decoración ni cursor.

Para el GIF, desde el `.mkv`:

```bash
ffmpeg -i /tmp/demo.mkv -vf "fps=12,scale=900:-1:flags=lanczos,split[a][b];\
[a]palettegen[p];[b][p]paletteuse" -loop 0 docs/screenshots/demo.gif
```

## Dos cosas que hay que cuidar

- **Nada personal en la imagen.** Se graba el escritorio real: rutas con el
  nombre de usuario, ventanas de fondo, nombres de fichero de clientes. Mirar la
  captura antes de subirla.
- **El idioma.** La interfaz sale en el idioma del sistema. Las capturas del
  metainfo deberían ser en **inglés** (`LANG=C ./build/src/ui/easy-screen-recorder`),
  porque son las que ve todo el mundo en Flathub.
