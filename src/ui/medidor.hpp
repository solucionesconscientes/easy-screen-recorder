// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

// El vumetro del microfono, para saber ANTES de grabar que el micro capta algo.
//
// Existe porque el fallo mas caro de una grabacion con voz es descubrir al
// reproducirla que el microfono estaba mudo, en otro dispositivo o con el
// volumen a cero. Media hora de trabajo tirada, y ningun aviso durante.
//
// Solo se compila si hay Qt6Multimedia. Sin el, `disponible()` devuelve false y
// la interfaz no enseña el control: mejor no ofrecerlo que ofrecer una barra
// que nunca se mueve.
class Medidor : public QObject {
    Q_OBJECT

public:
    explicit Medidor(QObject* padre = nullptr);
    ~Medidor() override;

    // Si esta compilacion puede medir de verdad.
    static bool disponible();

    // Empieza o para de escuchar el micro. Se para SIEMPRE al grabar: no por
    // exclusividad —el audio si admite varios clientes, al contrario que la
    // camara— sino porque medir mientras se graba no sirve de nada y gasta.
    void escuchar(bool si);

    // 0 a 1, el nivel eficaz (RMS) del ultimo trozo leido.
    qreal nivel() const { return nivel_; }

signals:
    void nivelCambiado();

private:
    void ponerNivel(qreal n);

    qreal nivel_ = 0.0;
    struct Interno;
    Interno* interno_ = nullptr;
};
