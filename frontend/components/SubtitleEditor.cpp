#include "SubtitleEditor.hpp"
#include <QApplication>
#include <QStyle>
#include <QSplitter>
#include <QSettings>
#include <QMessageBox>
#include <QFontDialog>
#include <QColorDialog>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QTextDocument>
#include <QBrush>
#include <obs.hpp>
#include <qt-wrappers.hpp>

#include "moc_SubtitleEditor.cpp"

SubtitleEditor::SubtitleEditor(QWidget *parent)
    : QWidget(parent),
      subtitleManager(nullptr),
      hasUnsavedChanges(false),
      currentEditingIndex(-1),
      isEditingMode(false),
      currentTextColor(Qt::black)
{
    SetupUI();
    SetupToolbars();
    SetupSplitter();
    ConnectSignals();
    
    // Setup auto-save timer
    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setSingleShot(true);
    autoSaveTimer->setInterval(5000); // 5 seconds
    connect(autoSaveTimer, &QTimer::timeout, this, &SubtitleEditor::OnAutoSave);
    
    // Initialize state
    SetEditingMode(false);
    UpdateStatus(tr("준비"));
    
    blog(LOG_INFO, "[SubtitleEditor] Subtitle editor initialized");
}

SubtitleEditor::~SubtitleEditor()
{
    if (hasUnsavedChanges) {
        SaveCurrentSubtitle();
    }
    blog(LOG_INFO, "[SubtitleEditor] Subtitle editor destroyed");
}

void SubtitleEditor::SetupUI()
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    // Create main splitter
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    mainSplitter->setChildrenCollapsible(false);
    
    // Left panel - subtitle list
    listPanel = new QWidget();
    listPanel->setMinimumWidth(200);
    listPanel->setMaximumWidth(350);
    
    listLayout = new QVBoxLayout(listPanel);
    listLayout->setContentsMargins(5, 5, 5, 5);
    
    listGroup = new QGroupBox(tr("자막 목록"));
    QVBoxLayout *listGroupLayout = new QVBoxLayout(listGroup);
    
    subtitleListWidget = new QListWidget();
    subtitleListWidget->setAlternatingRowColors(true);
    subtitleListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    listGroupLayout->addWidget(subtitleListWidget);
    
    // List controls
    listControlLayout = new QHBoxLayout();
    addSubtitleButton = new QPushButton(tr("추가"));
    addSubtitleButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    removeSubtitleButton = new QPushButton(tr("삭제"));
    removeSubtitleButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_TrashIcon));
    
    listControlLayout->addWidget(addSubtitleButton);
    listControlLayout->addWidget(removeSubtitleButton);
    listControlLayout->addStretch();
    
    subtitleCountLabel = new QLabel(tr("총 0개"));
    listControlLayout->addWidget(subtitleCountLabel);
    
    listGroupLayout->addLayout(listControlLayout);
    listLayout->addWidget(listGroup);
    
    // Right panel - editor
    editorPanel = new QWidget();
    editorPanel->setMinimumWidth(400);
    
    editorLayout = new QVBoxLayout(editorPanel);
    editorLayout->setContentsMargins(5, 5, 5, 5);
    
    // Title editing section
    titleGroup = new QGroupBox(tr("제목"));
    titleLayout = new QVBoxLayout(titleGroup);
    titleEdit = new QLineEdit();
    titleEdit->setPlaceholderText(tr("자막 제목을 입력하세요"));
    titleLayout->addWidget(titleEdit);
    editorLayout->addWidget(titleGroup);
    
    // Content editing section
    contentGroup = new QGroupBox(tr("내용"));
    contentLayout = new QVBoxLayout(contentGroup);
    contentEdit = new QTextEdit();
    contentEdit->setPlaceholderText(tr("자막 내용을 입력하세요"));
    contentEdit->setMinimumHeight(200);
    contentLayout->addWidget(contentEdit);
    editorLayout->addWidget(contentGroup);
    
    // Editor controls
    editorControlLayout = new QHBoxLayout();
    previewButton = new QPushButton(tr("미리보기"));
    previewButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_MediaPlay));
    applyButton = new QPushButton(tr("적용"));
    applyButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogApplyButton));
    revertButton = new QPushButton(tr("되돌리기"));
    revertButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
    
    editorControlLayout->addWidget(previewButton);
    editorControlLayout->addStretch();
    editorControlLayout->addWidget(revertButton);
    editorControlLayout->addWidget(applyButton);
    
    editorLayout->addLayout(editorControlLayout);
    
    // Status and info
    statusLayout = new QHBoxLayout();
    statusLabel = new QLabel(tr("준비"));
    characterCountLabel = new QLabel(tr("0자"));
    
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();
    statusLayout->addWidget(characterCountLabel);
    
    editorLayout->addLayout(statusLayout);
    
    // Add panels to splitter
    mainSplitter->addWidget(listPanel);
    mainSplitter->addWidget(editorPanel);
    mainSplitter->setSizes({250, 550});
    
    mainLayout->addWidget(mainSplitter);
}

