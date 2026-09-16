// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "medidor.hpp"

#ifdef ESR_CON_MULTIMEDIA
#include <QAudioFormat>
#include <QAudioSource>
#include <QIODevice>
#include <QMediaDevices>

#include <cmath>
#endif

#ifdef ESR_CON_MULTIMEDIA

struct Medidor::Interno {
    QAudioSource* fuente = nullptr;
    QIODevice* flujo = nullptr;
};

bool Medidor::disponible() { return true; }

Medidor::Medidor(QObject* padre) : QObject(padre), interno_(new Interno) {}

Medidor::~Medidor() {
    escuchar(false);
    delete interno_;
}

void Medidor::escuchar(bool si) {
    if (!si) {
        if (interno_->fuente != nullptr) {
            interno_->fuente->stop();
            interno_->fuente->deleteLater();
            interno_->fuente = nullptr;
            interno_->flujo = nullptr;
        }
        ponerNivel(0.0);
        return;
    }
    if (interno_->fuente != nullptr) return;

    const QAudioDevice entrada = QMediaDevices::defaultAudioInput();
    if (entrada.isNull()) return;

    // Mono a 16 kHz y 16 bits: para un vumetro sobra, y cuanto menos se lea
    // menos cuesta. No es la grabacion, es solo saber si entra señal.
    QAudioFormat formato;
    formato.setSampleRate(16000);
    formato.setChannelCount(1);
    formato.setSampleFormat(QAudioFormat::Int16);
    if (!entrada.isFormatSupported(formato)) formato = entrada.preferredFormat();

    interno_->fuente = new QAudioSource(entrada, formato, this);
    interno_->flujo = interno_->fuente->start();
    if (interno_->flujo == nullptr) {
        escuchar(false);
        return;
    }
    connect(interno_->flujo, &QIODevice::readyRead, this, [this, formato] {
        const QByteArray trozo = interno_->flujo->readAll();
        if (trozo.isEmpty()) return;
        if (formato.sampleFormat() != QAudioFormat::Int16) {
            // Un formato que no se sabe leer no se interpreta a ojo: se calla.
            return;
        }
        const auto* muestras = reinterpret_cast<const qint16*>(trozo.constData());
        const int n = static_cast<int>(trozo.size() / sizeof(qint16));
        if (n <= 0) return;
        // RMS y no el pico: el pico salta con cualquier chasquido y no dice si
        // una voz esta entrando bien.
        double suma = 0.0;
        for (int i = 0; i < n; ++i) {
            const double v = muestras[i] / 32768.0;
            suma += v * v;
        }
        ponerNivel(std::sqrt(suma / n));
    });
}

#else  // sin Qt6Multimedia

struct Medidor::Interno {};

bool Medidor::disponible() { return false; }

Medidor::Medidor(QObject* padre) : QObject(padre) {}
Medidor::~Medidor() = default;
void Medidor::escuchar(bool) {}

#endif

void Medidor::ponerNivel(qreal n) {
    // El umbral solo evita repintar por nada, y tiene que ser MUY pequeño.
    //
    // Estaba en 0,005 y con eso la barra se quedaba clavada: el microfono de
    // esta maquina da 0,004 de RMS incluso con un tono sonando (medido, -47,9 dB
    // con astats), asi que ninguna variacion real superaba el umbral y el nivel
    // no se movia nunca. Justo el «parece rota» que este control existe para
    // evitar.
    if (std::abs(n - nivel_) < 0.0005) return;
    nivel_ = n;
    emit nivelCambiado();
}
