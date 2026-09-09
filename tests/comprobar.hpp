#pragma once

// Arnes de test minimo. Sin dependencias externas a proposito: el proyecto no
// añade una libreria de tests para comprobar cuatro invariantes.

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace prueba {

inline int total = 0;
inline int fallos = 0;

inline void registrar(bool ok, const char* expresion, const char* fichero, int linea,
                      const std::string& nota = {}) {
    ++total;
    if (ok) return;
    ++fallos;
    std::fprintf(stderr, "FALLO %s:%d  %s%s%s\n", fichero, linea, expresion,
                 nota.empty() ? "" : "  -> ", nota.c_str());
}

inline int resumen(const char* nombre) {
    std::fprintf(stderr, "%s: %d comprobaciones, %d fallos\n", nombre, total, fallos);
    return fallos == 0 ? 0 : 1;
}

inline std::string leer(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) {
        std::fprintf(stderr, "no se puede abrir el fixture %s\n", ruta.c_str());
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

}  // namespace prueba

#define COMPROBAR(expr) prueba::registrar((expr), #expr, __FILE__, __LINE__)
#define COMPROBAR_NOTA(expr, nota) prueba::registrar((expr), #expr, __FILE__, __LINE__, (nota))