void SubtitleEditor::SetupToolbars()
{
    // Main toolbar
    toolbar = new QToolBar(tr("도구"));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setMovable(false);
    
    newAction = toolbar->addAction(QApplication::style()->standardIcon(QStyle::SP_FileIcon), tr("새로만들기"));
    saveAction = toolbar->addAction(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton), tr("저장"));
    deleteAction = toolbar->addAction(QApplication::style()->standardIcon(QStyle::SP_TrashIcon), tr("삭제"));
    toolbar->addSeparator();
    clearAction = toolbar->addAction(QApplication::style()->standardIcon(QStyle::SP_LineEditClearButton), tr("지우기"));
    toolbar->addSeparator();
    bibleAction = toolbar->addAction(tr("성경 검색"));
    hymnAction = toolbar->addAction(tr("찬송가 검색"));
    
    mainLayout->insertWidget(0, toolbar);
    
    // Format toolbar
    formatToolbar = new QToolBar(tr("서식"));
    formatToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    formatToolbar->setMovable(false);
    
    fontComboBox = new QFontComboBox();
    fontComboBox->setMaximumWidth(150);
    formatToolbar->addWidget(fontComboBox);
    
    fontSizeSpinBox = new QSpinBox();
    fontSizeSpinBox->setRange(8, 72);
    fontSizeSpinBox->setValue(12);
    fontSizeSpinBox->setMaximumWidth(60);
    formatToolbar->addWidget(fontSizeSpinBox);
    
    formatToolbar->addSeparator();
    
    boldButton = new QPushButton(tr("B"));
    boldButton->setCheckable(true);
    boldButton->setMaximumSize(30, 24);
    boldButton->setFont(QFont("Arial", 10, QFont::Bold));
    formatToolbar->addWidget(boldButton);
    
    italicButton = new QPushButton(tr("I"));
    italicButton->setCheckable(true);
    italicButton->setMaximumSize(30, 24);
    italicButton->setFont(QFont("Arial", 10, QFont::Normal, true));
    formatToolbar->addWidget(italicButton);
    
    underlineButton = new QPushButton(tr("U"));
    underlineButton->setCheckable(true);
    underlineButton->setMaximumSize(30, 24);
    QFont underlineFont = underlineButton->font();
    underlineFont.setUnderline(true);
    underlineButton->setFont(underlineFont);
    formatToolbar->addWidget(underlineButton);
    
    formatToolbar->addSeparator();
    
    colorButton = new QPushButton();
    colorButton->setText(tr("색상"));
    colorButton->setMaximumWidth(50);
    formatToolbar->addWidget(colorButton);
    
    mainLayout->insertWidget(1, formatToolbar);
}

void SubtitleEditor::SetupSplitter()
{
    // Enable splitter handle styling
    mainSplitter->setHandleWidth(3);
    mainSplitter->setStyleSheet(
        "QSplitter::handle {"
        "    background-color: #CCCCCC;"
        "    border: 1px solid #999999;"
        "}"
        "QSplitter::handle:hover {"
        "    background-color: #BBBBBB;"
        "}"
    );
}

