# Cambios

Formato: una entrada por tanda, con lo verificado. Las fechas son de la
máquina de desarrollo.

## Sin publicar

### La cuenta atrás se ve en la bandeja (2026-09-21)

Con la cuenta atrás puesta, la ventana **se aparta al instante** y los tres
segundos se cuentan en la bandeja, en un icono con el dígito a todo el alto.

Esto deshace una decisión que estaba escrita en el propio código: contar con la
ventana delante y apartarla al final, porque minimizar a mitad de cuenta «dejaba
el último segundo a ciegas». Era verdad mientras la bandeja no dijera nada. Ahora
lo dice, y además los tres segundos sirven para lo que deben servir: para que
cuando empiece a grabar la ventana ya no esté.

El «Cancelar» vivía en la ventana, que ahora no está, así que entra en el menú de
la bandeja y solo aparece mientras cuenta. Y el dígito se enseña **aunque el
reloj de bandeja esté apagado**: dura tres segundos y es lo único que dice cuánto
falta.

Verificado: los dígitos renderizados y mirados a 22 px, bien distintos del reloj
de dos líneas; el dibujo de los dos iconos unificado en una función en vez de
copiado; sin errores de QML al arrancar; ctest 12/12. El recorrido completo lo
confirmó el titular a mano, que es la única forma: la cuenta atrás solo arranca
desde el botón de la ventana.

### Reducir el tamaño al terminar, y que la máquina se mida a sí misma (2026-09-21)

**Lo que aporta este programa sobre GSR era «UI ligera, modo audio-only y
post-proceso», y el post-proceso llevaba desde el 15 de septiembre en
«investigado, pendiente de decisión». Ya existe.**

Al terminar de grabar, si se pide, la grabación se vuelve a comprimir por
procesador y el fichero se queda **en torno a la mitad** sin que se note. No es
magia: un codificador por hardware tiene un presupuesto de tiempo por fotograma y
hace lo que le da tiempo; uno por software vuelve sobre los mismos fotogramas y
encuentra lo que la GPU no tuvo tiempo de buscar. No se tira calidad, se recupera
el trabajo que la GPU no hizo.

Dos niveles, los dos medidos (tabla completa en `docs/post-proceso.md`): «Reducir
el tamaño» (CRF 23, queda en el 53 % con escritorio y el 55 % con movimiento,
prácticamente indistinguible) y «Reducir al máximo» (CRF 28, 42 % y 27 %, con una
pérdida pequeña pero real).

**Mismo códec, mejor codificador.** La versión agresiva se planteó cambiando a
HEVC y la medición lo desmintió: x264 en CRF 28 deja el fichero **más pequeño**
que x265 en CRF 28 (27 % contra 33 %), en la mitad de tiempo y sin cambiar de
formato. Cambiar de códec además traicionaría a quien eligió webm por la web o
h264 porque lo abre su televisor. Así que no se cambia.

**Quién se queda fuera, y por qué, todo medido:** webm, porque recomprimir vp8
con libvpx-vp9 costó **quince veces la duración del vídeo** para dejarlo en el
93 %; HDR y 10 bits, porque recomprimirlos destruiría justo lo que alguien fue a
buscar; solo audio, que ya está comprimido; y av1 hasta medir SVT-AV1. hevc sí
entra, avisando de que tarda el doble.

**La interfaz no promete ningún porcentaje, y es deliberado.** El porcentaje es
una división cuyo divisor es lo bueno que fuera el codificador de esa tarjeta: en
una GPU moderna la grabación ya nace apretada y el margen es menor. Así que la
ventana da el dato real al terminar, «12,4 MB → 6,3 MB», y **la máquina recuerda
lo que le pasó a ella** para decir la próxima vez «la última vez quedó en el
59 %». Cada equipo habla de lo suyo en vez de repetir una medición hecha en otro.
Una cifra **por nivel**, además: al probarlo se vio que con una sola, el 39 % de
«al máximo» se enseñaba también al elegir «reducir», y una cifra que no
corresponde es peor que ninguna.

**Y la misma idea contra el hevc poco fiable.** El grabador escribe en su log
«Driver does not advertise encoder features» cuando el driver no declara de qué
es capaz su codificador y ffmpeg lo conduce a ojo. Eso se lee al parar y se
recuerda, y a partir de ahí la lista de códecs dice «tu tarjeta lo hace mal: sale
más grande y peor que h264». No es una lista de tarjetas escrita a mano, que es
lo que el proyecto prohíbe: es el propio equipo diciendo lo que le pasa. En una
tarjeta que declare sus capacidades, esto no se enciende nunca.

**Lo elegido en la ventana se recuerda.** Hasta ahora se guardaban cuatro cosas;
ahora son dieciocho: fuente, formato, los dos códecs, calidad, fps, audio,
resolución máxima, cámara con su tamaño, posición y espejo, cuenta atrás, modo
repetición y sus segundos, caudal de emisión, guardar la emisión y qué hacer al
terminar. **No se guardan** la clave de emisión, que no toca el disco jamás, ni
la región recortada, que es de esa sesión. Y solo se apunta lo que se **elige**:
abrir y cerrar el programa sin tocar nada no escribe ni una línea, comprobado.

Verificado ejecutando: grabar y reducir por CLI deja 0,5 MB en 0,3 MB (59 %) y el
fichero sigue leyéndose; el original nunca se toca hasta que el reducido existe y
tiene duración, y si algo falla se queda intacto, con test; `ejecutar()` mataba la
recompresión a los cinco segundos —su límite pensado para las sondas de
`--check`— y ahora el margen se calcula sobre la duración del vídeo; y lanzando la
ventana con una fuente recordada, arranca con ella puesta, que es el caso difícil
porque la lista llega después de la detección.

Dos cosas más de casa: la segunda captura de la ficha pasa a ser el selector de
región, porque con las opciones ya desplegadas era idéntica a la primera; y el
**VFR**, que no se mencionaba en ningún texto de usuario, entra en la ficha y en
los dos README. No codificar fotogramas repetidos es media ventaja del programa y
era un secreto.

### Las opciones, a la vista y explicadas (2026-09-21)

Ocho cambios pedidos por el titular después de usarla, más lo que salió al
hacerlos. Todos empiezan en la misma raíz: **lo que está escondido no existe**.
La casilla del reloj de bandeja se había puesto solo en el menú de la bandeja y
el primero que la buscó la buscó en la ventana.

**Se acabó el botón «Avanzado».** Las opciones salen directamente, en las dos
columnas de siempre, y la ventana nace del tamaño que necesita. La página pasa a
ser desplazable: sin barra, el día que el contenido no quepa —otro tipo de letra,
otro panel, una pantalla más baja— las últimas filas se salen y no hay forma de
llegar a ellas, que es un fallo que este formulario ya tuvo. Esto **cambia
`CLAUDE.md:80`**, que decía lo contrario, y la línea se ha reescrito con el
motivo: los dos clics para grabar siguen siendo dos.

**Una «i» al lado de cada opción con explicación.** Los tooltips colgaban del
propio control y eran invisibles: nada decía que hubiera algo que leer. Ahora son
`Kirigami.ContextualHelpButton`, que es de KDE y se abre al pasar por encima y al
pulsar. Once opciones. Comprobado que existe **en el runtime del flatpak**
(`org.kde.Platform 6.11`) y no solo en el Kirigami del sistema, que es lo que
decide si la aplicación instalada se rompe.

**Calidad: se intentó poner las cifras y hubo que quitarlas.** «Alta» no le dice
nada a nadie, así que las opciones pasaron a leerse «Muy alta · ~3,0 MB/min», con
las cifras medidas aquí. Duró hasta medir los códecs: esa cifra sale de una
grabación con **h264**, y con hevc el mismo ajuste da casi el doble. Una cifra que
es verdad en una opción y mentira por el 80 % en la de al lado no es una cifra, y
hacerla depender del códec sería perseguir una tabla que además cambia con cada
GPU. Se quitan. La «i» explica lo que sí es estable: que se graba a calidad
constante, que por eso no hay tamaño que anunciar, y que depende de lo que se
mueva en pantalla y del códec. Detrás, por si alguien lo busca: QP equivalente a
h264 de 35, 30, 25 y 22 (`src/recorder/video_codec.c:9-17`).

