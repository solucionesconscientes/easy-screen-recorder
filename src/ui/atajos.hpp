// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QString>

class Controlador;

// Atajo global de grabar/parar, via el DBus de org.kde.kglobalaccel.
//
// Se habla el protocolo directamente en vez de enlazar KGlobalAccel: son dos
// llamadas y una señal, y asi no entra una dependencia de compilacion por
// eso. Las firmas estan comprobadas contra el servicio vivo de esta maquina
// (busctl introspect, 2026-09-15). Fuera de KDE el servicio no existe y esta
// clase simplemente no hace nada: el atajo es azucar, no una funcion basica.
class AtajosGlobales : public QObject {
    Q_OBJECT

public:
    explicit AtajosGlobales(Controlador* controlador, QObject* padre = nullptr);

    // Si se pueden recibir pulsaciones: hay servicio y la señal esta conectada.
    bool registrado() const { return registrado_; }

    // El atajo que KDE tiene puesto AHORA para cada accion, ya en texto
    // («Meta+Shift+R»). Se pregunta en vez de dar por hecho el que pedimos: el
    // usuario puede haberlo cambiado en Preferencias del sistema, y anunciar el
    // nuestro cuando el suyo es otro es peor que no anunciar nada.
    QString atajoGrabar() const { return atajo_grabar_; }
    QString atajoPausa() const { return atajo_pausa_; }

private slots:
    void alPulsar(const QString& componente, const QString& accion, qlonglong instante);

private:
    Controlador* controlador_;
    bool registrado_ = false;
    QString atajo_grabar_;
    QString atajo_pausa_;
};
