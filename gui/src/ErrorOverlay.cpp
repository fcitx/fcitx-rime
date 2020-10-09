/***************************************************************************
 *   Copyright (C) 2011 by Dario Freddi <drf@kde.org>                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "ErrorOverlay.h"

#include <QIcon>
#include <QEvent>
#include <QVBoxLayout>

ErrorOverlay::ErrorOverlay(QWidget *baseWidget, QWidget *parent) :
    QWidget(parent ? parent : baseWidget->window()),
    m_BaseWidget(baseWidget),
    m_enable(false) {
    setVisible(false);
    // Build the UI
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setSpacing(10);

    QLabel *pixmap = new QLabel();
    pixmap->setPixmap(QIcon::fromTheme("dialog-error").pixmap(64));

    m_message = new QLabel("");

    pixmap->setAlignment(Qt::AlignHCenter);
    m_message->setAlignment(Qt::AlignHCenter);

    layout->addStretch();
    layout->addWidget(pixmap);
    layout->addWidget(m_message);
    layout->addStretch();

    setLayout(layout);

    // Draw the transparent overlay background
    QPalette p = palette();
    p.setColor(backgroundRole(), QColor(0, 0, 0, 128));
    p.setColor(foregroundRole(), Qt::white);
    setPalette(p);
    setAutoFillBackground(true);

    m_BaseWidget->installEventFilter(this);

}

ErrorOverlay::~ErrorOverlay() { }

void ErrorOverlay::enable(const char *errorMessage) {

    m_message->setText(errorMessage);

    if(!m_BaseWidget) {
        return;
    }

    // reparent to the current top level widget of the base widget if needed
    // needed eg. in dock widgets
    if (parentWidget() != m_BaseWidget->window()) {
        setParent(m_BaseWidget->window());
    }

    show();

    // follow position changes
    const QPoint topLevelPos = m_BaseWidget->mapTo(window(), QPoint(0, 0));
    const QPoint parentPos = parentWidget()->mapFrom(window(), topLevelPos);
    move(parentPos);

    m_enable = true;
    // TODO: hide/scale icon if we don't have enough space
    resize(m_BaseWidget->size());
}
