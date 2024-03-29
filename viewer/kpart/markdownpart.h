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

#ifndef MARKDOWNPART_H
#define MARKDOWNPART_H

#include <kmarkdownviewpart_export.h>

// KF
#include <KParts/ReadOnlyPart>

// Qt
#include <QByteArray>
#include <QPoint>

class MarkdownBrowserExtension;
class MarkdownSourceDocument;
class KMarkdownView;
class SearchToolBar;
class KPluginMetaData;

class KMARKDOWNVIEWPART_EXPORT MarkdownPart : public KParts::ReadOnlyPart
{
    Q_OBJECT

public:
    enum Modus {
        ReadOnlyModus = 0,
        BrowserViewModus = 1
    };

    /**
     * Default constructor, with arguments as expected by MarkdownPartFactory
     */
    MarkdownPart(QWidget* parentWidget, QObject* parent, const KPluginMetaData& metaData, Modus modus);

    /**
     * Destructor
     */
    ~MarkdownPart() override;

    KMarkdownView* view() const;

    QAction* copySelectionAction() const;
    QAction* createCopyEmailAddressAction(QObject* parent, const QUrl& mailtoUrl);
    QAction* createCopyLinkTextAction(QObject* parent, const QString& text);
    QAction* createCopyLinkUrlAction(QObject* parent);
    QAction* createSaveLinkAsAction(QObject* parent);

    void copySelection();

    bool closeUrl() override;

    void pmpmDirectOpen(const QString& text, const QUrl& url, bool revealjs, bool flushCache);

Q_SIGNALS:
    void scrollPositionChanged(const QPoint& scrollPosition);

protected: // KParts::ReadOnlyPart API
    bool openFile() override;

    bool doOpenStream(const QString& mimeType) override;
    bool doWriteStream(const QByteArray& data) override;
    bool doCloseStream() override;


private:
    void setupActions(Modus modus);
    void prepareViewStateRestoringOnReload();
    void restoreScrollPosition();
    void reloadImagesWithoutCache();

    void handleOpenUrlRequest(const QUrl& url);
    void requestContextMenu(const QPoint& globalPos,
                            const QUrl& linkUrl, const QString& linkText,
                            bool hasSelection, bool forcesNewWindow);
    void showHoveredLink(const QString& link);

    void copyLinkUrl();
    void saveLinkAs();
    void selectAll();
    void back();
    void forward();

private:
    MarkdownSourceDocument* m_sourceDocument;
    KMarkdownView* m_widget;
    SearchToolBar* m_searchToolBar;
    QAction* m_copySelectionAction;
    QAction* m_selectAllAction;
    QAction* m_searchAction;
    QAction* m_searchNextAction;
    QAction* m_searchPreviousAction;
    QAction* m_backAction;
    QAction* m_forwardAction;

    MarkdownBrowserExtension* const m_browserExtension;

    QByteArray m_streamedData;

    QUrl m_previousUrl;
    QPoint m_previousScrollPosition;
};

inline KMarkdownView* MarkdownPart::view() const { return m_widget; }

#endif
