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

#include "markdownpart.h"

#include "markdownsourcedocument.h"
#include "markdownbrowserextension.h"
#include "searchtoolbar.h"

#include <kmarkdownview.h>

// KF
#include <KActionCollection>
#include <KStandardAction>
#include <KLocalizedString>
#include <KStringHandler>
#include <KFileItem>

// Qt
#include <QFile>
#include <QTextStream>
#include <QMimeDatabase>
#include <QBuffer>
#include <QShortcut>
#include <QDesktopServices>
#include <QMimeData>
#include <QClipboard>
#include <QApplication>
#include <QMenu>
#include <QVBoxLayout>
#include <QWebEngineHistory>


MarkdownPart::MarkdownPart(QWidget* parentWidget, QObject* parent, const KPluginMetaData& metaData, Modus modus)
    : KParts::ReadOnlyPart(parent)
    , m_sourceDocument(new MarkdownSourceDocument(this))
    , m_widget(new KMarkdownView(m_sourceDocument, parentWidget))
    , m_searchToolBar(new SearchToolBar(m_widget, parentWidget))
    , m_browserExtension(new MarkdownBrowserExtension(this))
{
    // set component data
    // the first arg must be the same as the subdirectory into which the part's rc file is installed
    setMetaData(metaData);

    // set internal UI
    auto mainLayout = new QVBoxLayout;
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    mainLayout->addWidget(m_widget);

    m_searchToolBar->hide();
    mainLayout->addWidget(m_searchToolBar);

    auto mainWidget = new QWidget(parentWidget);
    mainWidget->setLayout(mainLayout);
    setWidget(mainWidget);

    // set KXMLUI resource file
    setXMLFile(QStringLiteral("kmarkdownwebviewpartui.rc"));

    if (modus == BrowserViewModus) {
        connect(m_widget, &KMarkdownView::openUrlRequested,
                m_browserExtension, &MarkdownBrowserExtension::requestOpenUrl);
        connect(m_widget, &KMarkdownView::copyTextEnabledChanged,
                m_browserExtension, &MarkdownBrowserExtension::updateCopyAction);
//         connect(m_widget, &KMarkdownView::linkMiddleOrCtrlClicked,
//                 this, &MarkdownBrowserExtension::requestOpenUrlNewWindow);
        connect(m_widget, &KMarkdownView::contextMenuRequested,
                m_browserExtension, &MarkdownBrowserExtension::requestContextMenu);
    } else {
        connect(m_widget, &KMarkdownView::openUrlRequested,
                this, &MarkdownPart::handleOpenUrlRequest);
        connect(m_widget, &KMarkdownView::contextMenuRequested,
                this, &MarkdownPart::requestContextMenu);
    }
    connect(m_widget, &KMarkdownView::linkHovered,
            this, &MarkdownPart::showHoveredLink);
    connect(m_widget, &KMarkdownView::scrollPositionChanged,
            this, &MarkdownPart::scrollPositionChanged);

    setupActions(modus);
}

MarkdownPart::~MarkdownPart() = default;


void MarkdownPart::setupActions(Modus modus)
{
    // only register to xmlgui if not in browser mode
    QObject* copySelectionActionParent = (modus == BrowserViewModus) ? static_cast<QObject*>(this) : static_cast<QObject*>(actionCollection());
    m_copySelectionAction = KStandardAction::copy(copySelectionActionParent);
    m_copySelectionAction->setText(i18n("&Copy Text"));
    m_copySelectionAction->setEnabled(m_widget->isCopyTextEnabled());
    connect(m_widget, &KMarkdownView::copyTextEnabledChanged,
            m_copySelectionAction, &QAction::setEnabled);
    connect(m_copySelectionAction, &QAction::triggered, this, &MarkdownPart::copySelection);

    m_selectAllAction = KStandardAction::selectAll(this, &MarkdownPart::selectAll, actionCollection());
    m_selectAllAction->setEnabled(m_widget->isSelectAllEnabled());
    connect(m_widget, &KMarkdownView::selectAllEnabledChanged,
            m_selectAllAction, &QAction::setEnabled);
    m_selectAllAction->setShortcutContext(Qt::WidgetShortcut);
    m_widget->addAction(m_selectAllAction);

    m_searchAction = KStandardAction::find(m_searchToolBar, &SearchToolBar::startSearch, actionCollection());
    m_searchAction->setEnabled(false);
    m_widget->addAction(m_searchAction);

    m_searchNextAction = KStandardAction::findNext(m_searchToolBar, &SearchToolBar::searchNext, actionCollection());
    m_searchNextAction->setEnabled(false);
    m_widget->addAction(m_searchNextAction);

    m_searchPreviousAction = KStandardAction::findPrev(m_searchToolBar, &SearchToolBar::searchPrevious, actionCollection());
    m_searchPreviousAction->setEnabled(false);
    m_widget->addAction(m_searchPreviousAction);

    auto closeFindBarShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), widget());
    closeFindBarShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(closeFindBarShortcut, &QShortcut::activated, m_searchToolBar, &SearchToolBar::hide);

    m_backAction = KStandardAction::back(this, &MarkdownPart::back, actionCollection());
    m_widget->addAction(m_backAction);

    m_forwardAction = KStandardAction::forward(this, &MarkdownPart::forward, actionCollection());
    m_widget->addAction(m_forwardAction);
}