void SubtitleEditor::ConnectSignals()
{
    // Toolbar actions
    connect(newAction, &QAction::triggered, this, &SubtitleEditor::OnNewSubtitle);
    connect(saveAction, &QAction::triggered, this, &SubtitleEditor::OnSaveSubtitle);
    connect(deleteAction, &QAction::triggered, this, &SubtitleEditor::OnDeleteSubtitle);
    connect(clearAction, &QAction::triggered, this, &SubtitleEditor::OnClearEditor);
    connect(bibleAction, &QAction::triggered, this, &SubtitleEditor::OnBibleSearch);
    connect(hymnAction, &QAction::triggered, this, &SubtitleEditor::OnHymnSearch);
    
    // Format toolbar
    connect(fontComboBox, &QFontComboBox::currentFontChanged, this, &SubtitleEditor::OnFontChanged);
    connect(fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &SubtitleEditor::OnFontSizeChanged);
    connect(boldButton, &QPushButton::toggled, this, &SubtitleEditor::OnBoldToggled);
    connect(italicButton, &QPushButton::toggled, this, &SubtitleEditor::OnItalicToggled);
    connect(underlineButton, &QPushButton::toggled, this, &SubtitleEditor::OnUnderlineToggled);
    connect(colorButton, &QPushButton::clicked, this, &SubtitleEditor::OnColorButtonClicked);
    
    // List controls
    connect(subtitleListWidget, &QListWidget::itemSelectionChanged, this, &SubtitleEditor::OnSubtitleListSelectionChanged);
    connect(addSubtitleButton, &QPushButton::clicked, this, &SubtitleEditor::OnAddSubtitle);
    connect(removeSubtitleButton, &QPushButton::clicked, this, &SubtitleEditor::OnRemoveSubtitle);
    
    // Editor controls
    connect(previewButton, &QPushButton::clicked, this, &SubtitleEditor::OnPreviewSubtitle);
    connect(applyButton, &QPushButton::clicked, this, &SubtitleEditor::OnApplyChanges);
    connect(revertButton, &QPushButton::clicked, this, &SubtitleEditor::OnRevertChanges);
    
    // Content change detection
    connect(titleEdit, &QLineEdit::textChanged, this, &SubtitleEditor::OnTitleChanged);
    connect(contentEdit, &QTextEdit::textChanged, this, &SubtitleEditor::OnContentChanged);
    connect(contentEdit, &QTextEdit::textChanged, this, &SubtitleEditor::OnTextEditTextChanged);
    connect(contentEdit, &QTextEdit::cursorPositionChanged, this, &SubtitleEditor::UpdateToolbarState);
}

void SubtitleEditor::SetSubtitleManager(SubtitleManager *manager)
{
    if (subtitleManager) {
        disconnect(subtitleManager, nullptr, this, nullptr);
    }
    
    subtitleManager = manager;
    
    if (subtitleManager) {
        connect(subtitleManager, &SubtitleManager::SubtitleChanged, this, &SubtitleEditor::OnSubtitleChanged);
        connect(subtitleManager, &SubtitleManager::SubtitleListChanged, this, &SubtitleEditor::OnSubtitleListChanged);
        connect(subtitleManager, &SubtitleManager::TargetSourceChanged, this, &SubtitleEditor::OnTargetSourceChanged);
        
        UpdateSubtitleList();
        blog(LOG_INFO, "[SubtitleEditor] Subtitle manager connected");
    }
}

void SubtitleEditor::UpdateSubtitleList()
{
    if (!subtitleManager) return;
    
    subtitleListWidget->clear();
    
    QList<SubtitleItem> subtitles = subtitleManager->GetAllSubtitles();
    for (int i = 0; i < subtitles.size(); ++i) {
        const SubtitleItem &item = subtitles[i];
        QListWidgetItem *listItem = new QListWidgetItem();
        
        QString displayText = item.title.isEmpty() ? tr("(제목 없음)") : item.title;
        if (!item.enabled) {
            displayText += tr(" (비활성화됨)");
        }
        
        listItem->setText(displayText);
        listItem->setData(Qt::UserRole, i);
        listItem->setToolTip(QString(tr("제목: %1\n내용: %2")).arg(item.title).arg(item.content.left(100)));
        
        if (!item.enabled) {
            listItem->setForeground(QBrush(Qt::gray));
        }
        
        subtitleListWidget->addItem(listItem);
    }
    
    subtitleCountLabel->setText(tr("총 %1개").arg(subtitles.size()));
    
    // Update button states
    bool hasItems = subtitles.size() > 0;
    removeSubtitleButton->setEnabled(hasItems);
    deleteAction->setEnabled(hasItems);
}

