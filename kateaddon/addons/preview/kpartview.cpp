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

#include "kpartview.h"

#include <ktexteditorpreview_debug.h>

// KF
#include <KTextEditor/Document>

#include <KLocalizedString>
#include <KParts/BrowserExtension>
#include <KParts/ReadOnlyPart>
#include <KPluginFactory>
#include <KPluginLoader>

// Qt
#include <QDesktopServices>
#include <QLabel>
#include <QTemporaryFile>

using namespace KTextEditorPreview;

// There are two timers that run on update. One timer is fast, but is
// cancelled each time a new updated comes in. Another timer is slow, but is 
// not cancelled if another update comes in. With this, "while typing", the
// preview is updated every 1000ms, thus one sees that something is happening
// from the corner of one's eyes. After stopping typing, the preview is
// updated quickly after 150ms so that the preview has the newest version.
static const int updateDelayFast = 150; // ms
static const int updateDelaySlow = 1000; // ms

KPartView::KPartView(const KService::Ptr &service, QObject *parent)
    : QObject(parent)
{
    QString errorString;
    m_part = service->createInstance<KParts::ReadOnlyPart>(nullptr, this, QVariantList(), &errorString);

    if (!m_part) {
        m_errorLabel = new QLabel(errorString);
    } else if (!m_part->widget()) {
        // should not happen, but just be safe
        delete m_part;
        m_errorLabel = new QLabel(QStringLiteral("KPart provides no widget."));
    } else {
        m_updateSquashingTimerFast.setSingleShot(true);
        m_updateSquashingTimerFast.setInterval(updateDelayFast);
        connect(&m_updateSquashingTimerFast, &QTimer::timeout, this, &KPartView::updatePreview);

        m_updateSquashingTimerSlow.setSingleShot(true);
        m_updateSquashingTimerSlow.setInterval(updateDelaySlow);
        connect(&m_updateSquashingTimerSlow, &QTimer::timeout, this, &KPartView::updatePreview);

        auto browserExtension = m_part->browserExtension();
        if (browserExtension) {
            connect(browserExtension, &KParts::BrowserExtension::openUrlRequestDelayed, this, &KPartView::handleOpenUrlRequest);
        }
        m_part->widget()->installEventFilter(this);
    }
}

KPartView::~KPartView()
{
    delete m_errorLabel;
}

QWidget *KPartView::widget() const
{
    return m_part ? m_part->widget() : m_errorLabel;
}

KParts::ReadOnlyPart *KPartView::kPart() const
{
    return m_part;
}

KTextEditor::Document *KPartView::document() const
{
    return m_document;
}

bool KPartView::isAutoUpdating() const
{
    return m_autoUpdating;
}

bool KPartView::isRevealjs() const
{
    return m_revealjs;
}

void KPartView::setDocument(KTextEditor::Document *document)
{
    if (m_document == document) {
        return;
    }
    if (!m_part) {
        return;
    }

    if (m_document) {
        disconnect(m_document, &KTextEditor::Document::textChanged, this, &KPartView::triggerUpdatePreview);
        m_updateSquashingTimerFast.stop();
        m_updateSquashingTimerSlow.stop();
    }

    m_document = document;

    // delete any temporary file, to trigger creation of a new if needed
    // for some unique url/path of the temporary file for the new document (or use a counter ourselves?)
    // but see comment for stream url
    delete m_bufferFile;
    m_bufferFile = nullptr;

    if (m_document) {
        m_previewDirty = true;
        updatePreview();
        connect(m_document, &KTextEditor::Document::textChanged, this, &KPartView::triggerUpdatePreview);
    } else {
        m_part->closeUrl();
    }
}

void KPartView::setAutoUpdating(bool autoUpdating)
{
    if (m_autoUpdating == autoUpdating) {
        return;
    }

    m_autoUpdating = autoUpdating;

    if (m_autoUpdating) {
        if (m_document && m_part && m_previewDirty) {
            updatePreview();
        }
    } else {
        m_updateSquashingTimerSlow.stop();
        m_updateSquashingTimerFast.stop();
    }
}

void KPartView::setRevealjs(bool revealjs)
{
    if (m_revealjs == revealjs) {
        return;
    }

    m_revealjs = revealjs;

    if(m_document && m_part) {
        updatePreview();
    }
}


void KPartView::triggerUpdatePreview()
{
    m_previewDirty = true;
    
    if (m_part->widget()->isVisible() && m_autoUpdating) {
        // Reset fast timer each time
        m_updateSquashingTimerFast.start();
        // Start slow timer, if not already running (don't reset!)
        if(!m_updateSquashingTimerSlow.isActive())
            m_updateSquashingTimerSlow.start();
    }
}

void KPartView::updatePreview()
{
    m_updateSquashingTimerSlow.stop();
    m_updateSquashingTimerFast.stop();
    if (!m_part->widget()->isVisible()) {
        return;
    }

    // TODO: some kparts seem to steal the focus after they have loaded a file, sometimes also async
    // that possibly needs fixing in the respective kparts, as that could be considered non-cooperative

    // TODO: investigate if pushing of the data to the kpart could be done in a non-gui-thread,
    // so their loading of the file (e.g. ReadOnlyPart::openFile() is sync design) does not block

    auto mimeType = m_document->mimeType();
    if(m_revealjs)
        mimeType = QLatin1Literal("text/markdown+revealjs");

    KParts::OpenUrlArguments arguments;
    arguments.setMimeType(mimeType);
    m_part->setArguments(arguments);

    // try to stream the data to avoid filesystem I/O
    // create url unique for this document
    // TODO: encode existing url instead, and for yet-to-be-stored docs some other unique id
    
    // Use document's url to make relative links works in kmarkdownview (pmpm version)
    // TODO: Add GET parameter to make unique?
    // TODO: yet-to-be-stored docs don't have preview anyway??
    const QUrl streamUrl = m_document->url();
    if (m_part->openStream(mimeType, streamUrl)) {
        qCDebug(KTEPREVIEW) << "Pushing data via streaming API, url:" << streamUrl.url();
        m_part->writeStream(m_document->text().toUtf8());
        m_part->closeStream();

        m_previewDirty = false;
        return;
    }

    // have to go via filesystem for now, not nice
    if (!m_bufferFile) {
        m_bufferFile = new QTemporaryFile(this);
        m_bufferFile->open();
    } else {
        // reset position
        m_bufferFile->seek(0);
    }
    const QUrl tempFileUrl(QUrl::fromLocalFile(m_bufferFile->fileName()));
    qCDebug(KTEPREVIEW) << "Pushing data via temporary file, url:" << tempFileUrl.url();

    // write current data
    m_bufferFile->write(m_document->text().toUtf8());
    // truncate at end of new content
    m_bufferFile->resize(m_bufferFile->pos());
    m_bufferFile->flush();

    // TODO: find out why we need to send this queued
    QMetaObject::invokeMethod(m_part, "openUrl", Qt::QueuedConnection, Q_ARG(QUrl, tempFileUrl));

    m_previewDirty = false;
}

void KPartView::handleOpenUrlRequest(const QUrl &url)
{
    QDesktopServices::openUrl(url);
}

bool KPartView::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_part->widget() && event->type() == QEvent::Show) {
        if (m_document && m_autoUpdating && m_previewDirty) {
            updatePreview();
        }
        return true;
    }

    return QObject::eventFilter(object, event);
}
