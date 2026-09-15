// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

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

    bool registrado() const { return registrado_; }

private slots:
    void alPulsar(const QString& componente, const QString& accion, qlonglong instante);

private:
    Controlador* controlador_;
    bool registrado_ = false;
};
