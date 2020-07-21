/*
 *  Copyright (C) 2017 by Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "previewwidget.h"

#include "kpartview.h"
#include "ktexteditorpreviewplugin.h"
#include <ktexteditorpreview_debug.h>

// KF
#include <KTextEditor/Document>
#include <KTextEditor/MainWindow>
#include <KTextEditor/View>

#include <KAboutApplicationDialog>
#include <KAboutData>
#include <KConfigGroup>
#include <KGuiItem>
#include <KLocalizedString>
#include <KMimeTypeTrader>
#include <KParts/ReadOnlyPart>
#include <KService>
#include <KSharedConfig>
#include <KToggleAction>
#include <KXMLGUIFactory>

// Qt
#include <QAction>
#include <QDomElement>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QWidgetAction>
#include <QMimeDatabase>

using namespace KTextEditorPreview;

PreviewWidget::PreviewWidget(KTextEditorPreviewPlugin *core, KTextEditor::MainWindow *mainWindow, QWidget *parent)
    : QStackedWidget(parent)
    , KXMLGUIBuilder(this)
    , m_core(core)
    , m_mainWindow(mainWindow)
    , m_xmlGuiFactory(new KXMLGUIFactory(this, this))
{
    m_lockAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("object-unlocked")), i18n("Lock Current Document"), this);
    m_lockAction->setToolTip(i18n("Lock preview to current document"));
    m_lockAction->setCheckedState(KGuiItem(i18n("Unlock Current View"), QIcon::fromTheme(QStringLiteral("object-locked")), i18n("Unlock current view")));
    m_lockAction->setChecked(false);
    connect(m_lockAction, &QAction::triggered, this, &PreviewWidget::toggleDocumentLocking);
    addAction(m_lockAction);

    m_revealjsAction = new KToggleAction(QIcon::fromTheme(QStringLiteral("view-presentation")), i18n("Lock Current Document"), this);
    m_revealjsAction->setToolTip(i18n("Preview as reveal.js presentation"));
    m_revealjsAction->setCheckedState(KGuiItem(i18n("Preview as document"), QIcon::fromTheme(QStringLiteral("view-presentation")), i18n("Preview as document")));
    m_revealjsAction->setChecked(false);
    connect(m_revealjsAction, &QAction::triggered, this, &PreviewWidget::toggleRevealjs);
    addAction(m_revealjsAction);

    // TODO: better icon(s)
    const QIcon autoUpdateIcon = QIcon::fromTheme(QStringLiteral("media-playback-start"));
    m_autoUpdateAction = new KToggleAction(autoUpdateIcon, i18n("Automatically Update Preview"), this);
    m_autoUpdateAction->setToolTip(i18n("Enable automatic updates of the preview to the current document content"));
    m_autoUpdateAction->setCheckedState(KGuiItem(i18n("Manually Update Preview"), autoUpdateIcon, i18n("Disable automatic updates of the preview to the current document content")));
    m_autoUpdateAction->setChecked(false);
    connect(m_autoUpdateAction, &QAction::triggered, this, &PreviewWidget::toggleAutoUpdating);
    addAction(m_autoUpdateAction);

    m_updateAction = new QAction(QIcon::fromTheme(QStringLiteral("view-refresh")), i18n("Update Preview"), this);
    m_updateAction->setToolTip(i18n("Update the preview to the current document content"));
    connect(m_updateAction, &QAction::triggered, this, &PreviewWidget::updatePreview);
    m_updateAction->setEnabled(false);
    addAction(m_updateAction);

    m_updateNoCacheAction = new QAction(QIcon::fromTheme(QStringLiteral("update-none")), i18n("Update Preview without cache"), this);
    m_updateNoCacheAction->setToolTip(i18n("Update the preview to the current document content, not using images from cache"));
    connect(m_updateNoCacheAction, &QAction::triggered, this, &PreviewWidget::updatePreviewNoCache);
    m_updateNoCacheAction->setChecked(false);
    addAction(m_updateNoCacheAction);

    // manually prepare a proper dropdown menu button, because Qt itself does not do what one would expect
    // when adding a default menu->menuAction() to a QToolbar
    const auto kPartMenuIcon = QIcon::fromTheme(QStringLiteral("application-menu"));
    const auto kPartMenuText = i18n("View");

    // m_kPartMenu may not be a child of this, because otherwise its XMLGUI-menu is deleted when switching views
    // and therefore closing the tool view, which is a QMainWindow in KDevelop (IdealController::addView).
    // see KXMLGUIBuilder::createContainer => tagName == d->tagMenu
    m_kPartMenu = new QMenu;

    QToolButton *toolButton = new QToolButton();
    toolButton->setMenu(m_kPartMenu);
    toolButton->setIcon(kPartMenuIcon);
    toolButton->setText(kPartMenuText);
    toolButton->setPopupMode(QToolButton::InstantPopup);

    m_kPartMenuAction = new QWidgetAction(this);
    m_kPartMenuAction->setIcon(kPartMenuIcon);
    m_kPartMenuAction->setText(kPartMenuText);
    m_kPartMenuAction->setMenu(m_kPartMenu);
    m_kPartMenuAction->setDefaultWidget(toolButton);
    m_kPartMenuAction->setEnabled(false);
    addAction(m_kPartMenuAction);

    KService::Ptr service = KService::serviceByDesktopName(QLatin1Literal("kmarkdownwebviewpart"));
    m_part = (MarkdownPart*) service->createInstance<KParts::ReadOnlyPart>(nullptr, this, QVariantList(), nullptr);

    if(m_part)
        connect(m_part, &MarkdownPart::scrollPositionChanged, this, &PreviewWidget::setScrollPosition);

    m_aboutKPartAction = new QAction(this);
    connect(m_aboutKPartAction, &QAction::triggered, this, &PreviewWidget::showAboutKPartPlugin);
    m_aboutKPartAction->setEnabled(false);

    auto label = new QLabel(i18n("No preview available."), this);
    label->setAlignment(Qt::AlignHCenter);
    addWidget(label);

    connect(m_mainWindow, &KTextEditor::MainWindow::viewChanged, this, &PreviewWidget::setTextEditorView);

    setTextEditorView(m_mainWindow->activeView());
}

PreviewWidget::~PreviewWidget()
{
    delete m_kPartMenu;
}

void PreviewWidget::readSessionConfig(const KConfigGroup &configGroup)
{
    // TODO: also store document id/url and see to catch the same document on restoring config
    m_lockAction->setChecked(configGroup.readEntry("documentLocked", false));
    m_autoUpdateAction->setChecked(configGroup.readEntry("automaticUpdate", false));
}

void PreviewWidget::writeSessionConfig(KConfigGroup &configGroup) const
{
    configGroup.writeEntry("documentLocked", m_lockAction->isChecked());
    configGroup.writeEntry("automaticUpdate", m_autoUpdateAction->isChecked());
}

void PreviewWidget::setTextEditorView(KTextEditor::View *view)
{
    if ((view && view == m_previewedTextEditorView && view->document() == m_previewedTextEditorDocument && (!m_previewedTextEditorDocument || m_previewedTextEditorDocument->mode() == m_currentMode)) || !view || !isVisible() ||
        m_lockAction->isChecked()) {
        return;
    }

    m_previewedTextEditorView = view;
    m_previewedTextEditorDocument = view ? view->document() : nullptr;

    // Restore revealjs setting for this document
    if(m_previewedTextEditorDocument) {
        const bool revealjs = m_previewedTextEditorDocument->property("pmpmRevealjs").toBool();
        if(m_partView) {
            m_revealjsAction->setChecked(revealjs);
            m_partView->setRevealjs(revealjs);
        }
    }

    resetTextEditorView(m_previewedTextEditorDocument);
}

void PreviewWidget::resetTextEditorView(KTextEditor::Document *document)
{
    if (!isVisible() || m_previewedTextEditorDocument != document) {
        return;
    }

    bool isMarkdown = false;

    if (m_previewedTextEditorDocument) {
        m_currentMode = m_previewedTextEditorDocument->mode();

        // Try with mimetype only from filename. Otherwise we have risk for README.md bug, see e.g. https://gitlab.freedesktop.org/xdg/shared-mime-info/-/issues/127
        QUrl url = m_previewedTextEditorDocument->url();
        if(url.isEmpty()) {
            // Treat new documents as markdown documents?
            isMarkdown = true;
        } else {
            auto fileNameMimes = QMimeDatabase().mimeTypesForFileName(url.path());
            for (const QMimeType &mime: fileNameMimes) {
                if(mime.inherits(QLatin1Literal("text/markdown"))) {
                    isMarkdown = true;
                    break;
                }
            }
        }

        // Update if the mode is changed. The signal may also be emitted, when a new
        // url is loaded, therefore wait (QueuedConnection) for the document to load.
        connect(m_previewedTextEditorDocument, &KTextEditor::Document::modeChanged, this, &PreviewWidget::resetTextEditorView, static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
        // Explicitly clear the old document, which otherwise might be accessed in
        // m_partView->setDocument.
        connect(m_previewedTextEditorDocument, &KTextEditor::Document::aboutToClose, this, &PreviewWidget::unsetDocument, Qt::UniqueConnection);
    } else {
        m_currentMode.clear();
    }

    if(isMarkdown) {
        if(!m_partView) {
            qCDebug(KTEPREVIEW) << "Creating new kpart service instance.";
            m_partView = new KPartView(m_part, this);
            const bool autoupdate = m_autoUpdateAction->isChecked();
            m_partView->setAutoUpdating(autoupdate);
            m_partView->setRevealjs(m_revealjsAction->isChecked());
            int index = addWidget(m_partView->widget());
            setCurrentIndex(index);

            // update kpart menu
            const auto kPart = m_partView->kPart();
            if (kPart) {
                m_xmlGuiFactory->addClient(kPart);

                const auto kPartDisplayName = kPart->componentData().displayName();
                m_aboutKPartAction->setText(i18n("About %1", kPartDisplayName));
                m_aboutKPartAction->setEnabled(true);
                m_kPartMenu->addSeparator();
                m_kPartMenu->addAction(m_aboutKPartAction);
                m_kPartMenuAction->setEnabled(true);
            }

            m_updateAction->setEnabled(!autoupdate);
        }
    } else {
        if (m_partView) {
            clearMenu();
        }
        m_partView = nullptr;
    }

    if (m_partView) {
        QPoint scrollPosition = document ? document->property("pmpmScrollPosition").toPoint() : QPoint(0, 0);
        m_partView->setDocument(m_previewedTextEditorDocument, scrollPosition);
    }
}

void PreviewWidget::unsetDocument(KTextEditor::Document *document)
{
    if (!m_partView || m_previewedTextEditorDocument != document) {
        return;
    }

    m_partView->setDocument(nullptr, QPoint(0, 0));
    m_previewedTextEditorDocument = nullptr;

    // remove any current partview
    clearMenu();
    m_partView = nullptr;
}

void PreviewWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);

    m_updateAction->setEnabled(m_partView && !m_autoUpdateAction->isChecked());

    if (m_lockAction->isChecked()) {
        resetTextEditorView(m_previewedTextEditorDocument);
    } else {
        setTextEditorView(m_mainWindow->activeView());
    }
}

void PreviewWidget::hideEvent(QHideEvent *event)
{
    Q_UNUSED(event);

    // keep active part for reuse, but close preview document
    // TODO: we also get hide event in kdevelop when the view is changed,
    // need to find out how to filter this out or how to fix kdevelop
    // so currently keep the preview document
    //     unsetDocument(m_previewedTextEditorDocument);

    m_updateAction->setEnabled(false);
}

void PreviewWidget::toggleDocumentLocking(bool locked)
{
    if (!locked) {
        setTextEditorView(m_mainWindow->activeView());
    }
}

void PreviewWidget::toggleAutoUpdating(bool autoRefreshing)
{
    if (!m_partView) {
        // nothing to do
        return;
    }

    m_updateAction->setEnabled(!autoRefreshing && isVisible());
    m_partView->setAutoUpdating(autoRefreshing);
}

void PreviewWidget::toggleRevealjs(bool revealjs)
{
    if (!m_partView) {
        // nothing to do
        return;
    }
    m_partView->setRevealjs(revealjs);

    if(m_previewedTextEditorDocument)
        m_previewedTextEditorDocument->setProperty("pmpmRevealjs", revealjs);
}

void PreviewWidget::setScrollPosition(const QPoint& scrollPosition)
{
    if (!m_partView) {
        // nothing to do
        return;
    }

    if(m_previewedTextEditorDocument)
        m_previewedTextEditorDocument->setProperty("pmpmScrollPosition", scrollPosition);
}

void PreviewWidget::updatePreview()
{
    if (m_partView && m_partView->document()) {
        m_partView->updatePreview();
    }
}

void PreviewWidget::updatePreviewNoCache()
{
    if (m_partView && m_partView->document()) {
        m_partView->updatePreviewNoCache();
    }
}

QWidget *PreviewWidget::createContainer(QWidget *parent, int index, const QDomElement &element, QAction *&containerAction)
{
    containerAction = nullptr;

    if (element.attribute(QStringLiteral("deleted")).toLower() == QLatin1String("true")) {
        return nullptr;
    }

    const QString tagName = element.tagName().toLower();
    // filter out things we do not support
    // TODO: consider integrating the toolbars
    if (tagName == QLatin1String("mainwindow") || tagName == QLatin1String("toolbar") || tagName == QLatin1String("statusbar")) {
        return nullptr;
    }

    if (tagName == QLatin1String("menubar")) {
        return m_kPartMenu;
    }

    return KXMLGUIBuilder::createContainer(parent, index, element, containerAction);
}

void PreviewWidget::removeContainer(QWidget *container, QWidget *parent, QDomElement &element, QAction *containerAction)
{
    if (container == m_kPartMenu) {
        return;
    }

    KXMLGUIBuilder::removeContainer(container, parent, element, containerAction);
}

void PreviewWidget::showAboutKPartPlugin()
{
    if (m_partView && m_partView->kPart()) {
        QPointer<KAboutApplicationDialog> aboutDialog = new KAboutApplicationDialog(m_partView->kPart()->componentData(), this);
        aboutDialog->exec();
        delete aboutDialog;
    }
}

void PreviewWidget::clearMenu()
{
    // clear kpart menu
    m_xmlGuiFactory->removeClient(m_partView->kPart());
    m_kPartMenu->clear();

    removeWidget(m_partView->widget());
    delete m_partView;

    m_updateAction->setEnabled(false);
    m_kPartMenuAction->setEnabled(false);
    m_aboutKPartAction->setEnabled(false);
}
