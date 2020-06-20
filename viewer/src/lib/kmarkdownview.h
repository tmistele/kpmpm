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

#ifndef KMARKDOWNVIEW_H
#define KMARKDOWNVIEW_H

#include <kmarkdownview_export.h>
#include <kmarkdownview-config.h>

// Qt headers
#include <QWebEngineView>
#include <QWebEnginePage>
#include <QAction>

#include <QFile>

class KMarkdownViewPage;
class KAbstractMarkdownSourceDocument;
class KMarkdownHtmlView;
class QUrl;

class KMARKDOWNVIEW_EXPORT KMarkdownView : public
    QWebEngineView
{
    Q_OBJECT

public:
    typedef QWebEnginePage WebPage;

public:
    explicit KMarkdownView(KAbstractMarkdownSourceDocument* sourceDocument, QWidget* parent = nullptr);
    ~KMarkdownView() override;

public:
#ifdef DEBUG_WEB
    void inspectElement();
#endif
    void copySelection();
    void copyLinkUrl();
    void saveLinkAs();
    void selectAllText();
    bool isCopyTextEnabled() const;
    bool isSelectAllEnabled() const;
    QPoint scrollPosition() const;
    void setScrollPosition(int x, int y);
    void renderPage(QPainter* painter, const QRect& clip);
    void findText(const QString& text, WebPage::FindFlags findFlags = {});

Q_SIGNALS:
    void copyTextEnabledChanged(bool enabled);
    void selectAllEnabledChanged(bool enabled);
    void openUrlRequested(const QUrl& url);
    void contextMenuRequested(const QPoint& globalPos, const QUrl& linkUrl, const QString& linkText,
                              bool hasSelection, bool forcesNewWindow);
    void linkHovered(const QString& url);
    void renderingDone();

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void doLoadFinished(bool ok);
    void initDone();
    void textChanged(const QString& text);
    void pmpmUpdateText();
    bool pmpmTryInit();

private:
    // manual lifetime management here needed for NOT USE_QTWEBKIT, also fine in other case
    // we need to delete the page before the QWebEngineProfile object passed to it in the constructor
    // which itself is handled by this class' QObject child lifetime management
    QScopedPointer<KMarkdownViewPage> const m_viewPage;

    KMarkdownHtmlView* const m_htmlView;
    KAbstractMarkdownSourceDocument* const m_sourceDocument;

    QFile* m_pmpmPipe;
    QString m_pmpmClientPath;
    QString m_pmpmClientPathRevealjs;
    QString m_pmpmWebsocketPort;
    bool m_pmpmRevealjs;
    bool m_pmpmInitDone;
    bool m_pmpmDirty;
};

#ifdef DEBUG_WEB
inline void KMarkdownView::inspectElement()
{
    triggerPageAction(WebPage::InspectElement);
}
#endif

inline void KMarkdownView::copySelection()
{
    triggerPageAction(WebPage::Copy);
}

inline void KMarkdownView::copyLinkUrl()
{
    triggerPageAction(WebPage::CopyLinkToClipboard);
}

inline void KMarkdownView::saveLinkAs()
{
    triggerPageAction(WebPage::DownloadLinkToDisk);
}

inline void KMarkdownView::selectAllText()
{
    triggerPageAction(WebPage::SelectAll);
}

inline bool KMarkdownView::isCopyTextEnabled() const
{
    return pageAction(WebPage::Copy)->isEnabled();
}

inline bool KMarkdownView::isSelectAllEnabled() const
{
    return pageAction(WebPage::SelectAll)->isEnabled();
}

#endif