**«Tamaño del vídeo» pasa a ser «Resolución máxima»**, y las opciones dicen la
pantalla de verdad: «La de la pantalla (1366×768)». Los techos por encima de la
pantalla **dejan de ofrecerse**: «como mucho 1080p» en una pantalla de 768 px no
hacía absolutamente nada, y el proyecto no enseña lo que la máquina no va a usar.

**Fuera «auto» del códec de vídeo.** Estaba delante y era el default, y no dice
nada: para saber en qué se estaba grabando había que ir al log del grabador. Cada
códec sale ahora con una línea al lado que dice lo que hay que saber para
elegir: «h264 · lo reproduce todo, hasta un televisor viejo», «hevc · no lo abren los
televisores antiguos; mira la "i" antes», «av1 · sin patentes; hace
falta un equipo reciente para verlo».

Y ahí salió algo que había que medir antes de escribirlo, porque el folclore dice
lo contrario: **hevc no hace el fichero más pequeño, lo hace más grande**. Dos
rondas intercaladas grabando lo mismo a la misma calidad dieron h264 2,44 y 2,43
MB/min, hevc 4,55 y 4,32, vp8 2,83 y 2,84. O sea un 80 % MÁS en hevc. La causa
está en el código de GSR: a h264 y a hevc les pasa el mismo número de calidad
(`video_codec.c:22-24`; a vp8 lo dobla y a av1 y vp9 los multiplica por cuatro,
porque sus escalas son otras), y con ese mismo número este codificador gasta más
bits en hevc. Eso es más detalle, no menos tamaño. La «i» lo dice con las
cifras. La ventana elige **h264** cuando está, que es exactamente lo que elegía
GSR solo (`codec_select.c:449-462`). Un identificador que no conozcamos sale tal
cual, sin descripción inventada: la lista la da la máquina y GSR puede añadir
nombres cuando quiera.

Lo que se pierde con eso, dicho: la vuelta atrás de GSR cuando la captura no cabe
en la resolución máxima del codificador h264 de esa GPU, que es cosa de pantallas
enormes. `libesr` y el CLI siguen aceptando `auto`.

**Y midiendo esto salió un hallazgo que cambia el consejo entero: aquí hevc es
peor que h264 en todo.** Se montó el experimento bien hecho, que es la única
forma de saberlo: una grabación de escritorio convertida en referencia **sin
pérdida** a 30 fps fijos, recodificada con los dos códecs a los cuatro niveles
que la aplicación sabe pedir, y comparada contra esa referencia con SSIM y PSNR.
Mismo material para todos, sin el ruido de que el escritorio se mueva distinto en
cada pasada.

| Nivel | h264 MB/min · SSIM | hevc MB/min · SSIM |
|---|---|---|
| Media (QP 35) | 0,98 · 0,97399 | 1,34 · 0,97086 |
| Alta (QP 30) | 1,37 · 0,98942 | 1,73 · 0,98812 |
| Muy alta (QP 25) | 1,83 · 0,99640 | 2,16 · 0,99515 |
| Ultra (QP 22) | 2,05 · 0,99926 | 2,49 · 0,99720 |

En los cuatro, hevc ocupa entre un 18 % y un 37 % más **y** se parece menos al
original. Y no hay ningún punto donde gane: igualar la calidad de h264 en «Muy
alta» exige hevc en «Ultra», que cuesta un 36 % más de disco por la misma pinta.

La causa está en el log del propio grabador: «Driver does not advertise encoder
features, using guessed defaults». El driver de esta GPU **no declara lo que su
codificador HEVC sabe hacer**, así que ffmpeg lo conduce a ojo, y un codificador
conducido a ciegas reparte mal los bits. `vainfo` lo confirma desde fuera: esta
tarjeta solo anuncia puntos de entrada de codificación para H264 y JPEG.

Con eso se cae la idea que veníamos persiguiendo, la de traducir el nivel de
calidad por códec para que «Muy alta» significara lo mismo en todos: no hay tabla
de equivalencia que salvar cuando un códec pierde en todos los puntos. La «i» del
códec dice lo medido y recomienda h264 a quien no sepa cómo se porta su tarjeta.

Dos tropiezos del camino, por si hay que repetirlo: el ffmpeg del anfitrión **no
puede** codificar HEVC en esta GPU (hay que usar el del flatpak de GSR, que es el
que usa la aplicación de verdad), y ese ffmpeg del flatpak **no puede leer** una
referencia h264 sin pérdida, porque trae libopenh264 y solo entiende el perfil
baseline: la referencia va en ffv1.

**El audio por defecto pasa a ser los dos mezclados**, sistema y micrófono en una
pista. Con una salvaguarda que no es opcional: solo si hay micrófono **de
verdad**. GSR lista siempre `default_input`, que es un nombre que se inventa él, y
la lista trae además los «monitores», que son la salida de cada altavoz. Así que
`hay_microfono()` cuenta lo que no es ni inventado ni monitor, y sin micrófono el
default se queda en solo sistema. Con test: el volcado real de esta máquina da
sí, una lista fabricada sin entradas da no.

**Y el vúmetro deja de escuchar solo.** Su fila vivía detrás del plegado, así que
«escuchar cuando se ve» significaba «cuando abres Avanzado»; con el formulario
siempre a la vista pasaba a significar **«al arrancar»**, y la aplicación habría
encendido el piloto del micrófono nada más abrirla, sin que nadie haya pedido
grabar. Ahora hay un botón «Probar» y el micro se abre cuando se pide.

**Guardar la emisión viene marcada**, y debajo se dice dónde va a escribir y
cuántos GB por hora son con el caudal elegido: viniendo marcada, eso deja de ser
una elección informada y hay que decirlo a la cara. Y la casilla de emitir ya no
dice «en vez de guardar un fichero», que desde ayer es mentira.

**El reloj de bandeja se llama igual en los dos sitios**, adaptado a dónde se lee:
«Mostrar el tiempo de grabación en la bandeja» en la ventana y «Mostrar el tiempo
de grabación» en el menú de la bandeja, donde decir «en la bandeja» sobra.

Verificado ejecutando. La ventana se ha **mirado**: a 51 unidades de rejilla de
ancho, las dos columnas, las «i» en su sitio y sin etiquetas cortadas. Las 46 de
antes se quedaron cortas al meter una «i» por fila y volvía a leerse «ágenes por
segundo», que es el mismo corte que ya obligó a no igualar el ancho de las dos
columnas. ctest 12/12, sin un aviso, `reuse lint` en verde y 126 cadenas
traducidas, 0 sin terminar.

**Y el arranque, con una corrección por el camino.** La primera medida decía que
maquetar el formulario entero al abrir costaba unos 115 ms, y esa cifra estaba
mal: comparaba tres pasadas contra siete tomadas en otro momento. Repetida en
condiciones —portátil enchufado, siete pasadas de cada— sale así: la **v0.8.0 ya
publicada** arranca en **989 ms de media** (882-1071) y con todo esto encima,
**1061 y 1010 ms** en dos tandas. La diferencia entre las dos tandas de hoy es
casi igual de grande que la diferencia contra la base, así que **el coste del
rediseño no se distingue del ruido**.

Lo que sí queda claro es lo otro: el contrato de `CLAUDE.md` es menos de un
segundo y **esta máquina ya se pasaba antes de tocar nada**. No es una regresión
de esta tanda.

Lección de método, apuntada porque costó dos rodeos: con la medición a ±100 ms,
tres pasadas no distinguen nada, y **el portátil con la batería corre a la mitad**
(gobernador en ahorro, 1600 MHz fijos). Una tanda entera de medidas se tiró por
eso: `--check`, que nadie había tocado, había pasado de 0,66 s a 2,0 s.

### Guardar la emisión, y la casilla del reloj donde se busca (2026-09-21)

**Emitir en directo puede guardar además un fichero.** Lo que sale por el cable
se ha ido, y hasta ahora no quedaba nada. Ahora hay una casilla en el bloque de
emisión, apagada, y al marcarla la emisión se guarda en la carpeta de vídeos.

