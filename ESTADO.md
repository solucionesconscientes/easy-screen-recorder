# Estado del proyecto

Fecha: 2026-09-14. Última tanda ejecutada: **Tanda 3, completa.**

## Resumen en una línea

**El bloqueo B3 está cerrado**: cmake y ninja instalados, y la verificación
oficial (`cmake --build` + `ctest`) pasada por primera vez en la historia del
proyecto, con la infraestructura de test tapada donde tenía agujeros.

## Hecho y verificado

Cada línea se ha comprobado ejecutando un comando, el 2026-09-14 en la máquina
de desarrollo.

| Entregable | Estado | Cómo se comprobó |
|---|---|---|
| 1. cmake 4.2.3 y ninja 1.13.2 instalados | hecho | los instaló el titular con apt; `cmake --version` y `ninja --version` responden |
| 2. Verificación oficial | hecho | `cmake -S . -B build -G Ninja` + `cmake --build build`: 15/15 sin un warning. `ctest`: **7/7 en verde, 1,52 s** |
| 3. `scripts/verify-recording.sh` arreglado | hecho | sus dos fallos corregidos, ver abajo. Las 4 ramas probadas ejecutándolas |
| 4. `tests/prueba_entorno.cpp` | hecho | 38 comprobaciones, 0 fallos. Cubre lo que decide qué se ofrece al usuario |
| 5. Test de sincronía de versión | hecho | en `prueba_version` (ahora 22). Probado en negativo: con un `9.9.9` falso, falla |
| 6. CI en `.github/workflows/ci.yml` | hecho | escrito; hermético sin GSR. **Sin ejecutar en GitHub todavía**: se verá en el primer push |

Totales de test: `prueba_version` 22, `prueba_proceso` 17,
`prueba_capacidades` 92, `prueba_entorno` 38, más los 3 tests del CLI.

## Los dos fallos de verify-recording.sh, corregidos

| Fallo | Qué pasaba | Corrección |
|---|---|---|
| Solo miraba el PATH | En esta máquina decía "gpu-screen-recorder no está en PATH" con GSR funcionando como flatpak. El mismo diagnóstico falso que ya se corrigió en `localizar_gsr()` en la Tanda 2 | `gsr_disponible()` resuelve PATH y flatpak, igual que `entorno.cpp` y `volcar-capacidades.sh` |
| Salía siempre con 0 | Verde por construcción: la regla "verify-recording.sh en verde" no garantizaba nada | Ahora falla: sin binario compilado (1), con orden `grabar` y herramientas ausentes (1), y con orden `grabar` sin comprobaciones escritas (1, a propósito: obliga a la Tanda 4 a escribirlas). Solo es PENDIENTE con 0 mientras el CLI no anuncie `grabar` |

Las 4 ramas se probaron de verdad, incluida la de herramientas ausentes con un
PATH mínimo sin flatpak ni ffprobe, y la de `grabar` anunciado con un binario
falso.

## Qué cubre prueba_entorno

`detectar_desde_volcado()`, `anotar_carencias()`, `listo()`,
`graba_pantalla()` y `graba_audio_solo()` no tenían ni una comprobación, y son
las funciones que deciden qué se le ofrece al usuario. Casos:

- **Volcado real**: todo presente, cero carencias, LISTO. Y ffmpeg/ffprobe
  quedan **sin evaluar**, que no es lo mismo que ausentes.
- **Nada instalado**: GSR bloquea, gsr-cli avisa sin bloquear.
- **Versión antigua** (1.2.3 < 6.0.0): graba_pantalla() dice que sí pero
  listo() dice que no. Es la distinción que importa.
- **Volcado vacío**: no cuela como entorno listo.

## Sin hacer

| Qué | Por qué |
|---|---|
| Todo lo de la Tanda 4: parser de `--info`, cliente IPC, proceso asíncrono, orden `grabar` | Es la tanda siguiente. Ver ENCARGO.md |
| Comprobaciones reales de grabación en verify-recording.sh | Se escriben junto a la orden `grabar`. El script falla a propósito si `grabar` aparece sin ellas |
| Ver el CI en verde en GitHub | Falta un push. El YAML está escrito y el build local que replica es el que acaba de pasar |
| Licencia de Capturia | Decisión del titular. Ver `docs/LICENSING.md` |

## Bloqueos

- **B1** (acceso a GSR): cerrado en la Tanda 2.
- **B2** (versión mínima): cerrado, 6.0.0.
- **B3** (cmake/ninja): **CERRADO el 2026-09-14.** Los instaló el titular.
- Abiertos: ninguno.

## Dependencias

Sin cambios: `libcapturia` sigue en C++20 y POSIX pelados, tests sin framework.
cmake y ninja son herramientas de build, no dependencias del binario.

## Lo primero de la Tanda 4

1. Parser de `--info` con fixtures, distinguiendo hardware de `h264_software`
   (`commands.c:75-76` lo imprime solo porque existe libx264).
2. Decidir JSON para el IPC: parser mínimo propio o dependencia justificada.
3. Cliente IPC según `docs/gsr-ipc.md`, con el socket fuera de `/tmp`.
4. Gestión asíncrona del proceso grabador y máquina de estados.
5. Orden `capturia grabar` y `parar`, y las comprobaciones reales del arnés.
