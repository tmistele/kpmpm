/*
 * Copyright (C) 2017 by Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kmarkdownhtmlview.h"


KMarkdownHtmlView::KMarkdownHtmlView(QObject *parent)
    : QObject(parent)
{}


void KMarkdownHtmlView::setScrollPosition(int x, int y)
{
    m_scrollPosition = QPoint(x, y);

    Q_EMIT scrollPositionChanged(m_scrollPosition);
}

void KMarkdownHtmlView::emitRenderingDone()
{
    Q_EMIT renderingDone();
}

void KMarkdownHtmlView::emitInitDone()
{
    Q_EMIT initDone();
}

void KMarkdownHtmlView::emitAuthFail()
{
    Q_EMIT authFail();
}
