<!--
SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)

SPDX-License-Identifier: GPL-3.0-or-later
-->

# El diseño de la ventana

Todo lo que hay aquí se puede comprobar con un comando. Donde hay un criterio y
no un dato, se dice.

Los números viven en `src/ui/qml/Sistema.qml`, que es un singleton QML: se leen
desde cualquier fichero sin instanciar nada. Si vas a tocar la interfaz, léelo
antes; si vas a añadir un número nuevo, que salga de una de estas series.

Medido el 2026-09-21 en la máquina de desarrollo, con Breeze claro, tipo de
letra de 10 pt y `Kirigami.Units.gridUnit` en 18 px.

## 1. Jerarquía: por frecuencia, no por tipo de dato

La ventana tiene cuarenta y dos controles. **Uno se toca siempre** (la fuente),
cuatro a menudo (perfil, audio, emitir, repetición) y **diecisiete se tocan una
vez en la vida** (códecs, formato, resolución, carpeta por día, guion al
guardar).

Hasta la 0.9.0 los cuarenta y dos tenían el mismo tamaño, el mismo color y el
mismo peso, repartidos en dos columnas cortadas por espacio y no por sentido.
De ahí salieron los dos fallos que arreglan el resto de este documento:

- La ventana medía **1114x866 en una pantalla de 1366x768**. No cabía en la
  pantalla donde se diseñaba, y el gestor de ventanas la recortaba.
- Había **diecinueve botones «i»**. En la captura se ven como dos columnas de
  circulitos: eran la textura más visible de la página. Ruido.

Ahora: cabecera fija con fuente, Grabar y perfil; debajo, una columna de
tarjetas con título, por el momento en que se piensa cada cosa. La explicación
de cada opción va escrita debajo, siempre.

### Las secciones se pliegan, y su titulo dice lo que hay dentro

Con todo desplegado, la página medía **2567 px de contenido en un hueco de 445
px: 5,8 pantallas** para llegar a la última opción. Plegadas, **332 px**, o sea
que las seis caben de sobra en la primera pantalla y no hay que bajar nada. Con
la sección más larga abierta —Vídeo— son 991 px, 2,2 pantallas.

Esto **no es el botón «Avanzado» que se quitó en la 0.9.0**, y la diferencia es
una línea de texto: cada título plegado enseña lo que hay puesto dentro, «Vídeo
· Muy alta · 60 fps · h264». Aquel botón escondía sin decir qué escondía, y lo
que se buscó de verdad no se encontró. Aquí, plegado, se lee **más** de lo que
se sabía antes sin bajar hasta allí: la configuración entera cabe de un vistazo.

Se recuerda cuáles dejaste abiertas (`ui_abierta_<sección>`), y de fábrica
empiezan todas cerradas, porque así la primera pantalla es el mapa completo.

Cuidado al tocar esto: **la lógica no puede mirar `visible`**. Desde que hay
plegado, `visible` dice si la tarjeta está abierta, no si la opción aplica, y
mirarlo hacía que una sección cerrada perdiera las opciones puestas dentro. Para
eso están `raiz.emitiendo` y `raiz.puedeModoContent`.

## 2. Color: solo significa, nunca decora

Un color, un significado, y en ningún otro sitio.

| Color | Qué dice | Contraste medido |
|---|---|---|
| `#147ab0` | La acción principal. Una por pantalla | blanco encima: **4,73** (AA) |
| `#d62e3f` | Hay una grabación en curso | blanco encima: **4,86** (AA) |
| `#f67400` | En pausa | solo relleno, nunca texto |
| `#27ae60` | Guardado sin problemas | solo relleno, nunca texto |
| `#606b76` | Explicaciones, tema claro | sobre tarjeta: **5,44** (AA) |
| `#8e99a4` | Explicaciones, tema oscuro | sobre tarjeta: **5,25** (AA) |

El azul **no está escrito a mano**: es `Qt.darker(Kirigami.Theme.highlightColor,
1.5)`, o sea el acento del usuario oscurecido. Quien tenga Plasma en verde verá
el botón en verde.

Dos correcciones que hicieron falta, y las dos salieron de medir, no de mirar:

- **Blanco sobre el azul de Breeze da 2,49** y WCAG AA pide 4,5. Es el
  `highlightColor` tal cual, el que usa KDE en sus botones resaltados.
  Oscurecido un 50 % sube a 4,73.
- **El gris de las descripciones de Kirigami da 4,21.** Es su
  `disabledTextColor` (`#707d8a`, verificado muestreando el píxel renderizado).
  Bajado a L=42 % sube a 5,44. Se cambia poniendo
  `Kirigami.Theme.disabledTextColor` en cada `FormCard`, y lo heredan tanto sus
  delegados como los nuestros.