void SubtitleEditor::UpdateCharacterCount()
{
    QString text = contentEdit->toPlainText();
    characterCountLabel->setText(tr("%1자").arg(text.length()));
}

void SubtitleEditor::UpdateStatus(const QString &message)
{
    statusLabel->setText(message);
}

void SubtitleEditor::SetEditingMode(bool enabled)
{
    isEditingMode = enabled;
    
    titleEdit->setEnabled(enabled);
    contentEdit->setEnabled(enabled);
    applyButton->setEnabled(enabled && hasUnsavedChanges);
    revertButton->setEnabled(enabled && hasUnsavedChanges);
    previewButton->setEnabled(enabled);
    
    // Format toolbar
    fontComboBox->setEnabled(enabled);
    fontSizeSpinBox->setEnabled(enabled);
    boldButton->setEnabled(enabled);
    italicButton->setEnabled(enabled);
    underlineButton->setEnabled(enabled);
    colorButton->setEnabled(enabled);
    
    if (!enabled) {
        titleEdit->clear();
        contentEdit->clear();
        hasUnsavedChanges = false;
        currentEditingIndex = -1;
        UpdateStatus(tr("자막을 선택하여 편집하세요"));
    }
}

void SubtitleEditor::SaveCurrentSubtitle()
{
    if (!subtitleManager || !isEditingMode || !hasUnsavedChanges) return;
    
    QString title = titleEdit->text().trimmed();
    QString content = contentEdit->toPlainText().trimmed();
    
    if (currentEditingIndex >= 0) {
        // Update existing subtitle
        subtitleManager->UpdateSubtitle(currentEditingIndex, title, content);
        UpdateStatus(tr("자막이 업데이트되었습니다"));
    } else {
        // Add new subtitle
        subtitleManager->AddSubtitle(title, content);
        UpdateStatus(tr("새 자막이 추가되었습니다"));
        
        // Select the newly added subtitle
        int newIndex = subtitleManager->GetSubtitleCount() - 1;
        if (newIndex >= 0) {
            subtitleListWidget->setCurrentRow(newIndex);
            currentEditingIndex = newIndex;
        }
    }
    
    hasUnsavedChanges = false;
    originalTitle = title;
    originalContent = content;
    
    applyButton->setEnabled(false);
    revertButton->setEnabled(false);
    
    emit ContentModified(false);
}

void SubtitleEditor::RevertChanges()
{
    if (!isEditingMode) return;
    
    titleEdit->setText(originalTitle);
    contentEdit->setPlainText(originalContent);
    
    hasUnsavedChanges = false;
    applyButton->setEnabled(false);
    revertButton->setEnabled(false);
    
    UpdateStatus(tr("변경사항이 취소되었습니다"));
    emit ContentModified(false);
}

