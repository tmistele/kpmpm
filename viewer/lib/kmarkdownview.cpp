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

#include "kmarkdownview.h"

#include "kmarkdownviewpage.h"
#include "kabstractmarkdownsourcedocument.h"
#include "kmarkdownhtmlview.h"

// Qt
#include <QWebChannel>
#include <QWebEngineContextMenuData>
#include <QWebEngineProfile>
#include <QContextMenuEvent>

#include <QFile>
#include <QIODevice>

KMarkdownView::KMarkdownView(KAbstractMarkdownSourceDocument* sourceDocument, QWidget* parent)
    : QWebEngineView(parent)
    , m_viewPage(new KMarkdownViewPage(new QWebEngineProfile(this), this))
    , m_htmlView(new KMarkdownHtmlView(this))
    , m_sourceDocument(sourceDocument)
{
    setPage(m_viewPage.data());
    connect(m_viewPage.data(), &KMarkdownViewPage::openUrlRequested, this, &KMarkdownView::openUrlRequested);
    connect(m_viewPage.data(), &KMarkdownViewPage::linkHovered, this, &KMarkdownView::linkHovered);

    auto copyAction = pageAction(WebPage::Copy);
    connect(copyAction, &QAction::changed, this, [&] {
        Q_EMIT copyTextEnabledChanged(pageAction(WebPage::Copy)->isEnabled());
    });
    auto selectAllAction = pageAction(WebPage::SelectAll);
    connect(selectAllAction, &QAction::changed, this, [&] {
        Q_EMIT selectAllEnabledChanged(pageAction(WebPage::SelectAll)->isEnabled());
    });

    QWebChannel* channel = new QWebChannel(this);
    channel->registerObject(QStringLiteral("sourceTextObject"), m_sourceDocument);
    channel->registerObject(QStringLiteral("viewObject"), m_htmlView);
    m_viewPage->setWebChannel(channel);

    connect(m_htmlView, &KMarkdownHtmlView::renderingDone, this, &KMarkdownView::renderingDone);

    connect(m_sourceDocument, &KAbstractMarkdownSourceDocument::textChanged, this, &KMarkdownView::textChanged);

    connect(this, &KMarkdownView::loadFinished, this, &KMarkdownView::doLoadFinished);
    connect(m_htmlView, &KMarkdownHtmlView::initDone, this, &KMarkdownView::initDone);
    connect(m_htmlView, &KMarkdownHtmlView::authFail, this, &KMarkdownView::authFail);
    connect(m_htmlView, &KMarkdownHtmlView::scrollPositionChanged, this, &KMarkdownView::scrollPositionChanged);

    m_pmpmRevealjs = m_sourceDocument->revealjs();

    // Set dirty flag so that content is pushed on successful init
    m_pmpmDirty = true;
    pmpmTryInit();
}

KMarkdownView::~KMarkdownView() = default;

QPoint KMarkdownView::scrollPosition() const
{
    return m_htmlView->scrollPosition();
}

void KMarkdownView::setScrollPosition(int x, int y)
{
    m_htmlView->requestSetScrollPosition(x, y);
}

void KMarkdownView::reloadImagesWithoutCache()
{
    m_htmlView->requestReloadImagesWithoutCache();
}

void KMarkdownView::renderPage(QPainter* painter, const QRect& clip)
{
    Q_UNUSED(painter);
    Q_UNUSED(clip);
}

void KMarkdownView::findText(const QString& text, WebPage::FindFlags findFlags)
{
    page()->findText(text, findFlags);
}

void KMarkdownView::contextMenuEvent(QContextMenuEvent* event)
{
    QWebEngineContextMenuData result = page()->contextMenuData();

    // default menu arguments
    bool forcesNewWindow = false;
    bool hasSelection = false;

    if (!result.linkUrl().isValid()) {
        hasSelection = !result.selectedText().isEmpty();
    } else {
    }

    Q_EMIT contextMenuRequested(event->globalPos(),
                              result.linkUrl(), result.linkText(),
                              hasSelection, forcesNewWindow);

    event->accept();
}