No es una segunda grabación ni un segundo proceso: el **mismo** GSR escribe el
fichero mientras emite, así que no cuesta otra codificación ni el doble de GPU.
Lo trae de serie desde la 6.0.0, que es nuestro mínimo: `-ro` abre la carpeta y
la grabación se enciende por IPC con `start-replay-recording`
(`gpu-screen-recorder.1`, «-ro» y la sección IPC). Con `-ro` a secas no graba
nada, así que lo enciende `empezar_grabacion` en cuanto el socket responde; si el
grabador dice que no, se para todo y se avisa, porque quien marcó «guardar» no
puede enterarse al final de que no hay fichero.

Al parar se cierra ese fichero **antes** del stop, porque su respuesta trae la
ruta y la del stop de una emisión viene vacía; y si viene con algo, es la URL del
servidor, que ni es un fichero ni es algo que convenga enseñar, porque lleva la
clave pegada detrás. El fichero sale en `.flv`, que es el contenedor de la
emisión: GSR usa el mismo para las dos salidas.

En el CLI es `emitir --guardar`, y al parar imprime la ruta.

**Y la casilla del reloj de la bandeja pasa a estar también en «Avanzado».** La
puse solo en el menú de la bandeja razonando que era una preferencia de la
bandeja; el primero que la buscó la buscó en la ventana y no la encontró. Las dos
casillas son la misma preferencia, no dos copias: escriben en el controlador y
cada una se entera de lo que hace la otra, así que no pueden decir cosas
distintas.

Verificado ejecutando. Emitiendo a un receptor local de ffmpeg, nunca a una
cuenta real: por el CLI salieron las dos cosas a la vez, el fichero guardado con
h264 1366×768 + aac y 8,03 s y la emisión recibida con 8,07 s. Por el camino de
la interfaz, con la autoprueba: 3,25 s de fichero y 3,28 s recibidos, y el aviso
de «grabación guardada» con la ruta dentro. En `prueba_ajustes`, que `-ro` sale
con la carpeta, que sin carpeta no sale, y que pedir guardar en una grabacion
normal es un error. 131 comprobaciones, 0 fallos; ctest 12/12 y sin un aviso.

Un detalle que costó un rato y queda escrito: el primer intento guardó en una
carpeta de `/tmp` y pareció que no había guardado nada. Sí había guardado, pero
**dentro del sandbox del flatpak de GSR**, porque el `/tmp` de un flatpak es
privado. Es la misma trampa que ya obligó a poner la sesión bajo `~/.cache`.

### El tiempo de grabación en la bandeja, si se pide (2026-09-20)

Mientras se graba, la ventana está apartada a propósito. El tiempo que llevas
grabando solo se veía trayéndola de vuelta, que es justo lo que no hay que hacer.
Ahora lo dice la bandeja.

Va en un **segundo icono**, al lado del de la aplicación, y no pintado encima del
suyo: la marca tiene que seguir siendo reconocible justo cuando está grabando,
que es cuando se la busca en el panel. Solo existe mientras hay algo en marcha.

Y en **dos líneas, minutos arriba y segundos abajo**, porque el hueco de un icono
en el panel son unos 22 px: `01:23` en una línea sale a cuatro píxeles por cifra
y no se lee. Partido, cada cifra se queda con la mitad del alto, que es la altura
a la que el propio panel escribe su reloj. Pasada la hora, arriba van las horas
con su «h» y abajo los minutos; el tooltip lleva siempre el tiempo entero.

Fondo rojo y cifras blancas, y no el color de texto del tema. Un pixmap que
pintamos nosotros el panel no lo recolorea, así que con el color del tema las
cifras desaparecen en cuanto el panel va al revés del esquema de color. El rojo
se lee igual sobre un panel claro y sobre uno oscuro, y además dice lo que está
pasando; en pausa se apaga a gris.

**Es opcional y viene apagado.** La casilla está en el menú de la bandeja,
«Enseñar el tiempo en la bandeja», y no en el «Avanzado» de la ventana, por dos
razones: es una preferencia de la bandeja, y «Avanzado» se deshabilita mientras
se graba, que es justo el momento en que a alguien se le ocurre que querría ver
el tiempo. Se recuerda entre sesiones en la clave `reloj_bandeja`, y un valor que
no se entiende no enciende nada.

De paso, un fallo que estaba y no se veía: la bandeja no contaba «emitiendo»
entre los estados en marcha, así que emitiendo en directo ponía el icono de
reposo y escondía el «Terminar» que hacía falta para cortar el directo sin sacar
la ventana. La lista de estados en marcha estaba escrita dos veces con dos
contenidos distintos; ahora está una sola vez y dice lo mismo que la ventana.

**Y una caída, medida antes que supuesta:** darle al reloj el mismo `QMenu` que
al icono de la aplicación tumba la aplicación. Con esa línea puesta, una
grabación de tres segundos por `ESR_AUTOPRUEBA` acaba en violación de segmento al
salir y por el camino el exportador escupe «No id for action»; sin ella sale con
0, igual que el binario sin este cambio. Un mismo `QMenu` exportado por dos items
de bandeja se reparte los identificadores de sus acciones. Así que el reloj se
queda sin menú: el menú está a un icono de distancia.