// Slot implementations
void SubtitleEditor::OnNewSubtitle()
{
    if (hasUnsavedChanges) {
        int ret = QMessageBox::question(this, tr("변경사항 저장"), 
                                      tr("저장하지 않은 변경사항이 있습니다. 저장하시겠습니까?"),
                                      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Save) {
            SaveCurrentSubtitle();
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }
    
    subtitleListWidget->clearSelection();
    currentEditingIndex = -1;
    originalTitle = "";
    originalContent = "";
    
    titleEdit->clear();
    contentEdit->clear();
    titleEdit->setFocus();
    
    SetEditingMode(true);
    hasUnsavedChanges = false;
    
    UpdateStatus(tr("새 자막 작성 중"));
}

void SubtitleEditor::OnSaveSubtitle()
{
    SaveCurrentSubtitle();
}

void SubtitleEditor::OnDeleteSubtitle()
{
    QListWidgetItem *currentItem = subtitleListWidget->currentItem();
    if (!currentItem || !subtitleManager) return;
    
    int index = currentItem->data(Qt::UserRole).toInt();
    
    int ret = QMessageBox::question(this, tr("자막 삭제"), 
                                  tr("선택한 자막을 삭제하시겠습니까?"),
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        subtitleManager->RemoveSubtitle(index);
        SetEditingMode(false);
        UpdateStatus(tr("자막이 삭제되었습니다"));
    }
}

void SubtitleEditor::OnClearEditor()
{
    if (hasUnsavedChanges) {
        int ret = QMessageBox::question(this, tr("편집기 지우기"), 
                                      tr("저장하지 않은 변경사항이 있습니다. 계속하시겠습니까?"),
                                      QMessageBox::Yes | QMessageBox::No);
        if (ret != QMessageBox::Yes) return;
    }
    
    subtitleListWidget->clearSelection();
    SetEditingMode(false);
}

void SubtitleEditor::OnSubtitleListSelectionChanged()
{
    QListWidgetItem *currentItem = subtitleListWidget->currentItem();
    
    if (!currentItem) {
        SetEditingMode(false);
        return;
    }
    
    if (hasUnsavedChanges) {
        int ret = QMessageBox::question(this, tr("변경사항 저장"), 
                                      tr("저장하지 않은 변경사항이 있습니다. 저장하시겠습니까?"),
                                      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Save) {
            SaveCurrentSubtitle();
        } else if (ret == QMessageBox::Cancel) {
            // Restore previous selection
            return;
        }
    }
    
    int index = currentItem->data(Qt::UserRole).toInt();
    if (!subtitleManager || index < 0 || index >= subtitleManager->GetSubtitleCount()) {
        SetEditingMode(false);
        return;
    }
    
    SubtitleItem item = subtitleManager->GetSubtitle(index);
    
    currentEditingIndex = index;
    originalTitle = item.title;
    originalContent = item.content;
    
    titleEdit->setText(item.title);
    contentEdit->setPlainText(item.content);
    
    hasUnsavedChanges = false;
    SetEditingMode(true);
    
    UpdateStatus(tr("자막 편집 중: %1").arg(item.title.isEmpty() ? tr("(제목 없음)") : item.title));
}

void SubtitleEditor::OnAddSubtitle()
{
    OnNewSubtitle();
}

void SubtitleEditor::OnRemoveSubtitle()
{
    OnDeleteSubtitle();
}

void SubtitleEditor::OnPreviewSubtitle()
{
    if (!subtitleManager || !isEditingMode) return;
    
    QString content = contentEdit->toPlainText().trimmed();
    if (content.isEmpty()) return;
    
    // Temporarily set the content to the target source for preview
    QString originalContent = "";
    int originalIndex = subtitleManager->GetCurrentIndex();
    
    if (originalIndex >= 0) {
        SubtitleItem originalItem = subtitleManager->GetSubtitle(originalIndex);
        originalContent = originalItem.content;
    }
    
    // Create temporary subtitle for preview
    if (currentEditingIndex >= 0) {
        subtitleManager->SetCurrentSubtitle(currentEditingIndex);
        // Temporarily update the content
        subtitleManager->UpdateSubtitle(currentEditingIndex, titleEdit->text(), content);
    }
    
    UpdateStatus(tr("미리보기 중..."));
    
    // Restore after 3 seconds
    QTimer::singleShot(3000, [this, originalIndex]() {
        if (subtitleManager) {
            subtitleManager->SetCurrentSubtitle(originalIndex);
            UpdateStatus(tr("미리보기 완료"));
        }
    });
}

void SubtitleEditor::OnApplyChanges()
{
    SaveCurrentSubtitle();
}

void SubtitleEditor::OnRevertChanges()
{
    RevertChanges();
}

void SubtitleEditor::OnBibleSearch()
{
    // This would open the bible search dialog similar to SubtitleControlPanel
    // For now, just update status
    UpdateStatus(tr("성경 검색 기능은 구현 예정입니다"));
}

void SubtitleEditor::OnHymnSearch()
{
    // This would open the hymn search dialog similar to SubtitleControlPanel  
    // For now, just update status
    UpdateStatus(tr("찬송가 검색 기능은 구현 예정입니다"));
}

void SubtitleEditor::OnFontChanged()
{
    ApplyFormatting();
}

void SubtitleEditor::OnFontSizeChanged()
{
    ApplyFormatting();
}

void SubtitleEditor::OnBoldToggled()
{
    ApplyFormatting();
}

void SubtitleEditor::OnItalicToggled()
{
    ApplyFormatting();
}

void SubtitleEditor::OnUnderlineToggled()
{
    ApplyFormatting();
}

void SubtitleEditor::OnColorButtonClicked()
{
    QColor color = QColorDialog::getColor(currentTextColor, this, tr("텍스트 색상 선택"));
    if (color.isValid()) {
        currentTextColor = color;
        OnTextColorChanged();
    }
}

void SubtitleEditor::OnTextColorChanged()
{
    // Update color button appearance
    QString colorStyle = QString("background-color: %1").arg(currentTextColor.name());
    colorButton->setStyleSheet(colorStyle);
    
    ApplyFormatting();
}

void SubtitleEditor::ApplyFormatting()
{
    if (!isEditingMode || !contentEdit->hasFocus()) return;
    
    QTextCursor cursor = contentEdit->textCursor();
    QTextCharFormat format;
    
    format.setFont(fontComboBox->currentFont());
    format.setFontPointSize(fontSizeSpinBox->value());
    format.setFontWeight(boldButton->isChecked() ? QFont::Bold : QFont::Normal);
    format.setFontItalic(italicButton->isChecked());
    format.setFontUnderline(underlineButton->isChecked());
    format.setForeground(currentTextColor);
    
    cursor.mergeCharFormat(format);
    contentEdit->setTextCursor(cursor);
    contentEdit->setFocus();
}

void SubtitleEditor::OnAutoSave()
{
    if (hasUnsavedChanges && isEditingMode) {
        SaveCurrentSubtitle();
    }
}

void SubtitleEditor::OnContentChanged()
{
    if (!isEditingMode) return;
    
    hasUnsavedChanges = true;
    applyButton->setEnabled(true);
    revertButton->setEnabled(true);
    
    UpdateCharacterCount();
    
    // Restart auto-save timer
    autoSaveTimer->stop();
    autoSaveTimer->start();
    
    emit ContentModified(true);
}

void SubtitleEditor::OnTitleChanged()
{
    if (!isEditingMode) return;
    
    hasUnsavedChanges = true;
    applyButton->setEnabled(true);
    revertButton->setEnabled(true);
    
    emit ContentModified(true);
}

void SubtitleEditor::OnTextEditTextChanged()
{
    UpdateCharacterCount();
}

void SubtitleEditor::UpdateToolbarState()
{
    if (!isEditingMode) return;
    
    QTextCursor cursor = contentEdit->textCursor();
    QTextCharFormat format = cursor.charFormat();
    
    // Update toolbar buttons to match current format
    boldButton->setChecked(format.fontWeight() == QFont::Bold);
    italicButton->setChecked(format.fontItalic());
    underlineButton->setChecked(format.fontUnderline());
    
    if (format.font().family() != fontComboBox->currentFont().family()) {
        fontComboBox->setCurrentFont(format.font());
    }
    
    if (format.fontPointSize() != fontSizeSpinBox->value()) {
        fontSizeSpinBox->setValue(format.fontPointSize());
    }
    
    QColor textColor = format.foreground().color();
    if (textColor.isValid() && textColor != currentTextColor) {
        currentTextColor = textColor;
        QString colorStyle = QString("background-color: %1").arg(currentTextColor.name());
        colorButton->setStyleSheet(colorStyle);
    }
}

// SubtitleManager signal handlers
void SubtitleEditor::OnSubtitleChanged(int index)
{
    // Update preview or current selection if needed
    if (currentEditingIndex == index) {
        UpdateStatus(tr("현재 자막이 변경되었습니다"));
    }
}

void SubtitleEditor::OnSubtitleListChanged()
{
    UpdateSubtitleList();
}

void SubtitleEditor::OnTargetSourceChanged(const QString &sourceName)
{
    QString status = sourceName.isEmpty() ? tr("타겟 소스 없음") : tr("타겟 소스: %1").arg(sourceName);
    UpdateStatus(status);
}

// Public interface methods
void SubtitleEditor::FocusEditor()
{
    if (isEditingMode) {
        contentEdit->setFocus();
    } else {
        subtitleListWidget->setFocus();
    }
}

void SubtitleEditor::SetReadOnly(bool readOnly)
{
    titleEdit->setReadOnly(readOnly);
    contentEdit->setReadOnly(readOnly);
    
    bool enabled = !readOnly;
    toolbar->setEnabled(enabled);
    formatToolbar->setEnabled(enabled);
    addSubtitleButton->setEnabled(enabled);
    removeSubtitleButton->setEnabled(enabled);
}

QString SubtitleEditor::GetCurrentTitle() const
{
    return titleEdit->text();
}

QString SubtitleEditor::GetCurrentContent() const
{
    return contentEdit->toPlainText();
}

void SubtitleEditor::SetCurrentContent(const QString &title, const QString &content)
{
    titleEdit->setText(title);
    contentEdit->setPlainText(content);
    originalTitle = title;
    originalContent = content;
    hasUnsavedChanges = false;
    
    applyButton->setEnabled(false);
    revertButton->setEnabled(false);
}

void SubtitleEditor::SetPreferredSize(const QSize &size)
{
    resize(size);
}

QSize SubtitleEditor::GetPreferredSize() const
{
    return size();
}

// Event handlers
void SubtitleEditor::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    
    // Maintain splitter proportions
    if (mainSplitter && event->size().width() > 0) {
        int leftWidth = qMax(200, event->size().width() / 3);
        int rightWidth = event->size().width() - leftWidth - mainSplitter->handleWidth();
        mainSplitter->setSizes({leftWidth, rightWidth});
    }
}

