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
| `principal.png` | La ventana principal en reposo: el selector de fuente, el de audio y el botón de grabar. Es la `type="default"` | **Falta** |
| `grabando.png` | La aplicación grabando: el tiempo transcurrido, el botón de pausa y el indicador de la bandeja | **Falta** |
| `ajustes.png` | El panel de Avanzado abierto: formato, códec de vídeo, códec de audio y calidad | **Falta** |
| `demo.gif` | El GIF del README: abrir, elegir región, grabar, parar. 10–15 s | **Falta** |

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
