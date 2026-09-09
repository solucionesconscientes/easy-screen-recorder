#include "capturia/entorno.hpp"

#include "capturia/proceso.hpp"

namespace capturia {
namespace {

std::string linea_comando(const Sonda& s) {
    std::string linea = s.programa;
    for (const auto& a : s.args) linea += " " + a;
    return linea;
}

Herramienta sondear_binario(const std::string& nombre, const std::vector<std::string>& args) {
    Herramienta h;
    h.nombre = nombre;
    h.evaluada = true;

    const auto ruta = localizar(nombre);
    if (!ruta) {
        h.diagnostico = "no se encuentra en PATH";
        return h;
    }
    h.presente = true;
    h.ruta = *ruta;

    const auto r = ejecutar(nombre, args);
    if (!r.ejecutado) {
        h.diagnostico = r.motivo;
        return h;
    }
    if (r.expirado) {
        h.diagnostico = r.motivo;
        return h;
    }
    h.version = Version::desde_texto(r.salida);
    if (!h.version) h.diagnostico = "responde pero no se reconoce su version";
    return h;
}

}  // namespace

std::vector<Sonda> sondas() {
    // Estas seis son las que pide la Tanda 1 y las que genera
    // scripts/volcar-capacidades.sh. Que existan como opciones de GSR esta
    // tomado del enunciado, no verificado contra su codigo (ESTADO.md, B1).
    return {
        {"gpu-screen-recorder", {"--version"}},
        {"gpu-screen-recorder", {"--info"}},
        {"gpu-screen-recorder", {"--list-capture-options"}},
        {"gpu-screen-recorder", {"--list-audio-devices"}},
        {"gpu-screen-recorder", {"--list-application-audio"}},
        {"gsr-cli", {"--help"}},
    };
}

std::string volcar() {
    std::string texto;
    for (const auto& s : sondas()) {
        const auto r = ejecutar(s.programa, s.args);
        texto += std::string(kMarcaComando) + linea_comando(s) + "\n";
        texto += std::string(kMarcaCodigo) + std::to_string(r.ejecutado ? r.codigo : -1) + "\n";
        if (!r.ejecutado) {
            texto += "(no ejecutado) " + r.motivo + "\n";
        } else {
            texto += r.salida;
            if (!texto.empty() && texto.back() != '\n') texto += "\n";
            if (r.expirado) texto += "(expirado) " + r.motivo + "\n";
        }
        texto += std::string(kMarcaFin) + "\n";
    }
    return texto;
}

bool Entorno::graba_pantalla() const { return gsr.presente && capacidades.gsr_respondio; }

bool Entorno::graba_audio_solo() const {
    // Por ahora depende de ffmpeg. Si la investigacion de docs/gsr-audio-only.md
    // concluye que GSR graba audio sin video, esta condicion cambia.
    return ffmpeg.presente;
}

bool Entorno::listo() const {
    for (const auto& c : carencias) {
        if (c.bloqueante) return false;
    }
    return true;
}

namespace {

void anotar_carencias(Entorno& e) {
    if (!e.graba_pantalla()) {
        std::string que = "gpu-screen-recorder";
        if (!e.gsr.evaluada) {
            que += ": no evaluado";
        } else if (!e.gsr.presente) {
            que += ": " + (e.gsr.diagnostico.empty() ? std::string("ausente") : e.gsr.diagnostico);
        } else {
            que += ": presente en " + e.gsr.ruta + " pero no respondio a --version";
        }
        e.carencias.push_back(Carencia{que, "sin el no hay grabacion de pantalla", true});
    }

    if (!e.gsr_cli.presente) {
        e.carencias.push_back(Carencia{
            "gsr-cli: " + (e.gsr_cli.diagnostico.empty() ? std::string("ausente") : e.gsr_cli.diagnostico),
            "no bloquea: libcapturia hablara el IPC directamente. Pero sin gsr-cli no se puede "
            "contrastar a mano ese protocolo, que sigue sin documentar",
            false});
    }

    if (e.ffmpeg.evaluada && !e.ffmpeg.presente) {
        e.carencias.push_back(Carencia{
            "ffmpeg: " + (e.ffmpeg.diagnostico.empty() ? std::string("ausente") : e.ffmpeg.diagnostico),
            "sin el no hay backend propio de audio-only. Si GSR resulta grabar audio sin video, "
            "deja de hacer falta",
            false});
    }

    if (e.ffprobe.evaluada && !e.ffprobe.presente) {
        e.carencias.push_back(Carencia{
            "ffprobe: " + (e.ffprobe.diagnostico.empty() ? std::string("ausente") : e.ffprobe.diagnostico),
            "sin el, scripts/verify-recording.sh no puede comprobar lo que se graba",
            false});
    }
}

}  // namespace

Entorno detectar_desde_volcado(std::string_view volcado) {
    Entorno e;
    e.capacidades = interpretar_volcado(volcado);

    e.gsr.nombre = "gpu-screen-recorder";
    e.gsr.evaluada = true;
    e.gsr.presente = e.capacidades.gsr_respondio;
    e.gsr.version = e.capacidades.version_gsr;
    if (!e.gsr.presente) e.gsr.diagnostico = "el volcado no trae una respuesta valida a --version";

    e.gsr_cli.nombre = "gsr-cli";
    e.gsr_cli.evaluada = true;
    e.gsr_cli.presente = e.capacidades.gsr_cli_respondio;
    if (!e.gsr_cli.presente) e.gsr_cli.diagnostico = "el volcado no trae una respuesta valida";

    // El volcado no cubre ffmpeg: quedan sin evaluar a proposito.
    e.ffmpeg.nombre = "ffmpeg";
    e.ffprobe.nombre = "ffprobe";

    anotar_carencias(e);
    return e;
}

Entorno detectar() {
    Entorno e = detectar_desde_volcado(volcar());
    e.carencias.clear();

    // Rehacer la parte de GSR con acceso real al PATH: el volcado dice si
    // respondio, pero solo aqui sabemos si el binario existe siquiera.
    const auto ruta_gsr = localizar("gpu-screen-recorder");
    e.gsr.presente = ruta_gsr.has_value() && e.capacidades.gsr_respondio;
    if (ruta_gsr) {
        e.gsr.ruta = *ruta_gsr;
        if (!e.capacidades.gsr_respondio) e.gsr.diagnostico = "instalado pero no respondio a --version";
    } else {
        e.gsr.diagnostico = "no se encuentra en PATH";
    }

    const auto ruta_cli = localizar("gsr-cli");
    e.gsr_cli.presente = ruta_cli.has_value();
    if (ruta_cli) {
        e.gsr_cli.ruta = *ruta_cli;
    } else {
        e.gsr_cli.diagnostico = "no se encuentra en PATH";
    }

    e.ffmpeg = sondear_binario("ffmpeg", {"-version"});
    e.ffprobe = sondear_binario("ffprobe", {"-version"});

    anotar_carencias(e);
    return e;
}

}  // namespace capturia