void SubtitleEditor::closeEvent(QCloseEvent *event)
{
    if (hasUnsavedChanges) {
        int ret = QMessageBox::question(this, tr("자막 편집기 종료"), 
                                      tr("저장하지 않은 변경사항이 있습니다. 저장하시겠습니까?"),
                                      QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Save) {
            SaveCurrentSubtitle();
        } else if (ret == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    
    emit SubtitleEditorClosed();
    QWidget::closeEvent(event);
}

// SubtitleEditorDock implementation
SubtitleEditorDock::SubtitleEditorDock(QWidget *parent)
    : QDockWidget(tr("자막 편집기"), parent)
{
    subtitleEditor = new SubtitleEditor(this);
    setWidget(subtitleEditor);
    
    // Configure dock widget
    setFeatures(QDockWidget::DockWidgetMovable | 
                QDockWidget::DockWidgetFloatable | 
                QDockWidget::DockWidgetClosable);
    
    setMinimumSize(600, 400);
    resize(800, 600);
    
    connect(subtitleEditor, &SubtitleEditor::SubtitleEditorClosed, 
            this, &SubtitleEditorDock::SubtitleEditorDockClosed);
    
    blog(LOG_INFO, "[SubtitleEditorDock] Subtitle editor dock created");
}

SubtitleEditorDock::~SubtitleEditorDock()
{
    blog(LOG_INFO, "[SubtitleEditorDock] Subtitle editor dock destroyed");
}

void SubtitleEditorDock::SetSubtitleManager(SubtitleManager *manager)
{
    if (subtitleEditor) {
        subtitleEditor->SetSubtitleManager(manager);
    }
}

void SubtitleEditorDock::resizeEvent(QResizeEvent *event)
{
    QDockWidget::resizeEvent(event);
    
    // Maintain minimum size constraints
    if (event->size().width() < 600 || event->size().height() < 400) {
        resize(qMax(600, event->size().width()), qMax(400, event->size().height()));
    }
}

void SubtitleEditorDock::closeEvent(QCloseEvent *event)
{
    emit SubtitleEditorDockClosed();
    QDockWidget::closeEvent(event);
}