bool MarkdownPart::openFile()
{
    QFile file(localFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    prepareViewStateRestoringOnReload();

    QTextStream stream(&file);
    QString text = stream.readAll();

    file.close();

    disconnect(m_widget, &KMarkdownView::renderingDone, this, &MarkdownPart::restoreScrollPosition);
    // Restore scroll position only on document change
    if(url() != m_sourceDocument->url())
        connect(m_widget, &KMarkdownView::renderingDone, this, &MarkdownPart::restoreScrollPosition);

    m_sourceDocument->setUrl(url());
    m_sourceDocument->setText(text);
    m_sourceDocument->setRevealjs(false);
    m_searchAction->setEnabled(true);
    m_searchNextAction->setEnabled(true);
    m_searchPreviousAction->setEnabled(true);

    return true;
}

bool MarkdownPart::doOpenStream(const QString& mimeType)
{
    auto mime = QMimeDatabase().mimeTypeForName(mimeType);
    if (!mime.inherits(QStringLiteral("text/markdown"))) {
        return false;
    }

    m_streamedData.clear();
    m_sourceDocument->setUrl(QUrl(QLatin1Literal("")));
    m_sourceDocument->setText(QString());
    return true;
}

bool MarkdownPart::doWriteStream(const QByteArray& data)
{
    m_streamedData.append(data);
    return true;
}

bool MarkdownPart::doCloseStream()
{
    QBuffer buffer(&m_streamedData);

    if (!buffer.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m_streamedData.clear();
        return false;
    }

    prepareViewStateRestoringOnReload();

    QTextStream stream(&buffer);
    QString text = stream.readAll();
    disconnect(m_widget, &KMarkdownView::renderingDone, this, &MarkdownPart::restoreScrollPosition);
    // Restore scroll position only on document change
    if(url() != m_sourceDocument->url())
        connect(m_widget, &KMarkdownView::renderingDone, this, &MarkdownPart::restoreScrollPosition);

    m_sourceDocument->setUrl(url());
    m_sourceDocument->setText(text);
    m_searchAction->setEnabled(true);
    m_searchNextAction->setEnabled(true);
    m_searchPreviousAction->setEnabled(true);

    m_streamedData.clear();
    return true;
}

bool MarkdownPart::closeUrl()
{
    // protect against repeated call if already closed
    const auto currentUrl = url();
    if (currentUrl.isValid()) {
        m_previousScrollPosition = m_widget->scrollPosition();
        m_previousUrl = currentUrl;
    }

    m_sourceDocument->setUrl(QUrl(QLatin1Literal("")));
    m_sourceDocument->setText(QString());
    m_sourceDocument->setRevealjs(false);
    m_searchAction->setEnabled(false);
    m_searchNextAction->setEnabled(false);
    m_searchPreviousAction->setEnabled(false);
    m_streamedData.clear();

    return ReadOnlyPart::closeUrl();
}

void MarkdownPart::pmpmDirectOpen(const QString& text, const QUrl& newurl, bool revealjs, bool flushCache)
{
    prepareViewStateRestoringOnReload();

    disconnect(m_widget, &KMarkdownView::renderingDone, this, &MarkdownPart::restoreScrollPosition);
    // Restore scroll position only on document change
    if(newurl != m_sourceDocument->url())
        connect(m_widget, &KMarkdownView::renderingDone, this, &MarkdownPart::restoreScrollPosition);

    disconnect(m_widget, &KMarkdownView::renderingDone, this, &MarkdownPart::reloadImagesWithoutCache);
    // Reload images without cache after renderingDone
    if(flushCache)
        connect(m_widget, &KMarkdownView::renderingDone, this, &MarkdownPart::reloadImagesWithoutCache);

    setUrl(newurl);

    m_sourceDocument->setUrl(newurl);
    m_sourceDocument->setRevealjs(revealjs);
    // setText must come last, since this triggers rerender
    m_sourceDocument->setText(text);
    m_searchAction->setEnabled(true);
    m_searchNextAction->setEnabled(true);
    m_searchPreviousAction->setEnabled(true);
}

void MarkdownPart::prepareViewStateRestoringOnReload()
{
    if (url() == m_previousUrl) {
        KParts::OpenUrlArguments args(arguments());
        args.setXOffset(m_previousScrollPosition.x());
        args.setYOffset(m_previousScrollPosition.y());
        setArguments(args);
    }
}

void MarkdownPart::restoreScrollPosition()
{
    // After document switch + rendering done, clear history to prevent back/forward...
    m_widget->history()->clear();

    // ... and restore scroll position
    const KParts::OpenUrlArguments args(arguments());
    m_widget->setScrollPosition(args.xOffset(), args.yOffset());

    disconnect(m_widget, &KMarkdownView::renderingDone, this, &MarkdownPart::restoreScrollPosition);
}

void MarkdownPart::reloadImagesWithoutCache()
{
    m_widget->reloadImagesWithoutCache();
    disconnect(m_widget, &KMarkdownView::renderingDone, this, &MarkdownPart::reloadImagesWithoutCache);
}

void MarkdownPart::handleOpenUrlRequest(const QUrl& url)
{
    QDesktopServices::openUrl(url);
}

void MarkdownPart::requestContextMenu(const QPoint& globalPos,
                                      const QUrl& linkUrl, const QString& linkText,
                                      bool hasSelection, bool forcesNewWindow)
{
    Q_UNUSED(forcesNewWindow);

    QMenu menu(m_widget);

    if (!linkUrl.isValid()) {
        if (hasSelection) {
            menu.addAction(m_copySelectionAction);
        } else {
            QWebEngineHistory* history = m_widget->history();
            m_backAction->setEnabled(history->canGoBack());
            m_forwardAction->setEnabled(history->canGoForward());

            menu.addAction(m_backAction);
            menu.addAction(m_forwardAction);
            menu.addAction(m_selectAllAction);
            if (m_searchToolBar->isHidden()) {
                menu.addAction(m_searchAction);
            }
        }
    } else {
        auto action = menu.addAction(i18n("Open Link"));
        connect(action, &QAction::triggered, this, [&] {
            handleOpenUrlRequest(linkUrl);
        });
        menu.addSeparator();

        if (linkUrl.scheme() == QLatin1String("mailto")) {
            menu.addAction(createCopyEmailAddressAction(&menu, linkUrl));
        } else {
            if (!linkText.isEmpty()) {
                menu.addAction(createCopyLinkTextAction(&menu, linkText));
            }

            menu.addAction(createCopyLinkUrlAction(&menu));
        }
    }
#ifdef DEBUG_WEB
    auto action = menu.addAction(QStringLiteral("Inspect Element"));
    connect(action, &QAction::triggered, m_widget, &KMarkdownView::inspectElement);
#endif

    if (!menu.isEmpty()) {
        menu.exec(globalPos);
    }
}

void MarkdownPart::showHoveredLink(const QString& link)
{
    QString message;
    KFileItem fileItem;

    if (!link.isEmpty()) {
        QUrl linkUrl(link);

        // Protect the user against URL spoofing!
        linkUrl.setUserName(QString());

        const QString scheme = linkUrl.scheme();

        if (scheme == QLatin1String("javascript")) {
            message = KStringHandler::rsqueeze(link, 150);
        } else {
            message = linkUrl.toString();

            if (QString::compare(scheme, QLatin1String("mailto"), Qt::CaseInsensitive) == 0) {
                fileItem = KFileItem(linkUrl, QString(), KFileItem::Unknown);
            }
        }
    }

    Q_EMIT m_browserExtension->mouseOverInfo(fileItem);
    Q_EMIT setStatusBarText(message);
}

QAction* MarkdownPart::copySelectionAction() const
{
    return m_copySelectionAction;
}

QAction* MarkdownPart::createCopyEmailAddressAction(QObject* parent, const QUrl& mailtoUrl)
{
    auto action = new QAction(parent);
    action->setText(i18n("&Copy Email Address"));
    connect(action, &QAction::triggered, parent, [&] {
        QMimeData* data = new QMimeData;
        data->setText(mailtoUrl.path());
        QApplication::clipboard()->setMimeData(data, QClipboard::Clipboard);
    });

    return action;
}

QAction* MarkdownPart::createCopyLinkTextAction(QObject* parent, const QString& text)
{
    auto action = new QAction(parent);
    action->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    action->setText(i18n("Copy Link &Text"));
    connect(action, &QAction::triggered, parent, [&] {
        QMimeData* data = new QMimeData;
        data->setText(text);
        QApplication::clipboard()->setMimeData(data, QClipboard::Clipboard);
    });

    return action;
}

QAction* MarkdownPart::createCopyLinkUrlAction(QObject* parent)
{
    auto action = new QAction(parent);
    action->setText(i18n("Copy Link &URL"));
    connect(action, &QAction::triggered, this, &MarkdownPart::copyLinkUrl);

    return action;
}

QAction* MarkdownPart::createSaveLinkAsAction(QObject* parent)
{
    auto action = new QAction(parent);
    action->setText(i18n("&Save Link As..."));
    connect(action, &QAction::triggered, this, &MarkdownPart::saveLinkAs);

    return action;
}

void MarkdownPart::copySelection()
{
    m_widget->copySelection();
}

void MarkdownPart::copyLinkUrl()
{
    m_widget->copyLinkUrl();
}

void MarkdownPart::saveLinkAs()
{
    m_widget->saveLinkAs();
}

void MarkdownPart::selectAll()
{
    m_widget->selectAllText();
}

void MarkdownPart::back()
{
    m_widget->back();
}

void MarkdownPart::forward()
{
    m_widget->forward();
}
