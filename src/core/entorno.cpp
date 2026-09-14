#include "capturia/entorno.hpp"

#include <cstdlib>
#include <filesystem>
#include <thread>

#include "capturia/proceso.hpp"

namespace capturia {
namespace {

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
    h.origen = "PATH";

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

// Las dos raices donde flatpak instala por defecto. Un flatpak en una raiz a
// medida no se encuentra; se prefiere eso a lanzar «flatpak info» en cada
// arranque, porque --check tiene que ser barato.
bool flatpak_tiene_la_app() {
    if (!localizar("flatpak")) return false;

    std::vector<std::filesystem::path> raices{"/var/lib/flatpak"};
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg != nullptr && xdg[0] != '\0') {
        raices.emplace_back(std::filesystem::path(xdg) / "flatpak");
    } else if (const char* hogar = std::getenv("HOME"); hogar != nullptr && hogar[0] != '\0') {
        raices.emplace_back(std::filesystem::path(hogar) / ".local/share/flatpak");
    }

    for (const auto& raiz : raices) {
        std::error_code ec;
        if (std::filesystem::is_directory(raiz / "app" / std::string(kAppFlatpakGsr), ec)) return true;
    }
    return false;
}

// Cuanto se le deja a una sonda antes de darla por colgada.
//
// Medido en esta maquina con el flatpak ya caliente: entre 217 y 746 ms por
// sonda, frente a 104 ms de un binario nativo. El margen extra del flatpak es
// para el primer arranque, cuando su runtime todavia no esta en cache.
int limite_sonda_ms(const Invocacion& inv) {
    return inv.origen == "flatpak" ? 15000 : 5000;
}

}  // namespace

std::string Invocacion::linea(const std::vector<std::string>& args) const {
    std::string texto = programa;
    for (const auto& a : prefijo) texto += " " + a;
    for (const auto& a : args) texto += " " + a;
    return texto;
}

std::optional<Invocacion> localizar_gsr(const std::string& binario) {
    if (const auto ruta = localizar(binario)) {
        return Invocacion{binario, {}, "PATH", *ruta};
    }
    if (flatpak_tiene_la_app()) {
        return Invocacion{"flatpak",
                          {"run", "--command=" + binario, std::string(kAppFlatpakGsr)},
                          "flatpak",
                          std::string(kAppFlatpakGsr)};
    }
    return std::nullopt;
}

std::vector<Sonda> sondas() {
    // Verificadas contra GSR 6.0.0: salen de args_parser.c y las imprime
    // commands.c; gsr-cli responde a --help.
    //
    // --list-capture-options se quito en la Tanda 4: --info trae la seccion
    // capture_options identica (commands.c:268-269) y sondear dos veces
    // costaba entre 217 y 746 ms de arranque con el flatpak. El parser sigue
    // entendiendo esa lista si un volcado viejo la trae.
    return {
        {"gpu-screen-recorder", {"--version"}},
        {"gpu-screen-recorder", {"--info"}},
        {"gpu-screen-recorder", {"--list-audio-devices"}},
        {"gpu-screen-recorder", {"--list-application-audio"}},
        {"gsr-cli", {"--help"}},
    };
}

std::string volcar() {
    // Las sondas corren a la vez y el volcado se arma en orden al final. En
    // serie el coste era la suma (1,3 s medidos con el flatpak, ESTADO.md);
    // en paralelo es el de la sonda mas lenta. Cada una es un proceso
    // independiente que solo escribe en su propio hueco, asi que no comparten
    // nada que proteger.
    const auto lista = sondas();
    struct Hueco {
        std::string linea;
        ResultadoProceso r;
    };
    std::vector<Hueco> huecos(lista.size());

    std::vector<std::thread> hilos;
    hilos.reserve(lista.size());
    for (std::size_t i = 0; i < lista.size(); ++i) {
        hilos.emplace_back([&lista, &huecos, i] {
            const Sonda& s = lista[i];
            Hueco& h = huecos[i];
            const auto inv = localizar_gsr(s.programa);
            if (inv) {
                h.linea = inv->linea(s.args);
                std::vector<std::string> args = inv->prefijo;
                args.insert(args.end(), s.args.begin(), s.args.end());
                h.r = ejecutar(inv->programa, args, limite_sonda_ms(*inv));
            } else {
                h.linea = s.programa;
                for (const auto& a : s.args) h.linea += " " + a;
                h.r.motivo = "no se encuentra «" + s.programa + "» ni en PATH ni como flatpak";
            }
        });
    }
    for (auto& hilo : hilos) hilo.join();

    std::string texto;
    for (const auto& h : huecos) {
        // Sin invocacion no hay nada que ejecutar, pero el bloque se escribe
        // igual: un hueco silencioso en el volcado seria peor que un error.
        texto += std::string(kMarcaComando) + h.linea + "\n";
        texto += std::string(kMarcaCodigo) + std::to_string(h.r.ejecutado ? h.r.codigo : -1) + "\n";
        if (!h.r.ejecutado) {
            texto += "(no ejecutado) " + h.r.motivo + "\n";
        } else {
            texto += h.r.salida;
            if (!texto.empty() && texto.back() != '\n') texto += "\n";
            if (h.r.expirado) texto += "(expirado) " + h.r.motivo + "\n";
        }
        texto += std::string(kMarcaFin) + "\n";
    }
    return texto;
}