bool KMarkdownView::pmpmTryInit()
{
    m_pmpmPipe = nullptr;
    m_pmpmInitDone = false;

    QString runtimeDir = QString::fromLocal8Bit(qgetenv("XDG_RUNTIME_DIR"));

    QFile tmp(runtimeDir + QLatin1Literal("/pmpm/client_path"));
    if(!tmp.open(QIODevice::ReadOnly))
        return false;
    m_pmpmClientPath = QString::fromLocal8Bit(tmp.readAll());

    QFile tmp2(runtimeDir + QLatin1Literal("/pmpm/client_path_revealjs"));
    if(!tmp2.open(QIODevice::ReadOnly))
        return false;
    m_pmpmClientPathRevealjs = QString::fromLocal8Bit(tmp2.readAll());

    QFile tmp3(runtimeDir + QLatin1Literal("/pmpm/websocket_port"));
    if(!tmp3.open(QIODevice::ReadOnly))
        return false;
    m_pmpmWebsocketPort = QString::fromLocal8Bit(tmp3.readAll());

    QFile tmp4(runtimeDir + QLatin1Literal("/pmpm/websocket_secret"));
    if(!tmp4.open(QIODevice::ReadOnly))
        // Probably we first need to socket-activate pmpm. Will re-authenticate with correct secret later
        m_pmpmWebsocketSecret = QLatin1Literal("__doesnt_matter__");
    else
        m_pmpmWebsocketSecret = QString::fromLocal8Bit(tmp4.readAll());

    setUrl(QUrl(QLatin1Literal("file://") + (m_pmpmRevealjs ? m_pmpmClientPathRevealjs : m_pmpmClientPath)
        + QLatin1Literal("?port=") + m_pmpmWebsocketPort
        + QLatin1Literal("&secret=") + m_pmpmWebsocketSecret));

    m_pmpmPipe = new QFile(runtimeDir + QLatin1Literal("/pmpm/pipe"));
    // Unbuffered is needed so that content is directy flushed to pmpm on each write
    // ExistingOnly is needed so that we don't accidentally place a normal file where the pipe should be, if we run before pmpm is started
    if(!m_pmpmPipe->open(QIODevice::WriteOnly | QIODevice::Unbuffered | QIODevice::ExistingOnly))
        return false;

    return true;
}

void KMarkdownView::doLoadFinished(bool ok)
{
    if(!ok)
        return;

    m_viewPage->runJavaScript(QLatin1Literal("loadScript('qrc:kmarkdownwebview/init.js')"));
}

void KMarkdownView::initDone()
{
    m_pmpmInitDone = true;
    if(m_pmpmDirty)
        pmpmUpdateText();
}

void KMarkdownView::authFail()
{
    // After pmpm restart, authentication will fail
    // Detect this by checking if secret changed. If so, reconnect with new secret
    QString runtimeDir = QString::fromLocal8Bit(qgetenv("XDG_RUNTIME_DIR"));
    QFile tmp4(runtimeDir + QLatin1Literal("/pmpm/websocket_secret"));
    if(!tmp4.open(QIODevice::ReadOnly))
        return;
    QString newSecret = QString::fromLocal8Bit(tmp4.readAll());
    if(m_pmpmWebsocketSecret != newSecret) {
        m_pmpmWebsocketSecret = newSecret;
        m_htmlView->reconnectWithNewSecret(newSecret);
    }
}

void KMarkdownView::textChanged(const QString& text)
{
    if(text.isEmpty())
        return;

    m_pmpmDirty = true;

    const bool newRevealjs = m_sourceDocument->revealjs();
    const bool revealjsChanged = m_pmpmRevealjs != newRevealjs;
    m_pmpmRevealjs = newRevealjs;

    // If revealjs changed, we must change the url
    // In this case, don't update directly. This will happen automatically on initDone
    if(revealjsChanged) {
        m_pmpmInitDone = false;
        setUrl(QUrl(QLatin1Literal("file://") + (m_pmpmRevealjs ? m_pmpmClientPathRevealjs : m_pmpmClientPath)
            + QLatin1Literal("?port=") + m_pmpmWebsocketPort
            + QLatin1Literal("&secret=") + m_pmpmWebsocketSecret));
        return;
    }

    if(m_pmpmInitDone) {
        // If everything is set up and ready, directly send content
        pmpmUpdateText();
    } else if(!m_pmpmPipe || !m_pmpmPipe->isOpen()) {
        // If pipe is missing, try to initialize it
        pmpmTryInit();
    } else {
        // We just need to wait for initialization to finish
    }
}

void KMarkdownView::pmpmUpdateText()
{
    m_pmpmDirty = false;

    QByteArray data = m_sourceDocument->text().toUtf8();
    data.append('\0');

    if(m_pmpmRevealjs) {
        data.prepend(QByteArray::fromRawData("<!-- revealjs -->", 17));
    }

    const QUrl url = m_sourceDocument->url();
    if (url.scheme() == QLatin1Literal("file")) {
        // Prepend file name for local files so that relative paths for images etc. work
        data.prepend((
            QLatin1Literal("<!-- filepath:") +
            url.path() +
            QLatin1Literal(" -->\n")
        ).toUtf8());
    }

    m_pmpmPipe->write(data);
}
