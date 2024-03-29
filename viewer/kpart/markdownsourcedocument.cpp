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

#include "markdownsourcedocument.h"

void MarkdownSourceDocument::setText(const QString& text)
{
    m_text = text;

    Q_EMIT textChanged(m_text);
}

QString MarkdownSourceDocument::text() const
{
    return m_text;
}

void MarkdownSourceDocument::setUrl(const QUrl& url)
{
    const bool changed = url != m_url;
    m_url = url;
    if(changed)
        Q_EMIT urlChanged(m_url);
}

void MarkdownSourceDocument::setRevealjs(const bool revealjs)
{
    m_revealjs = revealjs;
}

QUrl MarkdownSourceDocument::url() const
{
    return m_url;
}

bool MarkdownSourceDocument::revealjs() const
{
    return m_revealjs;
}
