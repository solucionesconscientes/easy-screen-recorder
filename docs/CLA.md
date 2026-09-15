# Contributor License Agreement — Easy Screen Recorder

Titular: **Dalmau Romaní (Soluciones Conscientes)**.

> **TODO — pendiente de pegar el texto legal.**
>
> Pegar aquí el **Individual CLA** generado en
> [contributoragreements.org](https://contributoragreements.org/), con estas
> opciones:
>
> - Variante: **concesión de licencia** (*license grant*), no cesión de
>   copyright. El contribuidor **conserva** su copyright.
> - *Outbound*: **cualquier licencia** (*any licence*). Es lo que habilita el
>   modelo dual: permite al titular redistribuir la contribución tanto bajo
>   GPL-3.0-or-later como bajo una licencia comercial.
> - Jurisdicción y datos del titular: los de Soluciones Conscientes.
>
> Si además va a haber contribuciones de empresas, generar también el **Entity
> CLA** con las mismas opciones y añadirlo debajo.

No se redacta texto legal a mano aquí: se usa una plantilla estándar y revisada.
Mientras este fichero siga con el TODO, **no se acepta ninguna contribución
externa**, porque no habría nada que firmar.

## Por qué concesión y no cesión

Pedir la cesión del copyright espanta a colaboradores y no hace falta: con una
concesión de licencia amplia y *outbound* abierto, el titular puede relicenciar
la contribución sin que el colaborador pierda nada de lo suyo. Es lo que hacen
KDE, Qt y la mayoría de proyectos con modelo dual.

## Cómo se firmará

**No se monta nada hasta que llegue el primer pull request.** Hoy no hay
colaboradores externos, así que la infraestructura no protegería de nada y el
día que haga falta se monta en veinte minutos, con una persona real al otro lado
que lo justifique.

Cuando llegue, la vía es **[CLA Assistant Lite][lite]**, que es una GitHub
Action, no un servicio externo:

```yaml
# .github/workflows/cla.yml
- uses: contributor-assistant/github-action@v2
  with:
    path-to-signatures: 'signatures/cla.json'
    path-to-document: 'https://github.com/solucionesconscientes/easy-screen-recorder/blob/main/docs/CLA.md'
```

Comenta en cada PR, bloquea la fusión hasta la firma, y **el registro de firmas
vive en este repositorio** (`signatures/cla.json`), no en un tercero. Se ahorra
el Gist y se ahorra que otra empresa guarde los datos de quien colabora.

[lite]: https://github.com/contributor-assistant/github-action

## Lo que NO sirve aquí, aunque sea más cómodo

**El DCO** —esa línea `Signed-off-by:` que pone `git commit -s`— es muchísimo
más simple y hay quien lo recomienda como sustituto de un CLA. **No lo es para
este proyecto.**

El DCO certifica que quien envía el código tiene derecho a enviarlo bajo la
licencia del proyecto. **No concede el derecho a relicenciarlo.** Con solo DCO no
se puede vender una licencia comercial que incluya código de un tercero, así que
rompería el modelo dual en silencio: el repositorio seguiría pareciendo correcto
y la opción comercial estaría perdida en los ficheros que ese tercero tocó.

Si alguna vez se abandona el modelo dual, el DCO pasa a ser la opción correcta y
la más barata. Mientras el modelo dual esté vivo, hace falta un CLA.
