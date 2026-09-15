# Guía de marca — Easy Screen Recorder

Todos los valores de este documento salen del diseño original
(`docs/interno/brand-assets.dc.html`). **No se rediseña nada aquí**: si algo
tiene que cambiar, cambia en el diseño y de ahí se vuelve a extraer.

## El nombre

**Easy Screen Recorder**, siempre en inglés, siempre entero y siempre con las
tres iniciales en mayúscula. No se abrevia, no se traduce y no se sustituye por
«ESR» en nada que vea el usuario. `esr` y `libesr` existen solo como
identificadores internos: namespace de C++, nombre de la biblioteca y prefijo de
variables de entorno.

## Paleta

| Color | Hex | Para qué |
|---|---|---|
| Grafito | `#232629` | Fondo del icono, texto sobre claro, fondo de la cabecera y la imagen social |
| Hueso | `#EFF0F1` | Los corchetes del icono, texto sobre oscuro |
| Rojo de grabación | `#DA4453` | El punto del icono y el filo inferior de la imagen social. **Solo para el estado de grabación**: es el único acento y pierde fuerza si se usa de adorno |
| Gris de apoyo | `#6E7276` | Texto secundario |
| Gris claro | `#B9BDC0` | Subtítulo sobre fondo oscuro |
| Gris tenue | `#8B9095` | Texto terciario: la firma de la imagen social |
| Fondo oscuro | `#1B1E20` | Lienzo de prueba para las variantes en oscuro |
| Azul KDE | `#1D99F3` | La regla de la cabecera del README, y nada más |

Son los colores de Breeze de KDE a propósito: la aplicación vive en Plasma y
tiene que parecer de ahí.

## Tipografía

**Noto Sans** para todo. **Noto Sans Mono** para los textos técnicos de las
piezas gráficas.

| Uso | Tamaño | Peso | Interletraje |
|---|---|---|---|
| Logotipo horizontal | 27 px | 600 | −0,015 em |
| Logotipo compacto | 24 px, interlínea 1,15 | 600 | −0,015 em |
| Título de la cabecera del README | 46 px | 600 | −0,02 em |
| Título de la imagen social | 60 px | 600 | −0,02 em |
| Subtítulos | 22 px (cabecera) · 26 px (social) | 400 | normal |

El peso 600 es SemiBold, **no Bold**. En una máquina sin `fonts-noto-extra` no
está instalado y el navegador lo sintetiza o salta a 700, que es visiblemente
más pesado. Por eso los entregables de `docs/branding/` llevan el texto
**convertido a trazados** y no dependen de que la fuente esté: se ven igual en
cualquier sitio, incluido GitHub, que no carga fuentes externas.

Al lado de cada pieza hay un `*-editable.svg` con el texto vivo. Ese es el que
se toca para cambiar algo; el trazado se regenera con el comando que prescribe
el diseño:

```bash
inkscape logo-horizontal-light.svg --export-text-to-path --export-plain-svg \
  -o logo-horizontal-light.svg
```

## El icono

Un visor de cuatro corchetes con un punto de grabación en el centro. Tres
variantes, y las tres están en `src/ui/datos/` porque se instalan con la
aplicación:

| Fichero | Qué es |
|---|---|
| `es.solucionesconscientes.EasyScreenRecorder.svg` | Color, 128×128. Lanzador y ventana |
| `…-symbolic.svg` | Monocromo 16×16 con `currentColor`. Bandeja en reposo |
| `…-recording-symbolic.svg` | Igual pero con el punto en `#DA4453`. Bandeja grabando |

Los simbólicos usan `currentColor` para los corchetes, así que heredan el color
del tema y funcionan en bandeja clara y oscura sin duplicar ficheros. No llevan
fondo.

## Tamaño mínimo y margen

| Pieza | Mínimo | Margen alrededor |
|---|---|---|
| Icono de color | 16 px | El propio `rx="30"` del fondo ya da el aire; no añadir más |
| Simbólicos | 16 px, y están dibujados **para** 16 px | Ninguno: el viewBox es exacto |
| Logotipo horizontal | 120 px de ancho | La altura del icono a cada lado |
| Logotipo compacto | 90 px de ancho | Ídem |

Por debajo de esos anchos el texto deja de leerse y hay que usar el icono solo.

## Usos incorrectos

- **Traducir o abreviar el nombre.** Ni «Grabador Fácil de Pantalla» ni «ESR».
- **Reponer el texto con la fuente en vez del trazado** en una pieza de
  `docs/branding/`. Se verá distinto en cada máquina.
- **Usar el rojo `#DA4453` para algo que no sea grabación.** Es el único acento
  y su trabajo es significar «estoy grabando».
- **Poner el logotipo sobre una foto o un fondo con textura.** Está pensado para
  fondo plano, claro u oscuro.
- **Recolorear el icono de color.** El grafito y el hueso son el icono; si hace
  falta monocromo, existe el simbólico.
- **Estirar cualquier pieza.** Todas llevan `viewBox`: se escalan, no se
  deforman.
