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

#ifndef KTEXTEDITORPREVIEW_PREVIEWWIDGET_H
#define KTEXTEDITORPREVIEW_PREVIEWWIDGET_H

#include <markdownpart.h>

// KF
#include <KXMLGUIBuilder>
// Qt
#include <QPointer>
#include <QStackedWidget>

class KTextEditorPreviewPlugin;

namespace KTextEditor
{
class Document;
class MainWindow;
class View;
}
class KXMLGUIFactory;
class KToggleAction;
class KConfigGroup;

class QWidgetAction;
class QMenu;

namespace KTextEditorPreview
{
class KPartView;

/**
 * The actual widget shown in the toolview.
 *
 * It either shows a label "No preview available."
 * or the widget of the KPart currently used to preview
 * the selected document in its target format.
 *
 * The preview can be locked to a document by checking off the
 * lock action. If locked the currently shown document will not
 * be changed if another view is activated, unless the document
 * itself is closed, where then the label is shown instead.
 */
class PreviewWidget : public QStackedWidget, public KXMLGUIBuilder
{
    Q_OBJECT

public:
    /**
     * Constructor
     *
     * @param core the plugin object
     * @param mainWindow the main window with all the texteditor views
     * @param parent widget object taking the ownership
     */
    PreviewWidget(KTextEditorPreviewPlugin *core, KTextEditor::MainWindow *mainWindow, QWidget *parent);
    ~PreviewWidget() override;

    void readSessionConfig(const KConfigGroup &configGroup);
    void writeSessionConfig(KConfigGroup &configGroup) const;

public: // KXMLGUIBuilder API
    QWidget *createContainer(QWidget *parent, int index, const QDomElement &element, QAction *&containerAction) override;
    void removeContainer(QWidget *container, QWidget *parent, QDomElement &element, QAction *containerAction) override;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private Q_SLOTS:
    /**
     * Update the widget to the currently active view.
     *
     * If the document lock is set, this will not change the preview.
     * Otherwise the preview will switch to the document of the now active view
     * and show a preview for that, unless there is no matching kpart found.
     * In that case, or if there is no active view, a label will be shown with
     * "No preview available".
     *
     * @param view the view or, if there is none, a nullptr
     */
    void setTextEditorView(KTextEditor::View *view);
    void resetTextEditorView(KTextEditor::Document *document);
    void unsetDocument(KTextEditor::Document *document);

private:
    void toggleDocumentLocking(bool locked);
    void toggleAutoUpdating(bool autoRefreshing);
    void toggleRevealjs(bool revealjs);
    void updatePreview();
    void updatePreviewNoCache();
    void showAboutKPartPlugin();
    void clearMenu();

    void setScrollPosition(const QPoint& scrollPosition);

private:
    KToggleAction *m_lockAction;
    KToggleAction *m_autoUpdateAction;
    KToggleAction *m_revealjsAction;
    QAction *m_updateAction;
    QAction *m_updateNoCacheAction;
    QWidgetAction *m_kPartMenuAction;
    QMenu *m_kPartMenu;
    QAction *m_aboutKPartAction;

    KTextEditorPreviewPlugin *const m_core;
    KTextEditor::MainWindow *const m_mainWindow;

    KTextEditor::Document *m_previewedTextEditorDocument = nullptr;
    KTextEditor::View *m_previewedTextEditorView = nullptr;
    QString m_currentServiceId;
    QString m_currentMode;
    QPointer<KPartView> m_partView;
    KXMLGUIFactory *m_xmlGuiFactory;

    MarkdownPart* m_part = nullptr;
};

}

#endif
