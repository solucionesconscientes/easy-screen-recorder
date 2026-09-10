#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "capturia/entorno.hpp"
#include "capturia/version.hpp"

namespace {

void uso() {
    std::printf(
        "capturia %s\n"
        "\n"
        "Uso:\n"
        "  capturia --version   version de capturia\n"
        "  capturia --check     comprueba el entorno y dice que falta\n"
        "\n"
        "Todavia no graba nada. Ver ESTADO.md.\n",
        std::string(capturia::kVersionCapturia).c_str());
}

std::string rellenar(std::string s, std::size_t ancho) {
    while (s.size() < ancho) s.push_back(' ');
    return s;
}

void imprimir_herramienta(const capturia::Herramienta& h) {
    std::string estado;
    if (!h.evaluada) {
        estado = "sin mirar";
    } else if (!h.presente) {
        estado = "ausente";
    } else {
        estado = "presente";
    }

    std::string cola;
    if (h.presente && h.version) {
        cola = h.version->texto();
        if (!h.ruta.empty()) cola += "  " + h.ruta;
    } else if (h.presente && !h.ruta.empty()) {
        cola = h.ruta;
    }
    // Por que via se encontro. Importa: un GSR en flatpak no ve el /tmp del
    // sistema, asi que el socket y el fichero de salida tienen que ir a una
    // ruta compartida (docs/gsr-ipc.md).
    if (h.presente && !h.origen.empty() && h.origen != "PATH") {
        cola += "  [" + h.origen + "]";
    }
    if (!h.diagnostico.empty()) {
        if (!cola.empty()) cola += "  ";
        cola += h.diagnostico;
    }

    std::printf("  %s%s%s\n", rellenar(h.nombre, 22).c_str(), rellenar(estado, 11).c_str(),
                cola.c_str());
}

int comprobar() {
    const capturia::Entorno e = capturia::detectar();

    std::printf("capturia %s: comprobacion del entorno\n\n",
                std::string(capturia::kVersionCapturia).c_str());

    std::printf("Herramientas\n");
    imprimir_herramienta(e.gsr);
    imprimir_herramienta(e.gsr_cli);
    imprimir_herramienta(e.ffmpeg);
    imprimir_herramienta(e.ffprobe);

    std::printf("\nVersion de gpu-screen-recorder\n");
    if (e.capacidades.version_gsr) {
        std::printf("  detectada: %s\n", e.capacidades.version_gsr->texto().c_str());
    } else {
        std::printf("  detectada: ninguna\n");
    }
    if (const auto minima = capturia::version_minima_gsr()) {
        std::printf("  minima soportada: %s\n", minima->texto().c_str());
    } else {
        std::printf("  minima soportada: sin decidir\n");
    }

    std::printf("\nCapacidades leidas del volcado\n");
    std::printf("  %s%zu\n", rellenar("fuentes de captura", 24).c_str(),
                e.capacidades.fuentes_captura.size());
    std::printf("  %s%zu\n", rellenar("dispositivos de audio", 24).c_str(),
                e.capacidades.dispositivos_audio.size());
    std::printf("  %s%zu\n", rellenar("audio por aplicacion", 24).c_str(),
                e.capacidades.audio_por_aplicacion.size());

    // Si GSR no esta instalado, los avisos del parser solo repiten esa misma
    // noticia con otras palabras. El diagnostico util es el de mas abajo.
    if (e.gsr.presente && !e.capacidades.avisos.empty()) {
        std::printf("\nEl parser no supo interpretar\n");
        for (const auto& a : e.capacidades.avisos) std::printf("  - %s\n", a.c_str());
    }

    if (e.carencias.empty()) {
        std::printf("\nNo falta nada.\n");
    } else {
        std::printf("\nQue falta\n");
        for (const auto& c : e.carencias) {
            std::printf("  [%s] %s\n", c.bloqueante ? "bloqueante" : "aviso    ", c.que.c_str());
            std::printf("      %s\n", c.consecuencia.c_str());
        }
    }

    std::printf("\nGrabacion de pantalla: %s\n", e.graba_pantalla() ? "posible" : "NO posible");
    // GSR no graba audio sin video: exige -w siempre (docs/gsr-audio-only.md).
    // El modo audio-only sale de ffmpeg o no sale.
    std::printf("Audio sin video:       %s\n",
                e.graba_audio_solo() ? "posible con ffmpeg (backend sin escribir)"
                                     : "NO posible: falta ffmpeg");
    std::printf("\nResultado: %s\n", e.listo() ? "LISTO" : "NO LISTO");

    return e.listo() ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    const std::vector<std::string_view> args(argv + 1, argv + argc);

    if (args.empty()) {
        uso();
        return 0;
    }
    if (args.size() == 1) {
        if (args[0] == "--version" || args[0] == "-v") {
            std::printf("capturia %s\n", std::string(capturia::kVersionCapturia).c_str());
            return 0;
        }
        if (args[0] == "--check") {
            return comprobar();
        }
        if (args[0] == "--help" || args[0] == "-h") {
            uso();
            return 0;
        }
    }

    std::fprintf(stderr, "capturia: no entiendo esa orden\n\n");
    uso();
    return 2;
}