bool Entorno::graba_pantalla() const { return gsr.presente && capacidades.gsr_respondio; }

bool Entorno::graba_audio_solo() const {
    // Depende de ffmpeg y va a seguir dependiendo. GSR exige fuente de video
    // siempre: `-w` esta declarado obligatorio en args_parser.c:536 y sin el se
    // niega a arrancar. Comprobado tambien ejecutandolo. Ver
    // docs/gsr-audio-only.md.
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

    // Una version por debajo de la minima no es un aviso: nuestra capa entera
    // se apoya en el IPC de gsr-cli y no se ha comprobado en versiones
    // anteriores. Ver version_minima_gsr().
    if (e.gsr.presente && e.gsr.version) {
        if (const auto minima = version_minima_gsr(); minima && *e.gsr.version < *minima) {
            e.carencias.push_back(Carencia{
                "gpu-screen-recorder " + e.gsr.version->texto() + ": por debajo de la minima " +
                    minima->texto(),
                "toda la capa de control usa el IPC de gsr-cli, sin comprobar en versiones anteriores",
                true});
        }
    }

    if (!e.gsr_cli.presente) {
        e.carencias.push_back(Carencia{
            "gsr-cli: " + (e.gsr_cli.diagnostico.empty() ? std::string("ausente") : e.gsr_cli.diagnostico),
            "no bloquea: libcapturia habla el IPC directamente, que ya esta documentado en "
            "docs/gsr-ipc.md. Pero sin gsr-cli no se puede contrastar ese protocolo a mano",
            false});
    }

    if (e.ffmpeg.evaluada && !e.ffmpeg.presente) {
        e.carencias.push_back(Carencia{
            "ffmpeg: " + (e.ffmpeg.diagnostico.empty() ? std::string("ausente") : e.ffmpeg.diagnostico),
            "sin el no hay modo audio-only: GSR exige fuente de video y no puede darlo el "
            "(docs/gsr-audio-only.md)",
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

    // Rehacer la parte de GSR con acceso real al entorno: el volcado dice si
    // respondio, pero solo aqui sabemos por que via esta instalado.
    const auto inv_gsr = localizar_gsr("gpu-screen-recorder");
    e.gsr.presente = inv_gsr.has_value() && e.capacidades.gsr_respondio;
    if (inv_gsr) {
        e.gsr.ruta = inv_gsr->ruta;
        e.gsr.origen = inv_gsr->origen;
        if (!e.capacidades.gsr_respondio) e.gsr.diagnostico = "instalado pero no respondio a --version";
    } else {
        e.gsr.diagnostico = "no se encuentra ni en PATH ni como flatpak";
    }

    const auto inv_cli = localizar_gsr("gsr-cli");
    e.gsr_cli.presente = inv_cli.has_value();
    if (inv_cli) {
        e.gsr_cli.ruta = inv_cli->ruta;
        e.gsr_cli.origen = inv_cli->origen;
    } else {
        e.gsr_cli.diagnostico = "no se encuentra ni en PATH ni como flatpak";
    }

    e.ffmpeg = sondear_binario("ffmpeg", {"-version"});
    e.ffprobe = sondear_binario("ffprobe", {"-version"});

    anotar_carencias(e);
    return e;
}

}  // namespace capturia
