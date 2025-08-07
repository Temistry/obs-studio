#pragma once

#include <QWidget>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include <QResizeEvent>
#include <QTimer>
#include <QScrollArea>
#include <QToolBar>
#include <QAction>
#include <QComboBox>
#include <QSpinBox>
#include <QFontComboBox>
#include <QColorDialog>
#include <QMenu>
#include "SubtitleManager.hpp"

class SubtitleEditor : public QWidget {
    Q_OBJECT

private:
    SubtitleManager *subtitleManager;
    
    // Main layout components
    QVBoxLayout *mainLayout;
    QSplitter *mainSplitter;
    
    // Toolbar for quick actions
    QToolBar *toolbar;
    QAction *newAction;
    QAction *saveAction;
    QAction *deleteAction;
    QAction *clearAction;
    QAction *bibleAction;
    QAction *hymnAction;
    
    // Font formatting toolbar
    QToolBar *formatToolbar;
    QFontComboBox *fontComboBox;
    QSpinBox *fontSizeSpinBox;
    QPushButton *boldButton;
    QPushButton *italicButton;
    QPushButton *underlineButton;
    QPushButton *colorButton;
    QColor currentTextColor;
    
    // Left panel - subtitle list
    QWidget *listPanel;
    QVBoxLayout *listLayout;
    QGroupBox *listGroup;
    QListWidget *subtitleListWidget;
    QHBoxLayout *listControlLayout;
    QPushButton *addSubtitleButton;
    QPushButton *removeSubtitleButton;
    QLabel *subtitleCountLabel;
    
    // Right panel - editor
    QWidget *editorPanel;
    QVBoxLayout *editorLayout;
    
    // Title editing section
    QGroupBox *titleGroup;
    QVBoxLayout *titleLayout;
    QLineEdit *titleEdit;
    
    // Content editing section
    QGroupBox *contentGroup;
    QVBoxLayout *contentLayout;
    QTextEdit *contentEdit;
    
    // Editor controls
    QHBoxLayout *editorControlLayout;
    QPushButton *previewButton;
    QPushButton *applyButton;
    QPushButton *revertButton;
    
    // Status and info
    QHBoxLayout *statusLayout;
    QLabel *statusLabel;
    QLabel *characterCountLabel;
    
    // Auto-save functionality
    QTimer *autoSaveTimer;
    bool hasUnsavedChanges;
    int currentEditingIndex;
    
    // UI state
    bool isEditingMode;
    QString originalTitle;
    QString originalContent;
    
    void SetupUI();
    void SetupToolbars();
    void SetupSplitter();
    void ConnectSignals();
    void UpdateSubtitleList();
    void UpdateCharacterCount();
    void UpdateStatus(const QString &message);
    void SetEditingMode(bool enabled);
    void ApplyFormatting();
    void SaveCurrentSubtitle();
    void RevertChanges();

public:
    explicit SubtitleEditor(QWidget *parent = nullptr);
    ~SubtitleEditor();
    
    // Public interface
    void SetSubtitleManager(SubtitleManager *manager);
    SubtitleManager* GetSubtitleManager() const { return subtitleManager; }
    
    // Editor state
    bool HasUnsavedChanges() const { return hasUnsavedChanges; }
    void FocusEditor();
    void SetReadOnly(bool readOnly);
    
    // Content access
    QString GetCurrentTitle() const;
    QString GetCurrentContent() const;
    void SetCurrentContent(const QString &title, const QString &content);
    
    // Size and layout management
    void SetPreferredSize(const QSize &size);
    QSize GetPreferredSize() const;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

public slots:
    // Subtitle management
    void OnNewSubtitle();
    void OnSaveSubtitle();
    void OnDeleteSubtitle();
    void OnClearEditor();
    
    // List management
    void OnSubtitleListSelectionChanged();
    void OnAddSubtitle();
    void OnRemoveSubtitle();
    
    // Editor controls
    void OnPreviewSubtitle();
    void OnApplyChanges();
    void OnRevertChanges();
    
    // Text formatting
    void OnFontChanged();
    void OnFontSizeChanged();
    void OnBoldToggled();
    void OnItalicToggled();
    void OnUnderlineToggled();
    void OnColorButtonClicked();
    void OnTextColorChanged();
    
    // Content search
    void OnBibleSearch();
    void OnHymnSearch();
    
    // Auto-save
    void OnAutoSave();
    void OnContentChanged();
    void OnTitleChanged();
    
    // SubtitleManager signals
    void OnSubtitleChanged(int index);
    void OnSubtitleListChanged();
    void OnTargetSourceChanged(const QString &sourceName);

private slots:
    void OnTextEditTextChanged();
    void UpdateToolbarState();

signals:
    void SubtitleEditorClosed();
    void SubtitleApplied(int index);
    void ContentModified(bool hasChanges);
};

class SubtitleEditorDock : public QDockWidget {
    Q_OBJECT

private:
    SubtitleEditor *subtitleEditor;

public:
    explicit SubtitleEditorDock(QWidget *parent = nullptr);
    ~SubtitleEditorDock();
    
    SubtitleEditor* GetEditor() const { return subtitleEditor; }
    void SetSubtitleManager(SubtitleManager *manager);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

signals:
    void SubtitleEditorDockClosed();
};