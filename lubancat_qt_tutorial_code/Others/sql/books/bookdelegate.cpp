/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "bookdelegate.h"

#include <QtWidgets>

BookDelegate::BookDelegate(QObject *parent)
    : QSqlRelationalDelegate(parent), star(QPixmap(":images/star.png"))
{
}

void BookDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const
{
    if (index.column() != 5) {
        QStyleOptionViewItem opt = option;
        // Since we draw the grid ourselves:
        opt.rect.adjust(0, 0, -1, -1);
        QSqlRelationalDelegate::paint(painter, opt, index);
    } else {
        const QAbstractItemModel *model = index.model();
        QPalette::ColorGroup cg = (option.state & QStyle::State_Enabled) ?
            (option.state & QStyle::State_Active) ?
                        QPalette::Normal :
                        QPalette::Inactive :
                        QPalette::Disabled;

        if (option.state & QStyle::State_Selected)
            painter->fillRect(
                        option.rect,
                        option.palette.color(cg, QPalette::Highlight));

        int rating = model->data(index, Qt::DisplayRole).toInt();
        int width = star.width();
        int height = star.height();
        int x = option.rect.x();
        int y = option.rect.y() + (option.rect.height() / 2) - (height / 2);
        for (int i = 0; i < rating; ++i) {
            painter->drawPixmap(x, y, star);
            x += width;
        }
        // Since we draw the grid ourselves:
        drawFocus(painter, option, option.rect.adjusted(0, 0, -1, -1));
    }

    QPen pen = painter->pen();
    painter->setPen(option.palette.color(QPalette::Mid));
    painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight());
    painter->drawLine(option.rect.topRight(), option.rect.bottomRight());
    painter->setPen(pen);
}

QSize BookDelegate::sizeHint(const QStyleOptionViewItem &option,
                                 const QModelIndex &index) const
{
    if (index.column() == 5)
        return QSize(5 * star.width(), star.height()) + QSize(1, 1);
    // Since we draw the grid ourselves:
    return QSqlRelationalDelegate::sizeHint(option, index) + QSize(1, 1);
}

bool BookDelegate::editorEvent(QEvent *event, QAbstractItemModel *model,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index)
{
    if (index.column() != 5)
        return QSqlRelationalDelegate::editorEvent(event, model, option, index);

    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        int stars = qBound(0, int(0.7 + qreal(mouseEvent->pos().x()
            - option.rect.x()) / star.width()), 5);
        model->setData(index, QVariant(stars));
        // So that the selection can change:
        return false;
    }

    return true;
}

QWidget *BookDelegate::createEditor(QWidget *parent,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    if (index.column() != 4)
        return QSqlRelationalDelegate::createEditor(parent, option, index);

    // For editing the year, return a spinbox with a range from -1000 to 2100.
    QSpinBox *sb = new QSpinBox(parent);
    sb->setFrame(false);
    sb->setMaximum(2100);
    sb->setMinimum(-1000);

    return sb;
}
