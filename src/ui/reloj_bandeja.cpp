// SPDX-FileCopyrightText: 2026 Dalmau Romaní (Soluciones Conscientes)
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "reloj_bandeja.hpp"

#include <QColor>
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>
#include <QRect>

namespace esr::ui {
namespace {

QString dosCifras(int n) { return QStringLiteral("%1").arg(n, 2, 10, QLatin1Char('0')); }

}  // namespace

QString tiempoEscrito(int segundos) {
    const int horas = segundos / 3600;
    const QString resto =
        dosCifras((segundos / 60) % 60) + QStringLiteral(":") + dosCifras(segundos % 60);
    return horas > 0 ? QString::number(horas) + QStringLiteral(":") + resto : resto;
}

std::pair<QString, QString> lineasReloj(int segundos) {
    if (segundos < 3600) return {dosCifras(segundos / 60), dosCifras(segundos % 60)};
    return {QString::number(segundos / 3600) + QStringLiteral("h"),
            dosCifras((segundos / 60) % 60)};
}

QIcon iconoReloj(int segundos, bool pausado) {
    const auto lineas = lineasReloj(segundos);
    // Fondo de color y cifras blancas, y NO el color de texto del tema: un
    // pixmap que pintamos nosotros el panel no lo recolorea, asi que con el
    // color del tema las cifras desaparecen en cuanto el panel va al reves del
    // esquema de color. El rojo se lee igual sobre un panel claro y sobre uno
    // oscuro, y encima dice lo que esta pasando; en pausa se apaga a gris, que
    // es la otra cosa que puede estar pasando. Es el rojo de Breeze que ya usa
    // el selector de region.
    const QColor fondo = pausado ? QColor(0x7f, 0x8c, 0x8d) : QColor(0xda, 0x44, 0x53);
    QIcon icono;
    // Varios tamaños porque el panel pide el que le cuadra: 22 es el del panel
    // por defecto y 64 cubre los gruesos y el escalado. Con un solo tamaño, el
    // host escalaria un pixmap con texto dentro, que es lo que se ve sucio.
    for (int lado : {22, 24, 32, 48, 64}) {
        QPixmap lienzo(lado, lado);
        lienzo.fill(Qt::transparent);
        QPainter p(&lienzo);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);
        p.setBrush(fondo);
        p.drawRoundedRect(QRectF(0, 0, lado, lado), lado / 5.0, lado / 5.0);
        // Fuente de ancho fijo: con una proporcional «11» ocupa menos que «00» y
        // el reloj parece moverse solo al cambiar de numero.
        QFont fuente = QFontDatabase::systemFont(QFontDatabase::FixedFont);
        fuente.setBold(true);
        // El tamaño se busca en vez de calcularse: se parte de la mitad del alto
        // y se baja hasta que la linea mas larga cabe a lo ancho. Asi «1h» y
        // «12h» entran sin un caso aparte.
        int px = lado / 2;
        for (; px > 5; --px) {
            fuente.setPixelSize(px);
            const QFontMetrics fm(fuente);
            if (fm.horizontalAdvance(lineas.first) <= lado - 2 &&
                fm.horizontalAdvance(lineas.second) <= lado - 2) {
                break;
            }
        }
        fuente.setPixelSize(px);
        p.setFont(fuente);
        p.setPen(Qt::white);
        p.drawText(QRect(0, 0, lado, lado / 2), Qt::AlignCenter, lineas.first);
        p.drawText(QRect(0, lado / 2, lado, lado - lado / 2), Qt::AlignCenter, lineas.second);
        p.end();
        icono.addPixmap(lienzo);
    }
    return icono;
}

}  // namespace esr::ui
