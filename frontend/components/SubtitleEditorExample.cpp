/* 
 * SubtitleEditor Integration Example
 * 
 * This file demonstrates how to integrate and use the new SubtitleEditor component
 * in your OBS Studio application. The SubtitleEditor provides a separate, resizable
 * and dockable UI specifically focused on editing subtitle content.
 */

#include "SubtitleEditor.hpp"
#include "SubtitleManager.hpp"
#include <QMainWindow>
#include <QMenuBar>
#include <QAction>
#include <QApplication>

/* 
 * Example integration in your main window class (e.g., OBSBasic):
 * 
 * 1. Add to your header file (e.g., OBSBasic.hpp):
 */

/*
class OBSBasic : public QMainWindow {
    Q_OBJECT
    
private:
    SubtitleManager *subtitleManager;
    SubtitleEditorDock *subtitleEditorDock;
    
    // ... other members
    
public slots:
    void OpenSubtitleEditor();
    void CloseSubtitleEditor();
};
*/

/*
 * 2. In your constructor (e.g., OBSBasic constructor):
 */

void ExampleConstructorCode() {
    /*
    // Initialize subtitle manager
    subtitleManager = new SubtitleManager(this);
    
    // Create subtitle editor dock (initially hidden)
    subtitleEditorDock = new SubtitleEditorDock(this);
    subtitleEditorDock->SetSubtitleManager(subtitleManager);
    subtitleEditorDock->hide();
    
    // Add as dockable widget
    addDockWidget(Qt::RightDockWidgetArea, subtitleEditorDock);
    
    // Connect signals
    connect(subtitleEditorDock, &SubtitleEditorDock::SubtitleEditorDockClosed, 
            this, &OBSBasic::CloseSubtitleEditor);
    
    // Add menu action
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    QAction *subtitleEditorAction = viewMenu->addAction(tr("자막 편집기"));
    subtitleEditorAction->setCheckable(true);
    subtitleEditorAction->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_E));
    
    connect(subtitleEditorAction, &QAction::triggered, [this, subtitleEditorAction](bool checked) {
        if (checked) {
            OpenSubtitleEditor();
        } else {
            CloseSubtitleEditor();
        }
    });
    */
}

/*
 * 3. Implementation of slot methods:
 */

void ExampleSlotImplementations() {
    /*
    void OBSBasic::OpenSubtitleEditor() {
        if (subtitleEditorDock) {
            subtitleEditorDock->show();
            subtitleEditorDock->raise();
            subtitleEditorDock->activateWindow();
            
            // Focus the editor
            SubtitleEditor *editor = subtitleEditorDock->GetEditor();
            if (editor) {
                editor->FocusEditor();
            }
        }
    }
    
    void OBSBasic::CloseSubtitleEditor() {
        if (subtitleEditorDock) {
            subtitleEditorDock->hide();
        }
    }
    */
}

/*
 * 4. Advanced usage - Creating floating editor window:
 */

void ExampleFloatingEditor() {
    /*
    // Create a floating subtitle editor window
    SubtitleEditor *floatingEditor = new SubtitleEditor();
    floatingEditor->SetSubtitleManager(subtitleManager);
    floatingEditor->setWindowFlags(Qt::Window);
    floatingEditor->setWindowTitle(tr("자막 편집기 - 독립 창"));
    floatingEditor->resize(800, 600);
    floatingEditor->show();
    
    // Save/restore window geometry
    QSettings settings;
    floatingEditor->restoreGeometry(settings.value("SubtitleEditor/geometry").toByteArray());
    
    connect(floatingEditor, &SubtitleEditor::SubtitleEditorClosed, [=]() {
        QSettings settings;
        settings.setValue("SubtitleEditor/geometry", floatingEditor->saveGeometry());
        floatingEditor->deleteLater();
    });
    */
}

/*
 * 5. Programmatic content manipulation:
 */

void ExampleProgrammaticUsage() {
    /*
    // Get the editor instance
    SubtitleEditor *editor = subtitleEditorDock->GetEditor();
    if (editor && editor->GetSubtitleManager()) {
        // Set specific content
        editor->SetCurrentContent("예배 제목", "오늘의 말씀 내용입니다.");
        
        // Check for unsaved changes
        if (editor->HasUnsavedChanges()) {
            // Handle unsaved changes
        }
        
        // Set read-only mode for viewing
        editor->SetReadOnly(true);
        
        // Get current content
        QString currentTitle = editor->GetCurrentTitle();
        QString currentContent = editor->GetCurrentContent();
    }
    */
}

/*
 * 6. Integration with existing hotkeys (in your hotkey setup):
 */

void ExampleHotkeyIntegration() {
    /*
    // Add hotkey for opening subtitle editor
    auto subtitleEditor = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
        if (pressed) {
            QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "OpenSubtitleEditor",
                                    Qt::QueuedConnection);
        }
    };
    
    obs_hotkey_id subtitleEditorHotkey = obs_hotkey_register_frontend(
        "OBSBasic.SubtitleEditor", 
        "Subtitle.Editor", 
        subtitleEditor, 
        this
    );
    
    LoadHotkey(subtitleEditorHotkey, "OBSBasic.SubtitleEditor");
    */
}

/*
 * Key Features of the SubtitleEditor:
 * 
 * 1. Resizable and Dockable:
 *    - Can be docked to any side of the main window
 *    - Can be undocked as a floating window
 *    - Remembers size and position
 *    - Splitter between subtitle list and editor is adjustable
 * 
 * 2. Rich Text Editing:
 *    - Font selection and size adjustment
 *    - Bold, italic, underline formatting
 *    - Text color selection
 *    - Real-time character count
 * 
 * 3. Auto-save and Change Detection:
 *    - Automatic saving after 5 seconds of inactivity
 *    - Detects unsaved changes
 *    - Confirmation dialogs for unsaved work
 * 
 * 4. Integrated with SubtitleManager:
 *    - Full integration with existing subtitle system
 *    - Support for worship folders
 *    - Bible and hymn search integration (when implemented)
 * 
 * 5. User-Friendly Interface:
 *    - Intuitive toolbar with common actions
 *    - Context-sensitive controls
 *    - Status updates and feedback
 *    - Preview functionality
 * 
 * 6. Flexible Layout:
 *    - Left panel shows subtitle list with quick access
 *    - Right panel provides full editing capabilities
 *    - Toolbars can be customized or hidden
 *    - Responsive design adapts to window size
 * 
 * Usage Tips:
 * 
 * - The editor works best when docked to the right side of the main window
 * - Minimum recommended size is 600x400 pixels
 * - Use Ctrl+S to save manually, or rely on auto-save
 * - Double-click a subtitle in the list to quickly edit it
 * - Use the preview button to test subtitles before applying
 * - The splitter can be dragged to adjust the list/editor ratio
 */