Verificado ejecutando, y contra el binario. Que las cifras se leen a 22 px: un
arnés compila `reloj_bandeja.cpp`, le pide los iconos y los guarda, y se han
mirado sobre panel claro (#eff0f1) y oscuro (#232629); ahí entran también los
casos raros, 00:09, 01:03, 59:59, 1h/00 y 12h/34, porque el tamaño de letra se
busca hasta que la línea más larga cabe a lo ancho.

Que aparece de verdad: con una grabación en marcha, el proceso registra **dos**
items de bandeja, y el segundo exporta `IconPixmap` en 22, 24, 32, 48 y 64 px y
el tooltip «Grabando · 00:00», leídos por DBus, no supuestos. Que la casilla está
y manda: `com.canonical.dbusmenu.GetLayout` la devuelve como `checkmark = 1`, y
con la opción apagada el proceso registra un solo item durante toda la grabación.
Que se va al parar: pulsando «Parar y guardar» por DBus, como haría el panel,
queda un item. Y que la preferencia se recuerda, con ida y vuelta en
`prueba_configuracion`, donde además un «true» escrito a mano no la enciende.

Regresión: compila sin un aviso, ctest 12/12, `reuse lint` en verde.

Sin verificar: el gris de la pausa solo se ha visto en el arnés, no en el panel,
y el tamaño que pide tu panel, que es lo que decide cómo de grandes se ven las
cifras.

### El recorte de región se ajusta antes de grabar (2026-09-20)

Soltar el ratón elegía la región y arrancaba la grabación en el mismo gesto. Un
recorte torcido no tenía arreglo: había que dejar que grabase, pararla y volver
a abrir el selector. Y el rótulo no decía en ningún momento qué iba a pasar al
soltar, así que el primer arrastre de cualquiera es a ciegas.

Ahora soltar deja el recorte puesto. Se mueve arrastrándolo por dentro y se
estira por los bordes y las esquinas, con el cursor diciendo cuál se agarra.
**Enter graba**; Esc cancela. El rótulo cambia en cuanto hay recorte, porque
antes de tenerlo Enter no hace nada y ofrecerlo sería mentir.

Por dentro el recorte pasa a guardarse como rectángulo y no como los dos puntos
del arrastre: con dos puntos, estirar un borde obliga a adivinar cuál de ellos
es. Pasarse de largo con un borde cambia de borde, en vez de dejar el rectángulo
del revés o clavado en cero. El agarre son 12 px, recortados a la mitad del lado
para que en un recorte pequeño no alcancen los dos bordes a la vez. Nada se sale
de la pantalla, y se limita mientras se arrastra: lo que se ve es lo que se
graba.

Y el velo pasa a ser **cuatro trozos alrededor del recorte** en vez de uno encima
de todo. El comentario del fichero decía «el recorte en claro sobre el fondo
oscurecido» y eso no era verdad: el velo tapaba también el recorte, así que el
encuadre se elegía a través de un 45 % de negro. Ahora el recorte se ve tal cual,
que es justo lo que se va a grabar. El alfa va en el color y no en `opacity`,
porque cuatro trozos con opacidad de grupo obligan a componer la pantalla entera
aparte, y se tocan sin solaparse, porque solapados el negro se sumaría y la
costura se vería.

Eso dejó al rótulo sin velo debajo: con un recorte grande cae dentro, sobre lo
que haya, y blanco sobre blanco no se lee. Lleva una pastilla oscura detrás.

Verificado con un arnés desechable que carga el QML real y le manda los eventos
de ratón y de teclado. Vive fuera del repositorio: dentro costaría Qt6::Test como
dependencia, y la interfaz no se verifica. Arrastrar y soltar deja 300×250 sin
emitir nada; arrastrar por dentro mueve sin cambiar el tamaño; el borde derecho y
la esquina de arriba a la izquierda estiran solo lo suyo; Enter emite
`400x300+100+110` y cierra; un clic suelto no es recorte y Enter no hace nada;
Esc cancela.

El velo no se da por bueno de palabra: el arnés lee la imagen de la ventana con
`grabWindow()` y cuenta píxeles. Dentro del recorte, cero píxeles tapados; fuera,
cero agujeros; los cuatro lados al negro del 45 % exacto (alfa 115); y la
pastilla del rótulo cubre todo lo que hay debajo (alfa mínimo 200, el suyo). 43
comprobaciones, todas en verde.

Sigue **sin verificar** el arrastre humano sobre el compositor real y la escala
fraccionaria.

### Las cadenas que faltaban en la traducción al inglés (2026-09-20)

Siete cadenas de las dos últimas tandas nunca llegaron al `.ts`: los cuatro
caudales de emisión, su explicación y las dos del menú de la bandeja. En inglés
salían en español, porque Qt cae al texto fuente cuando no encuentra traducción,
y eso no se ve desde aquí, que es en español. Regeneradas con `lupdate
-no-obsolete`, que además se llevó tres entradas muertas: las cifras de 4500 kbps
que estaban mal y se corrigieron en la Tanda 18.

`lrelease` dice 118 traducciones, 118 terminadas y 0 sin terminar, y las cadenas
nuevas se leyeron del `.qm` compilado, no del `.ts`.

### Empezar desde la bandeja, y apartarse antes de grabar (2026-09-17)

Al dar a Grabar se veía la ventana minimizarse dentro del vídeo. La causa no era
la animación, era el orden: el minimizado colgaba de `onEstadoCambiado`, o sea
que ocurría cuando el estado ya era «grabando», y ese estado solo llega cuando
`empezar_grabacion()` ha vuelto con éxito. Para entonces GSR llevaba rato
capturando. Y no eran milisegundos: entre pulsar y minimizarse cabe todo el
arranque del grabador, con un techo de `kEsperaSocketMs`, 15 s
(`grabacion.cpp:29`).

Lo de fondo, sin embargo, era otra cosa: **no había forma de EMPEZAR desde la
bandeja**. Su menú sabía mostrar, guardar, pausar, parar y salir, todo para
operar una grabación ya en marcha. Así que la única vía de empezar era la
ventana, que es justo la que no debe salir. Ahora hay «Grabar la pantalla» en el
menú, con el atajo escrito al lado cuando está registrado, porque el menú es
donde alguien lo va a descubrir: la ventana también lo dice, pero la ventana es
lo que no está delante cuando hace falta.

Con la ventana ya apartada no hay nada que minimizar, así que no hay animación
que colarse ni retardo que adivinar. Empieza cuando se pulsa, y el icono de la
bandeja lo confirma en el instante en que el grabador arranca de verdad.

Para el botón de la ventana sí hace falta apartarse primero: se espera la señal
de visibilidad y además un margen de 350 ms, con un tope de 900 ms para que el
botón no se quede muerto en un escritorio que no minimice. El margen hace falta
porque en Wayland esa señal dice que el compositor aceptó el cambio de estado,
no que haya acabado de dibujar. **La duración real de la animación de KWin quedó
sin medir**: el arnés para medirla grababa el escritorio completo y se descartó
por eso, así que el margen está dimensionado por arriba.

El atajo global también se cubre: `alternarGrabacion()` llamaba a `grabar()`
directamente desde C++ y se saltaba todo esto.

La cuenta atrás de tres segundos pasa a venir **desactivada**. Lo que se
espera al dar a Grabar es que grabe, y una espera que nadie pidió deja dudando
cuándo empieza de verdad. Quien la quiera sigue teniéndola en Avanzado.

Se probó y se descartó minimizar al llegar a «1» de la cuenta atrás para que ese
segundo tapara la animación: dejaba el último segundo a ciegas y no se sabía
cuándo empezaba de verdad, que es peor defecto que el que venía a arreglar. La
cuenta se ve entera y los dos caminos usan el mismo mecanismo.

Verificado: compila sin avisos con `-Werror`, 12/12 en ctest, la interfaz
arranca sin errores de QML, y la entrada nueva del menú se leyó del item de la
bandeja por DBus (`com.canonical.dbusmenu.GetLayout`) contra el binario en
marcha. La grabación resultante **no** se ha verificado: la regla del proyecto
es que la interfaz no se verifica.

### Capturas de la ficha, rehechas y repetibles (2026-09-17)

Las tres eran de la v0.1.0 y ya no se parecían a la aplicación: enseñaban
`eDP-1` en vez de «Laptop screen · 1366×768», la ventana estirada y el panel
Avanzado cortado por abajo. Y eran RGBA de dos tamaños distintos, cuando Flathub
pide PNG sin transparencia y todas iguales.

Ahora están en inglés, sin maximizar, a 1100×790 y sin alfa, con la emisión en
directo ya visible en el panel Avanzado.

**Y deja de ser una tarea manual.** Lo que la bloqueaba no era pereza: en
Wayland la aplicación **no puede ponerse delante sola**, porque
`requestActivate()` necesita un token de activación que solo concede un clic de
verdad. La salida es la interfaz de scripting de **KWin**, que sí puede activar
una ventana y quitarle el maximizado. `scripts/capturas-ficha.sh` hace las tres
de una, sin un solo clic.

Dos trampas apuntadas en `docs/screenshots/README.md` para la próxima vez:

- El identificador de la ventana es **`resourceName`** y no `resourceClass`: la
  clase es `es.solucionesconscientes.EasyScreenRecorder`, así que buscar ahí
  «easy-screen-recorder» no casa por las mayúsculas, el script no encuentra nada
  y no dice por qué.
- La de «grabando» se toma arrancando la grabación **antes** de abrir la
  ventana: así se abre ya en ese estado y no se minimiza, porque minimizarse es
  su reacción a que el estado *cambie*.

El metainfo pasa a apuntar a un **commit** en vez de a un tag. Apuntar a un tag
obligaría a capturar antes de etiquetar, o a mover una etiqueta ya publicada.
Verificado: las tres URLs responden 200 y `appstreamcli validate` da el metainfo
por bueno.

### Emitir en directo por RTMP (2026-09-17)

**GSR ya emitía y no lo sabíamos.** Reconoce `rtmp://` y `rtmps://` en `-o`
(`args_parser.c:234`) y su manual trae el ejemplo de Twitch. Esto no añade una
función nueva: expone una que ya estaba, con las reglas correctas.

- **Verificado emitiendo de verdad** contra un `ffmpeg -listen 1` en
  `127.0.0.1`, nunca contra una cuenta real: llegaron h264 1366×768 y aac,
  21,1 s a **2,16 Mbps de los 2500 pedidos**. Y por el camino de la interfaz,
  24,8 s.
- Lo que hacía falta: el contenedor **flv** (que solo respeta **aac**, no opus),
  el **bitrate constante** (`-bm cbr`), y dejar que la salida sea una URL —había
  tres sitios que daban por hecho un fichero—. Ojo con `-q`: en modo cbr son
  kbps y no `very_high`.
- **La clave no se guarda en ningún sitio**, y se dice al lado del campo donde
  se pega. Lo que sí se recuerda es el servidor de ingesta, que es una URL
  pública. `recordar_url_emision()` se niega a guardar una cadena que parezca
  llevar la clave pegada detrás, y la clave no se imprime ni va al log.
- **Emitiendo no hay pausa.** El manual de GSR sugiere que no se puede, pero
  probado **sí la acepta** y responde `Paused`; lo que pasa es que deja de
  mandar imagen y la plataforma da la emisión por caída. La interfaz no la
  ofrece; la línea de comandos avisa y deja hacer.
- Y el modo repetición se excluye: emitiendo no hay buffer que guardar.
- El arnés pasa a **nueve casos**, con la emisión dentro.

**Un error que cometí y queda escrito**: las primeras cifras de bitrate que puse
(4500 kbps para 1080p) las saqué de memoria y estaban mal. YouTube pide 10 Mbps
para 1080p30 y 12 para 1080p60. Ahora el selector lleva sus cifras, con la fuente
en `docs/emision.md`, y el bitrate se preselecciona según la pantalla y las
imágenes por segundo. De leer la fuente oficial salieron dos cosas más: YouTube
pide keyframes cada 2 s y GSR ya usa 2,0 por defecto, así que cumplimos sin
tocar nada; y recomienda **RTMPS**, que es lo que pone el texto de ejemplo.

### La lista de aplicaciones sonando se calculaba una sola vez (2026-09-16)

De una pregunta del titular: «¿por qué smplayer? podría ser cualquier app». Y
sí, es cualquiera — la lista sale de preguntarle a GSR qué está sonando. Pero al
mirarlo apareció que **se calculaba solo al arrancar**, y esa lista casi nunca
es la buena: entre abrir el grabador y darle a grabar, el usuario abre justo la
aplicación que quería grabar, y ahí ya no aparecía. Había que reiniciar.

Ahora se vuelve a preguntar **al desplegar el selector de audio**, que es el
único momento en que importa. Es una sonda suelta, no la detección entera: cuesta
lo que tarde GSR en contestar, no los 0,7-0,9 s del arranque, y va en hilo aparte
para no congelar el menú.

Verificado en caliente: con la aplicación abierta y nada sonando, la lista traía
`["Brave"]`; al empezar a sonar algo pasó a `["Brave","paplay"]` sin reiniciar.

### El atajo global llevaba tandas sin funcionar (2026-09-16)

Salió de una pregunta del titular —«¿hay atajos de teclado?»— que ya era la
respuesta: si hay que preguntarlo, no se anuncian. Al ir a mirarlo aparecieron
**tres fallos encadenados**, y el atajo no funcionaba desde hacía tandas.

- **El camino DBus llevaba guiones, y DBus no los admite.** El componente se
  llamaba `capturia`, sin guiones, así que el camino era válido y el atajo
  funcionaba. **Al renombrar el proyecto a `easy-screen-recorder` aparecieron
  los guiones**, la conexión a la señal falló en silencio y el atajo dejó de
  disparar nada. Seguía saliendo en Preferencias del sistema, eso sí.
  Comprobado con `busctl --user tree`: el camino real es
  `/component/easy_screen_recorder`, con guiones bajos. Ahora se calcula, para
  que el próximo renombrado no vuelva a romperlo.
- **Y la tecla por defecto estaba ocupada.** Meta+Shift+R la tiene **Spectacle**
  para «Iniciar/detener grabación de región», y también Meta+Alt+R y
  Meta+Ctrl+R. kglobalaccel no concede una tecla ocupada: devuelve un cero y no
  se queja. Ahora se prueban varias candidatas y se coge la primera libre; en
  esta máquina queda **Meta+Shift+G**.
- **Y una acción guardada sin tecla no se arregla pidiendo por las buenas.**
  Con la bandera normal, kglobalaccel carga el «sin tecla» guardado y rechaza
  cualquier default, para siempre. Medido: bandera 2, ni una combinación libre
  concedida; bandera 4, concedida. Se pide primero por las buenas —para no pisar
  la tecla que el usuario haya elegido— y solo se fuerza si no ha quedado
  ninguna.

Además:

- **Atajo nuevo de pausa**, que es el que de verdad hacía falta: pausar obligaba
  a sacar la ventana a la pantalla que se está grabando. En esta máquina queda
  **Meta+Shift+P**. Verificado midiendo: 19 s de reloj con 8 en pausa dan un
  fichero de **10,6 s**.
- **La interfaz los anuncia**, y con la tecla de verdad: se le pregunta a KDE
  cuál tiene puesta en vez de suponer la que pedimos, así que si el usuario la
  cambia, el texto le sigue.
- **«Solo smplayer» no decía de qué.** Se leía como «solamente smplayer». Pasa a
  **«Solo el sonido de smplayer»**, que es lo que hace: graba el audio de esa
  aplicación y deja fuera todo lo demás.

Verificado ejecutando: los tres atajos por DBus sobre la aplicación viva
—empezar, pausar, reanudar y parar—, build sin un warning, `ctest` 12/12,
`verify-recording.sh` con los ocho casos en verde, `reuse lint` conforme.

### El apagón, hasta el final: audio, contenedores y reparación (2026-09-16)

El arreglo anterior cubría la mitad. Al medir formato por formato salieron dos
agujeros más, y uno era grave.

- **El modo de solo audio lo perdía TODO menos wav.** Medido matando ffmpeg a
  los 8 segundos: opus, flac y mp3 dejaban un fichero de **0 bytes** y aac uno
  de 44. Nuestro backend de audio no forzaba el volcado a disco, y el arreglo
  anterior solo tocaba la grabación de pantalla, que va por otro camino. Ahora
  también lo fuerza: opus 7,1 s, mp3 8,3 s, wav 8,4 s.
- **Y aac seguía perdiéndose entero**, porque la familia mp4 guarda su índice al
  final. Medido cortando un fichero por la mitad: sin fragmentar salen **cero**
  segundos de audio; fragmentado, **29,4 de los 30** que había dentro. Se
  fragmenta con `frag_duration` y no con `frag_keyframe`: en audio puro todos
  los fotogramas son clave y aun así ffmpeg no cerraba un solo fragmento.
- **Los contenedores de vídeo no se comportan igual**, y conviene saberlo:

  | | ¿Se abre tal cual tras el corte? | Recuperado de 10 s |
  |---|---|---|
  | mp4 | **sí** — GSR lo escribe fragmentado | 8,14 s |
  | mkv | no, sin duración | 8,13 s al rehacerlo |
  | webm | no, sin duración | 8,13 s al rehacerlo |

- Así que **la aplicación repara sola** lo que quedó a medias. Al abrirla, si
  detecta una grabación sin cerrar lo dice y ofrece arreglarla; rehace el
  contenedor copiando los flujos, sin recodificar y sin perder calidad. Orden
  nueva `easy-screen-recorder-cli reparar`. El original no se toca hasta que el
  arreglado existe y tiene duración.
- `flac` es el único que queda a medias: conserva **todo** el audio —verificado,
  se decodifica entero— pero sin la duración en la cabecera.
- El arnés pasa a **ocho casos**: mata al grabador con SIGKILL y exige que lo
  grabado siga ahí y que `reparar` lo deje utilizable.

### Lo grabado sobrevive a un apagón, y la interfaz explica lo que hace (2026-09-16)

- **Un corte de luz se llevaba TODA la grabación.** Medido con un A/B limpio —10
  segundos grabados y un SIGKILL al grabador, que es lo más parecido a un apagón
  que se puede provocar a mano—: sin nada se recuperan **0 segundos**; con
  `flush_packets=1` se recuperan **8,1 de los 10**. Se pierde la cola sin cerrar,
  no la grabación entera. Ahora va siempre, y no cuesta nada: la misma grabación
  parada bien da la misma duración y el mismo tamaño con y sin la opción.
- **«Calidad» no son bitrates, y ahora lo dice.** Son valores de QP —calidad
  constante— de 35, 30, 25 y 22 (`video_codec.c:9-17` de GSR), así que el tamaño
  lo decide lo que pase en pantalla. Medido en esta máquina con el escritorio
  poco movido: **1,5 / 2,4 / 3,0 / 4,6 MB por minuto**. Va en una ayuda
  emergente, con la advertencia de que sube con el movimiento.
- **Ayudas emergentes** donde hacía falta explicar algo y no cabía: qué es el
  modo repetición, qué diferencia hay entre una pista y dos, qué hace superponer
  la cámara y por qué la vista previa se apaga al grabar, y qué significa
  codificar solo al cambiar la pantalla.
- **El botón «Cambiar…» estaba lejos de la ruta.** El hueco sobrante se estiraba
  entre los dos y el botón acababa pegado al borde derecho, a media ventana de lo
  que cambia. Ahora el hueco va después del botón.
- **Descoordinación al maximizar.** Abrir «Avanzado» con la ventana maximizada le
  daba un ancho y un alto propios y la dejaba ni maximizada ni del tamaño pedido.
  Ahora, si manda el gestor de ventanas, no se le toca el tamaño. Verificado:
  maximizada a 1366×686, abrir «Avanzado» la deja igual.
- `docs/interno/PROMPT-COMPARATIVA.md`: un prompt para comparar la aplicación con
  lo que ya existe en Windows, escrito para que el modelo busque contraejemplos y
  no para que dé la razón.

Verificado: build sin un warning, `ctest` 12/12 (109 comprobaciones en ajustes),
`verify-recording.sh` con los siete casos en verde, `reuse lint` conforme.

### La cámara se coloca arrastrándola, y «Avanzado» en dos columnas (2026-09-16)

- **Las cuatro esquinas se quedaban cortas.** Sobre una barra de tareas, un
  panel lateral o una ventana fija en una punta, las cuatro fallan a la vez y no
  hay una quinta. GSR admite **posición libre** en porcentaje (`x=50%;y=10%`,
  convertido a píxeles contra el tamaño del vídeo), verificado grabando, así que
  ahora se arrastra donde sea.
- **Y se ve dónde va a quedar antes de grabar**: un mapa con la pantalla a su
  proporción y la cámara dentro, también a la suya, que se mueve con el ratón.
  Al lado, la vista previa con tu cara. Las dos ayudas contestan preguntas
  distintas —cómo salgo y dónde salgo— y por eso están las dos.
- La proporción de la cámara sale de su mejor modo (`1280x720@30hz` → 16:9), no
  de una suposición: un 4:3 no se coloca igual que un 16:9.
- **«Avanzado» pasa a dos columnas y la ventana se ensancha al abrirlo.** Era
  una sola columna y las últimas filas quedaban fuera de la pantalla mientras
  sobraba la mitad derecha. Tres cambios, los tres medidos con la ventana
  delante:
  - Dos columnas en vez de una.
  - **Etiquetas al lado del control, no encima**: Kirigami elegía «encima», que
    dobla el alto de cada fila; con once filas eran casi 200 px de más.
  - Sin igualar el ancho de las etiquetas entre columnas: con «Códec de audio:»
    mandando, las de la izquierda se salían por el borde y «Imágenes por
    segundo» se leía «ágenes por segundo».
  - Resultado medido: de no caber en 768 px a **828×667**, con todo visible
    hasta «Guardar en».
- Por el CLI, `--camara-esquina` se cambia por `--camara-x` y `--camara-y`, en
  porcentaje. La validación rechaza una cámara que se saldría del borde, porque
  ahí GSR la recortaría sin decir nada.

Verificado: build sin un warning, `ctest` 12/12, `verify-recording.sh` con los
siete casos en verde, `reuse lint` conforme. Y grabando por el camino de la
interfaz con la cámara al 5 %, 5 %: aparece exactamente ahí.

### Nueve funciones, y una premisa que estaba mal (2026-09-16)

El hallazgo que ordena toda la tanda: **GSR ya compone pantalla y cámara en
vivo, él solo y en un proceso.** `-w` admite varias fuentes unidas por `|`,
igual que `-a`, y cada una acepta `x`, `y`, `width`, `height`, `halign`,
`valign`, `hflip` y `vflip` detrás de un `;`. Verificado grabando con el CLI del
proyecto **sin escribir una línea de código**.

Eso tira abajo la Parte 2 entera del ROADMAP —dos procesos, dos ficheros,
composición con ffmpeg, estado `componiendo`, 0,21× del tiempo grabado— que se
había diseñado y medido sobre la premisa contraria. El diseño viejo queda en el
documento como registro, con la lección escrita: se leyó el código de GSR para
ver cómo rodearlo y no se leyó entero el manual de la opción que ya se usaba.

Lo implementado:

- **Cámara superpuesta**, con tamaño y esquina elegidos **antes** de grabar. El
  tamaño va en porcentaje del ancho, no en píxeles: un valor fijo es un cuarto
  de pantalla en 1366 y un décimo en 4K. La altura no se pasa, para que GSR
  mantenga la proporción.
- **Espejo**, activado por defecto. Sin él uno se ve al revés de como se ve en
  un espejo y no se reconoce.
- **Vista previa de la cámara** para encuadrarte antes de empezar, con
  QtMultimedia. Se apaga al arrancar la grabación, y no por capricho: **una
  cámara V4L2 admite un solo cliente**, medido (`Device or resource busy`) y
  dicho por el propio manual de GSR. Verte *durante* la grabación no es posible
  con esta arquitectura.
- **Codificar solo cuando la pantalla cambie** (`-fm content`), que se ofrece
  **solo donde funciona**: en Wayland sobre un monitor GSR lo acepta, avisa por
  stderr y lo ignora. Otro caso de «pides una cosa y recibes otra» atajado
  antes de lanzar.
- **Límite de tamaño del vídeo** (`-s`): grabar en la resolución de la pantalla
  y entregar 1080p o 720p. Verificado: pedir 800x600 sobre 1366x768 da 800x450,
  escalado con su proporción.
- **Vúmetro del micrófono** antes de grabar, porque el fallo caro es descubrir
  al reproducir que estaba mudo. Con escala en decibelios, no lineal: el micro
  de la máquina de desarrollo da 0,004 de RMS y en escala lineal la barra no se
  movía.
- **Cuenta atrás de 3 segundos**, cancelable, antes de empezar.
- **Audio de una sola aplicación** (`app:nombre` de GSR), con las que están
  sonando en ese momento añadidas al selector de audio.
- **Modo repetición** (replay buffer): guarda en memoria los últimos 30 s, 1, 5
  o 15 minutos y no escribe nada hasta que se lo pides. Orden nueva
  `easy-screen-recorder-cli guardar`, botón propio y entrada en la bandeja. El
  atajo global **guarda** en vez de parar, que es lo que uno quiere del «se me
  ha escapado eso».

Tres cosas que costaron y conviene no repetir:

- **`find_package(Qt6 COMPONENTS Multimedia)` en una llamada aparte pone
  `Qt6_FOUND` a FALSE** si el componente falta. Con eso, la interfaz entera
  dejó de compilarse **en silencio** durante varios pasos, con el binario viejo
  todavía en `build/` y los `grep -i error` saliendo limpios. Va en
  `OPTIONAL_COMPONENTS`.
- **La marca de modo repetición en la sesión.** El socket no dice de qué modo es
  la grabación, así que una ventana abierta a mitad de un replay ofrecía «Parar
  y guardar», y eso no guarda nada: tirarías el buffer creyendo que lo salvabas.
- **`--device=all` en el flatpak**, y solo por la vista previa: `dri`, `input` y
  `usb` se probaron y ninguno expone `/dev/videoN`. La grabación no lo necesita,
  porque la cámara la captura GSR desde el anfitrión.

Verificación, toda ejecutando: compilación sin un warning, `ctest` **12/12**
(106 comprobaciones en `prueba_ajustes`), `verify-recording.sh` con **siete**
casos en verde —pantalla, pistas mezcladas, pistas separadas, cámara
superpuesta, repetición, webm y audio—, `reuse lint` conforme. El menú de la
bandeja y el volcado del buffer, probados por DBus contra el flatpak instalado.
La vista previa y el vúmetro, verificados dentro del flatpak construido con el
SDK: la aplicación abre `/dev/video0` y aparece como flujo de captura en
`pactl`, con niveles que coinciden con los que mide ffmpeg por su cuenta.

### Lo que se rompía al usarla de verdad (2026-09-16)

Cuatro cosas que salieron de instalar el flatpak y usarlo, no de leer el código.

- **Al grabar, la aplicación desaparecía.** La ventana se ocultaba para no salir
  en el vídeo, y la única vía de vuelta era el icono de la bandeja, que **en el
  flatpak no llegaba a existir**: el manifiesto no pedía
  `--talk-name=org.kde.StatusNotifierWatcher` y el registro fallaba en silencio,
  con una sola línea en stderr («KDE platform plugin is loaded but SNI
  unavailable»). Había que reabrir la aplicación desde el menú de inicio para
  poder pausar.
  - El permiso entra en el manifiesto. Verificado en los dos sentidos: sin él, el
    watcher no lista ningún item nuestro; con él, lo lista y el aviso desaparece.
  - Y la ventana **se minimiza en vez de ocultarse**, así que sigue en la barra
    de tareas aunque no haya bandeja. Medido: `visibility` pasa a `Minimized` y
    `visible` sigue en `true`, que es justo por lo que la comprobación anterior
    (`!visible`) nunca la habría traído de vuelta.
  - La bandeja gana **menú**: mostrar, pausa/reanudar, parar y guardar, salir.
    Pausar sin sacar la ventana, que es lo que hacía falta. Verificado por DBus
    sobre el flatpak instalado: se pausa, se reanuda y se para desde el menú, y
    el fichero queda escrito.
- **El desplegable de fuentes decía `eDP-1` y `/dev/video0`.** Ahora dice
  «Pantalla del portátil · 1366×768» y «Cámara · Integrated Webcam HD». El
  identificador de GSR no se toca: solo cambia el texto.
  - El nombre de la cámara sale de `/sys/class/video4linux/videoN/name`, que es
    donde lo pone el driver; la clase de pantalla, del prefijo del conector DRM.
    Si no se reconoce, «Pantalla» a secas: genérico es honesto, adivinar no.
  - Las fuentes se **agrupan y ordenan**: monitores, luego lo que hay que decidir
    al empezar, luego cámaras. GSR las listaba como le salían y la cámara caía
    entre «region» y «portal».
  - El identificador solo reaparece cuando hace falta para distinguir dos
    fuentes que se llamarían igual.
- **«Preguntar al empezar (lo elige el sistema)»** decía quién decide y no QUÉ
  se decide. Pasa a «Elegir ventana o pantalla al empezar». Y «La ventana que
  tenga el foco», a «La ventana activa».
- **El audio de las dos fuentes solo se podía grabar en pistas separadas**, y
  con dos pistas casi todos los reproductores suenan solo la primera: el
  micrófono parecía no haberse grabado. Se añade «Los dos, en una sola pista»,
  la sintaxis `a|b` de GSR, y va **delante** de la opción de pistas separadas,
  que queda etiquetada «(para editar)».
  - `scripts/verify-recording.sh` lo comprueba de aquí en adelante: graba de las
    dos maneras y exige **una** pista de audio en la mezclada y **dos** en la
    separada. En verde.
  - Mezclando, `flac` desaparece del selector de códec y la validación lo
    rechaza: GSR lo cambiaría a opus por detrás (`codec_select.c:186-191`).

Y de repasar qué ofrece el desplegable de códecs, tres más:

- **`h264_software` no grababa nunca.** Sale de `--info`, que lo imprime solo
  porque la máquina tiene libx264 (`commands.c:75-76` de GSR), pero **no es un
  valor de `-k`** (`args_parser.c:20-38`): el grabador muere al arrancar con
  «-k should either be 'auto', 'h264', …». Se quita del selector. La
  codificación por CPU en GSR es `-encoder cpu`, otra opción y otra
  conversación; queda apuntada, sin implementar, en ESTADO.md.
- **El códec de vídeo y el formato se elegían por separado, y tres de las
  parejas que la interfaz permitía no grababan NADA.** Medido grabando: `h264`
  en webm, `hevc` en webm y `vp8` en mp4 arrancan, mueren al escribir la
  cabecera («Only VP8 or VP9 or AV1 video … are supported for WebM») y no dejan
  fichero. Es el fallo más caro que puede tener esto, porque se descubre cuando
  ya has grabado.
  - El selector de códec pasa a filtrarse por el formato elegido, igual que ya
    hacía el de audio, y `validar()` rechaza la pareja antes de lanzar, así que
    el CLI también queda cubierto.
  - La tabla sale de meter un flujo de cada códec en cada contenedor con ffmpeg:
    webm solo admite vp8, vp9 y av1; mp4 admite todo menos vp8; mkv todo.
  - `auto` vale siempre: GSR mira el contenedor antes de elegir. Verificado —
    `auto` en un `.webm` da vp8, no h264.
  - `scripts/verify-recording.sh` graba ahora también en webm y exige que el
    vídeo sea de los que webm acepta.
- **«Automática (recomendada)» en la calidad del audio no era un nivel de
  calidad.** Medido: es el default del codificador, y coincide con una opción
  que ya estaba en la lista — 96 kbps en opus, 128 en aac. O sea, vaguedad y una
  entrada repetida. Se quita; queda **96 kbps · voz** preseleccionada, que es
  exactamente lo que entregaba antes en opus, así que nadie recibe algo distinto
  de lo de ayer. No se añaden escalones por encima de 192: el núcleo llega a 512
  y el CLI los acepta, pero opus es transparente bastante antes y quien quiere
  más no quiere más kbps, quiere flac, que ya está en el selector de Formato.

Y un quinto, encontrado al ir a tocar el control del audio:

- **El panel «Avanzado» no cabía en la ventana.** Crecía a 32 unidades de
  rejilla fijas y en una pantalla de 1366×768 las tres últimas filas —imágenes
  por segundo, carpeta y **audio**— quedaban fuera del borde inferior, sin barra
  de desplazamiento ni nada que insinuara que seguían ahí. Ahora la ventana se
  ajusta a lo que el formulario mide, hasta donde dé la pantalla. Medido: la
  ventana converge a 711 px con el contenido entero dentro.

Verificación: compilación sin un warning, `ctest` **12/12** (148 comprobaciones
en `prueba_capacidades` y 86 en `prueba_ajustes`), `scripts/verify-recording.sh`
en verde con las tres comprobaciones nuevas —pistas mezcladas, pistas separadas
y webm—, y `reuse lint` conforme.

Lo que esta máquina graba de verdad, medido grabando 3 s con cada códec y
pasando ffprobe el 2026-09-16 (Intel, `/dev/dri/card1`, Wayland, GSR 6.0.0):
**h264**, **hevc** y **vp8**. `vp9` falla y lo dice; `av1` **cae a h264 en
silencio**, con un aviso solo en el log — ninguno de los dos los lista `--info`
aquí, así que la interfaz no los ofrece.

### Distribución propia (2026-09-16)
- `scripts/publicar-flatpak.sh`: repositorio Flatpak propio y firmado, con
  `.flatpakref`, `.flatpakrepo`, bundle y una página de instalación. Las
  actualizaciones llegan con `flatpak update`.
- Se sirve desde `flatpak.solucionesconscientes.es` en Cloudflare Pages, que ya
  está en uso para el sitio. Medido: 148 ficheros y 8,1 MB por versión, con
  4,1 MiB el fichero mayor — los límites del plan gratuito son 20.000 ficheros
  y 25 MiB, así que sobra.
- **Verificado de punta a punta**: repositorio firmado (`summary.sig`),
  instalado desde el `.flatpakref` como lo haría un usuario, el remoto queda
  configurado, `flatpak update` opera y la app arranca.
- Flathub queda para más adelante, con el motivo y el texto de su política
  escritos en `empaquetado/flathub/NOTAS.md`.

### Diseño de pantalla + webcam (2026-09-16)
- `docs/ROADMAP.md` gana la Parte 2 con el diseño medido, **sin implementar**.
- Lo medido: dos grabaciones de GSR a la vez funcionan sin contención, y la
  composición con `ffmpeg` sale a **0,17× del tiempo grabado** mezclando en CPU
  y codificando en GPU, frente a 0,48× haciéndolo todo en CPU.
- `overlay_vaapi` **no funciona** en esta GPU: «Function not implemented»
  incluso en el caso mínimo. La composición entera en GPU no es una opción aquí.
- Se descarta el plugin de GSR, que sería la vía para superponer en vivo: se
  carga dentro de su proceso y rompería el modelo dual.
- Capturas de pantalla rehechas en inglés.

### Interfaz en inglés (2026-09-16)
- **La interfaz sigue el idioma del sistema**: castellano si el sistema está en
  castellano, inglés en cualquier otro caso. 45 cadenas traducidas, con el
  catálogo empotrado en el binario.
- Y el cambio que lo hizo posible: **el texto visible deja de ser el
  identificador.** El desplegable de fuentes pasa a `{texto, valor}`; antes el
  QML decidía el modo con `currentText.indexOf("Solo audio")` y el atajo global
  comparaba igual, así que traducir habría roto el formulario en silencio.
- Las fuentes especiales dejan de enseñar su nombre interno: `portal` pasa a
  «Preguntar al empezar», `region` a «Elegir una región arrastrando» y
  `focused` a «La ventana que tenga el foco».
- Sigue en castellano lo que viene del núcleo: ~47 mensajes de error de
  `libesr` y la línea de comandos. El porqué y la vía, en `docs/ROADMAP.md`.

### Alcance y recursos, medidos (2026-09-16)
- Los README dicen dónde funciona de verdad —cualquier escritorio, Wayland y
  X11— y qué es lo único nativo de KDE.
- **Cifras de consumo medidas**, no adjetivos: CLI 0,00 s y 4 MB; detección
  completa 0,6–1,2 s y 49 MB; interfaz 1,0–1,6 s y 94 MB. En un i5-6200U de
  2015 con gráficos integrados.
- Y el aviso que va con eso: si la GPU no tiene codificador por hardware, GSR
  cae a CPU y la ventaja desaparece. `--check` dice en qué caso estás.
- `ESR_MEDIR_ARRANQUE`: el renombrado había dejado esa variable con espacios
  dentro del nombre, y `medir-arranque.sh` no la ponía, así que medir la
  interfaz se colgaba para siempre.

### Empaquetado y alcance corregidos (2026-09-16)
- **El `.deb` declara los 14 módulos QML** que necesita en tiempo de ejecución,
  Kirigami incluido. `dpkg-shlibdeps` no los ve: un `import` de QML no es
  enlazado ELF. Sin ellos el paquete se instalaba y la interfaz no arrancaba.
- **Deja de decirse «para KDE Plasma»**: funciona en cualquier escritorio y
  sobre Wayland y X11. Lo único atado a KDE es el atajo global y el icono de
  bandeja, y los dos se degradan sin ruido.

### Páginas de manual (2026-09-16)
- `easy-screen-recorder(1)` y `easy-screen-recorder-cli(1)`, instaladas por el
  CMake en `share/man/man1`. Van con el proyecto y no con el empaquetado:
  documentan el programa, no cómo se empaqueta.

### Formatos de solo audio (2026-09-15)
- **Tres formatos nuevos**: AAC en `.m4a` (compatibilidad), WAV (sin pérdida,
  para editar) y MP3 (solo compatibilidad heredada). Con Opus y FLAC, cinco.
- **Calidad del audio** elegible (96/128/192 kbps) en los formatos con
  pérdida. En FLAC y WAV el control **se esconde**, no se deshabilita: ahí ese
  ajuste no existe.
- «Lo que suena» pasa a llamarse **«Audio del sistema»** en la interfaz y en la
  ayuda del CLI.
- `--bitrate` en el CLI.
- La tabla de formatos vive en un solo sitio, y de ella salen la lista de la
  interfaz, la extensión por defecto y el codificador de ffmpeg. Antes la
  extensión era un `formato == "flac" ? "flac" : "opus"` repetido en dos
  ficheros, que con cinco formatos habría dado el fichero mal nombrado.

### Flatpak funcionando (2026-09-15)
- **La aplicación empaquetada ya graba.** Dentro de un sandbox, GSR se lanza en
  el anfitrión con `flatpak-spawn --host`. Verificado grabando: 416 s por CLI y
  2,79 s por la UI, comprobados con `ffprobe` desde fuera del sandbox.
- Dentro de un sandbox no se busca en `PATH`, solo en el anfitrión, por las dos
  vías (binario nativo o flatpak de GSR). Fuera, el comportamiento no cambia.
- El manifiesto declara `--device=dri`: sin él la interfaz se dibujaba en
  software.
- Runtime `org.kde.Platform` 6.11 confirmado construyendo y ejecutando.

### Empaquetado Debian (2026-09-15)
- `debian/` con `control`, `rules`, `changelog`, `copyright` y
  `source/format`. Construcción con `dh --buildsystem=cmake+ninja`.
- `gpu-screen-recorder` va en `Recommends`, no en `Depends`: no está en los
  repositorios y un `Depends` dejaría el paquete sin instalar.
- Contenido del paquete verificado con un `.deb` hecho a mano: los dos
  binarios y los cinco ficheros de datos en su sitio.

### Renombrado (2026-09-15)
- **Proyecto renombrado desde Capturia a Easy Screen Recorder.** El nombre es
  siempre en inglés y sin abreviar, en todos los idiomas.
- Identificador de aplicación: `org.capturia.Capturia` pasa a
  `es.solucionesconscientes.EasyScreenRecorder`.
- Binarios: la UI es `easy-screen-recorder` (antes `capturia-ui`) y la consola
  `easy-screen-recorder-cli` (antes `capturia`).
- Biblioteca: `libcapturia` pasa a `libesr`, con las cabeceras en `include/esr/`
  y el namespace `esr`. El URI de QML pasa a `es.solucionesconscientes.esr`.
- Configuración en `~/.config/easy-screen-recorder/` y sesión en
  `~/.cache/easy-screen-recorder/`. **Sin migración**: no hay usuarios todavía.
- Sin cambios de funcionalidad.

### Tandas 9 y 10 (2026-09-15)
- Selector de región propio (arrastrar elige, Esc cancela) y formatos y
  códecs en Avanzado, con el códec de audio filtrado por formato.
- Carpeta de destino elegible y con memoria (vídeos y audio por separado),
  compartida entre la UI y el CLI. Defaults: Vídeos y Música (XDG).
- Atajo global Meta+Shift+R para empezar/parar, registrado en KGlobalAccel
  por DBus y cambiable en Preferencias del sistema.
- La ventana se aparta al grabar pantalla y la bandeja solo enciende el
  punto rojo grabando.
- UI verificada como cliente X11 (xcb); borrador de manifiesto de Flathub
  con sus bloqueos documentados en empaquetado/flathub/NOTAS.md.

### Tanda 7 (2026-09-15)
- La interfaz gráfica: Qt6/QML + Kirigami, dos clics (Fuente, Grabar),
  "Avanzado" plegado con calidad, fps y qué audio. Arranque medido en
  0,73-0,89 s hasta el primer frame.
- Graba de verdad por su propio camino (verificado con ffprobe) y comparte
  estado con el CLI: abierta a mitad de una grabación enseña el reloj real.
- Bandeja con notificación al guardar, .desktop, icono y metainfo validados,
  y reglas de instalación comprobadas con un install de ensayo.

### Tanda 6 (2026-09-15)
- Investigación del post-proceso (`docs/post-proceso.md`): la telemetría del
  puntero para auto-zoom es viable en KDE y las cuatro piezas están
  ejecutadas. Decisión del titular: **pospuesto** hasta después de la UI.

### Tanda 5 (2026-09-15)
- Modo solo-audio real por ffmpeg + PipeWire: `capturia audio`, opus y flac,
  sin depender de GSR.
- Detección en paralelo: `--check` baja de 1,3 s a 0,68-0,90 s.
- El arnés verifica pantalla y audio grabando de verdad.

### Tanda 4 (2026-09-15)
- **Easy Screen Recorder graba.** `grabar`, `parar`, `pausar`, `reanudar`, `estado`,
  `fuentes`, `dispositivos`, `--check --volcado`.
- Cliente IPC propio del protocolo de GSR, con respuestas diferidas sin
  límite de tiempo.
- Validación contenedor+códec antes de lanzar: lo que GSR cambiaría por
  detrás aquí es un error a la cara.
- Verificado con ffprobe: monitor, región y cámara v4l2.

### Tanda 3 (2026-09-14)
- Verificación oficial con CMake y Ninja por primera vez; CI hermético.
- `verify-recording.sh` deja de ser verde por construcción.
- Tests para la detección de entorno y la sincronía de versión.

### Tandas 1 y 2 (2026-09-10)
- Núcleo de detección: qué sabe hacer esta máquina, sin inventar nada.
- Investigación de GSR 6.0.0 con el código delante: IPC documentado y
  ejecutado, audio-only imposible por GSR, licencias razonadas.
