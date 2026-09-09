#include "capturia/proceso.hpp"

#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace capturia {

std::optional<std::string> localizar(const std::string& programa) {
    if (programa.find('/') != std::string::npos) {
        if (::access(programa.c_str(), X_OK) == 0) return programa;
        return std::nullopt;
    }

    const char* path = std::getenv("PATH");
    if (path == nullptr) return std::nullopt;

    std::string_view resto{path};
    while (!resto.empty()) {
        const std::size_t corte = resto.find(':');
        const std::string_view dir = resto.substr(0, corte);
        resto = (corte == std::string_view::npos) ? std::string_view{} : resto.substr(corte + 1);
        if (dir.empty()) continue;

        std::filesystem::path candidato = std::filesystem::path(std::string(dir)) / programa;
        if (::access(candidato.c_str(), X_OK) == 0) {
            std::error_code ec;
            if (std::filesystem::is_regular_file(candidato, ec)) return candidato.string();
        }
    }
    return std::nullopt;
}

ResultadoProceso ejecutar(const std::string& programa,
                          const std::vector<std::string>& args,
                          int limite_ms) {
    ResultadoProceso r;

    const auto ruta = localizar(programa);
    if (!ruta) {
        r.motivo = "no se encuentra «" + programa + "» en PATH";
        return r;
    }

    // argv se arma antes del fork: entre fork y exec solo son seguras las
    // llamadas async-signal-safe, y reservar memoria no lo es.
    std::vector<char*> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char*>(ruta->c_str()));
    for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    int tubo[2];
    if (::pipe(tubo) != 0) {
        r.motivo = std::string("pipe: ") + std::strerror(errno);
        return r;
    }

    const pid_t hijo = ::fork();
    if (hijo < 0) {
        ::close(tubo[0]);
        ::close(tubo[1]);
        r.motivo = std::string("fork: ") + std::strerror(errno);
        return r;
    }

    if (hijo == 0) {
        ::close(tubo[0]);
        ::dup2(tubo[1], STDOUT_FILENO);
        ::dup2(tubo[1], STDERR_FILENO);
        ::close(tubo[1]);
        ::execv(ruta->c_str(), argv.data());
        ::_exit(127);
    }

    ::close(tubo[1]);
    r.ejecutado = true;

    const auto tope = std::chrono::steady_clock::now() + std::chrono::milliseconds(limite_ms);
    std::array<char, 4096> buf{};
    bool matado = false;
    for (;;) {
        const auto restante = std::chrono::duration_cast<std::chrono::milliseconds>(
                                  tope - std::chrono::steady_clock::now())
                                  .count();
        if (restante <= 0) {
            matado = true;
            break;
        }
        pollfd pfd{tubo[0], POLLIN, 0};
        const int listo = ::poll(&pfd, 1, static_cast<int>(restante));
        if (listo < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (listo == 0) {
            matado = true;
            break;
        }
        const ssize_t leidos = ::read(tubo[0], buf.data(), buf.size());
        if (leidos <= 0) break;  // 0 = EOF, <0 = error
        r.salida.append(buf.data(), static_cast<std::size_t>(leidos));
    }
    ::close(tubo[0]);

    if (matado) {
        ::kill(hijo, SIGKILL);
        r.expirado = true;
        r.motivo = "«" + programa + "» no respondio en " + std::to_string(limite_ms) + " ms";
    }

    int estado = 0;
    ::waitpid(hijo, &estado, 0);
    r.codigo = WIFEXITED(estado) ? WEXITSTATUS(estado) : -1;
    return r;
}

}  // namespace capturia
