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

#include "kmarkdownviewpage.h"

#include <QWebEngineSettings>
#include <QWebEnginePage>
#include <QDesktopServices>

KMarkdownViewPage::KMarkdownViewPage(QWebEngineProfile* profile, QObject* parent)
    : QWebEnginePage(profile, parent)
{
    settings()->setAttribute(WebSettings::JavascriptEnabled, true);
    settings()->setAttribute(WebSettings::PluginsEnabled, false);
    settings()->setAttribute(WebSettings::LocalContentCanAccessRemoteUrls, false);
    settings()->setAttribute(WebSettings::LocalContentCanAccessFileUrls, true);
}

KMarkdownViewPage::~KMarkdownViewPage() = default;

bool KMarkdownViewPage::acceptNavigationRequest(const QUrl& url,
                                                NavigationType type,
                                                bool isMainFrame)
{
    Q_UNUSED(isMainFrame);

    // Open links
    if(type == QWebEnginePage::NavigationTypeLinkClicked) {
        QDesktopServices::openUrl(url);
        return false;
    }

    // Allow file://
    if (url.scheme() == QLatin1Literal("file"))
        return true;

    emit openUrlRequested(url);
    return false;
}
