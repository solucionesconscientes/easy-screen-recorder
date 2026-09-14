#include "capturia/entorno.hpp"

#include <algorithm>
#include <string>

#include "comprobar.hpp"

using namespace capturia;

namespace {

std::string fixture(const std::string& nombre) {
    return prueba::leer(std::string(CAPTURIA_DIR_FIXTURES) + "/" + nombre);
}

std::size_t bloqueantes(const Entorno& e) {
    return static_cast<std::size_t>(std::count_if(
        e.carencias.begin(), e.carencias.end(), [](const Carencia& c) { return c.bloqueante; }));
}

bool alguna_carencia_contiene(const Entorno& e, std::string_view trozo) {
    return std::any_of(e.carencias.begin(), e.carencias.end(), [&](const Carencia& c) {
        return c.que.find(trozo) != std::string::npos;
    });
}

// --- Volcado real de esta maquina -------------------------------------------
// Es el caso "todo instalado y respondiendo". Si falla aqui, lo que decide que
// se le ofrece al usuario esta roto, aunque el parser de listas vaya bien.
void volcado_real() {
    const Entorno e = detectar_desde_volcado(prueba::leer(CAPTURIA_VOLCADO_REAL));

    COMPROBAR(e.gsr.evaluada);
    COMPROBAR(e.gsr.presente);
    COMPROBAR_NOTA(e.gsr.version && *e.gsr.version == (Version{6, 0, 0}),
                   e.gsr.version ? e.gsr.version->texto() : "sin version");
    COMPROBAR(e.gsr_cli.evaluada);
    COMPROBAR(e.gsr_cli.presente);

    // El volcado no cubre ffmpeg ni ffprobe: tienen que quedar sin evaluar,
    // que no es lo mismo que ausentes. Si esto cambia a "ausente", --check
    // sobre fichero acusaria a ffmpeg sin haberlo mirado.
    COMPROBAR(!e.ffmpeg.evaluada);
    COMPROBAR(!e.ffprobe.evaluada);
    COMPROBAR(e.ffmpeg.nombre == "ffmpeg");
    COMPROBAR(e.ffprobe.nombre == "ffprobe");

    COMPROBAR(e.graba_pantalla());
    // Sin evaluar ffmpeg no se afirma que el audio-only sea posible.
    COMPROBAR(!e.graba_audio_solo());

    // Y como ffmpeg quedo sin evaluar, tampoco puede aparecer como carencia.
    COMPROBAR_NOTA(e.carencias.empty(),
                   e.carencias.empty() ? "" : e.carencias.front().que);
    COMPROBAR(e.listo());
}

// --- Nada instalado ----------------------------------------------------------
void sin_nada_instalado() {
    const Entorno e = detectar_desde_volcado(fixture("sin-nada-instalado.txt"));

    COMPROBAR(e.gsr.evaluada);
    COMPROBAR(!e.gsr.presente);
    COMPROBAR_NOTA(!e.gsr.diagnostico.empty(), "ausente sin decir por que no ayuda a nadie");
    COMPROBAR(!e.gsr_cli.presente);

    COMPROBAR(!e.graba_pantalla());
    COMPROBAR(!e.graba_audio_solo());
    COMPROBAR(!e.listo());

    // GSR ausente bloquea; gsr-cli ausente avisa pero no bloquea, porque
    // libcapturia habla el IPC directamente.
    COMPROBAR_NOTA(bloqueantes(e) == 1, "bloqueantes=" + std::to_string(bloqueantes(e)));
    COMPROBAR(alguna_carencia_contiene(e, "gpu-screen-recorder"));
    COMPROBAR(alguna_carencia_contiene(e, "gsr-cli"));
    for (const Carencia& c : e.carencias) {
        COMPROBAR_NOTA(!c.consecuencia.empty(), c.que);
        if (c.que.find("gsr-cli") != std::string::npos) COMPROBAR(!c.bloqueante);
    }
}

// --- Version por debajo de la minima -----------------------------------------
// GSR responde, o sea que grabar se puede; pero nuestra capa de control no
// esta comprobada ahi, asi que listo() tiene que decir que no.
void version_antigua() {
    const Entorno e = detectar_desde_volcado(fixture("version-antigua.txt"));

    COMPROBAR(e.gsr.presente);
    COMPROBAR(e.gsr.version && *e.gsr.version == (Version{1, 2, 3}));
    COMPROBAR(e.graba_pantalla());
    COMPROBAR(!e.listo());

    COMPROBAR_NOTA(bloqueantes(e) == 1, "bloqueantes=" + std::to_string(bloqueantes(e)));
    COMPROBAR(alguna_carencia_contiene(e, "por debajo de la minima"));

    // Ese fixture no trae bloque de gsr-cli: ausente, sin bloquear.
    COMPROBAR(!e.gsr_cli.presente);
}

// --- Volcado vacio ------------------------------------------------------------
// Un fichero vacio o basura no puede colar como "entorno listo".
void volcado_vacio() {
    const Entorno e = detectar_desde_volcado("");

    COMPROBAR(!e.gsr.presente);
    COMPROBAR(!e.graba_pantalla());
    COMPROBAR(!e.listo());
    COMPROBAR(!e.capacidades.avisos.empty());
    COMPROBAR(bloqueantes(e) >= 1);
}

}  // namespace

int main() {
    volcado_real();
    sin_nada_instalado();
    version_antigua();
    volcado_vacio();
    return prueba::resumen("prueba_entorno");
}