Naranja y verde **nunca** como texto: sobre blanco dan 2,83 y 2,87, y ahí no
hay arreglo.

### El ángulo áureo, y por qué no se usa

Girando el **ángulo áureo (137,507°)** desde el azul de Breeze (H=200,6°), el
cuarto paso cae en H=30,6°, y el naranja de Breeze está en H=28,3°: **2,3
grados**. El primer paso queda a 16° de su rojo. Es llamativo y queda apuntado,
pero **no se afirma** que Breeze se diseñara así.

Se probó darle a cada sección un tono girado por ese ángulo y **se descartó**:
teñir «Vídeo» de violeta y «Audio» de verde rompe la regla de arriba. El color
pasaría a decorar, y el rojo dejaría de ser el único que grita.

## 3. Proporción: de una serie, no del ojo

- **Ventana de ajustes: 540x687.** Proporción 1:1,2722; la raíz de phi es
  1,2720. Y cabe con holgura en una pantalla de 768.
- **Ventana grabando: 540x334.** 540/1,618 = 333,7: rectángulo áureo.
- **Ritmo, en Fibonacci:** 3, 5, 8, 13, 21, 34, 55 px → texto con su
  explicación / radio de esquina / entre controles / entre tarjetas / margen de
  la cabecera / botón secundario / botón de grabar. Dos de esos números ya son
  los de Kirigami (`cornerRadius` 5, `largeSpacing` 8), así que el ritmo nuestro
  y el nativo coinciden en vez de pelearse.
- **Tipos:** base 10 pt, paso raíz de phi (1,272) → 10, 13, 16, 21, 26, **33**.
  Los saltos grandes caen en 13, 21 y 34, que son Fibonacci, y no por magia: las
  razones de Fibonacci convergen en phi. El reloj de grabar usa el 33, porque
  esa pantalla se mira de reojo y desde lejos.

### El reparto áureo que se quitó

La cabecera se quedaba la parte corta del reparto áureo, 0,382 del alto (262 px
de 687). **Se midió lo que costaba y se quitó**: su contenido pide 221 px, así
que el mínimo regalaba 41 px a una cabecera ya holgada y se los quitaba al hueco
desplazable, que es el que anda justo. Cuando la proporción y la medida no
coinciden, manda la medida.

## 4. Los textos

Ocho reglas, sacadas de los defectos reales que tenían nuestras cadenas.

1. **Una sola voz: tú, presente, verbo delante.** Llegaron a convivir cinco
   registros: «Sale en el vídeo», «Contar 3 segundos», «pégala aquí»,
   «Mirando qué sabe hacer esta máquina…».
2. **La etiqueta nombra; la explicación dice qué te cambia.** Sin dos puntos:
   eran del formulario a dos columnas.
3. **Primero el beneficio, después el mecanismo.**
4. **Ninguna frase abre en negativo.** «La clave no se guarda» → «La clave se
   usa y se olvida».
5. **Cifras de esta máquina, no adjetivos.** «quedó en el 60 % de su tamaño»
   vence a «mucho más pequeño», y sale de la última grabación de verdad.
6. **Nada que señale a la interfaz.** Había un «mira la «i»»; si hay que
   escribirlo, la «i» sobra.
7. **Ni una palabra que el usuario no diría en voz alta.** buffer → memoria,
   caudal → ocupa, nivel → volumen.
8. **El hecho, nunca el juicio sobre su equipo.** «tu tarjeta lo hace mal» →
   «en tu tarjeta sale más grande y con menos detalle que h264».

## 5. Cómo añadir una fila

- ¿Es una casilla? `FormCard.FormCheckDelegate`, con su `description`.
- ¿Es un botón o una acción? `FormCard.FormButtonDelegate`.
- ¿Lleva un desplegable, un número o un deslizador? `FilaAjuste`, que es
  nuestro: nombre y explicación a la izquierda, control a la derecha, y la
  explicación a lo ancho de la fila entera.

`FilaAjuste` existe porque los delegados de Kirigami Addons no dejan tocar el
control que llevan dentro, y aquí hace falta: el desplegable del audio vuelve a
preguntar qué está sonando al abrirse, y los campos de tiempo se escriben a
mano.

La explicación va **a lo ancho de la fila, debajo de todo**. Metida en la
columna del nombre se quedaba en 250 px y salían párrafos de cuatro líneas: la
página medida crecía unos 400 px de puro estrechamiento.
