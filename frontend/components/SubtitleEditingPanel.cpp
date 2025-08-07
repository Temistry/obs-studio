#include "SubtitleEditingPanel.hpp"
#include "SubtitleControlPanel.hpp" // For dialog classes
#include <QApplication>
#include <QStyle>
#include <QMessageBox>
#include <QCloseEvent>
#include <obs.hpp>
#include <qt-wrappers.hpp>

#include "moc_SubtitleEditingPanel.cpp"

SubtitleEditingPanel::SubtitleEditingPanel(QWidget *parent)
    : QWidget(parent),
      subtitleManager(nullptr),
      editingIndex(-1),
      isEditing(false)
{
    SetupUI();
    ConnectSignals();
    
    // 자동 저장 타이머 설정
    autoSaveTimer = new QTimer(this);
    autoSaveTimer->setSingleShot(true);
    autoSaveTimer->setInterval(5000); // 5초
    connect(autoSaveTimer, &QTimer::timeout, this, &SubtitleEditingPanel::OnAutoSave);
    
    SetEditMode(false);
    
    blog(LOG_INFO, "[SubtitleEditingPanel] Subtitle editing panel initialized");
}

SubtitleEditingPanel::~SubtitleEditingPanel()
{
    blog(LOG_INFO, "[SubtitleEditingPanel] Subtitle editing panel destroyed");
}

void SubtitleEditingPanel::SetupUI()
{
    // 메인 레이아웃
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);
    
    // 자막 편집 그룹박스
    editGroup = new QGroupBox("자막 편집", this);
    editLayout = new QVBoxLayout(editGroup);
    
    // 스크롤 영역 생성
    editScrollArea = new QScrollArea(editGroup);
    editScrollWidget = new QWidget();
    editScrollLayout = new QVBoxLayout(editScrollWidget);
    
    // 제목 편집
    titleLayout = new QHBoxLayout();
    titleLabel = new QLabel("제목:", editScrollWidget);
    titleEdit = new QLineEdit(editScrollWidget);
    titleEdit->setPlaceholderText("자막 제목을 입력하세요");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(titleEdit);
    
    // 내용 편집
    contentLabel = new QLabel("내용:", editScrollWidget);
    contentEdit = new QTextEdit(editScrollWidget);
    contentEdit->setPlaceholderText("자막 내용을 입력하세요");
    contentEdit->setMinimumHeight(100);
    contentEdit->setMaximumHeight(200);
    
    // 편집 버튼들
    editButtonLayout = new QHBoxLayout();
    bibleSearchButton = new QPushButton("성경 검색", editScrollWidget);
    hymnSearchButton = new QPushButton("찬송가 검색", editScrollWidget);
    saveButton = new QPushButton("저장", editScrollWidget);
    cancelButton = new QPushButton("취소", editScrollWidget);
    
    // 버튼 아이콘 설정
    saveButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
    cancelButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
    
    editButtonLayout->addWidget(bibleSearchButton);
    editButtonLayout->addWidget(hymnSearchButton);
    editButtonLayout->addStretch();
    editButtonLayout->addWidget(saveButton);
    editButtonLayout->addWidget(cancelButton);
    
    // 스크롤 위젯에 레이아웃 추가
    editScrollLayout->addLayout(titleLayout);
    editScrollLayout->addWidget(contentLabel);
    editScrollLayout->addWidget(contentEdit);
    editScrollLayout->addLayout(editButtonLayout);
    editScrollLayout->addStretch();
    
    // 스크롤 영역 설정
    editScrollArea->setWidget(editScrollWidget);
    editScrollArea->setWidgetResizable(true);
    editScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    editScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    editScrollArea->setMinimumHeight(200);
    
    // 메인 레이아웃에 추가
    editLayout->addWidget(editScrollArea);
    mainLayout->addWidget(editGroup);
    
    // 크기 정책 설정
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    setMinimumWidth(300);
    setMinimumHeight(250);
}

void SubtitleEditingPanel::ConnectSignals()
{
    connect(saveButton, &QPushButton::clicked, this, &SubtitleEditingPanel::OnSaveSubtitle);
    connect(cancelButton, &QPushButton::clicked, this, &SubtitleEditingPanel::OnCancelEdit);
    connect(bibleSearchButton, &QPushButton::clicked, this, &SubtitleEditingPanel::OnBibleSearch);
    connect(hymnSearchButton, &QPushButton::clicked, this, &SubtitleEditingPanel::OnHymnSearch);
    
    // 내용 변경 감지
    connect(titleEdit, &QLineEdit::textChanged, this, &SubtitleEditingPanel::ContentChanged);
    connect(contentEdit, &QTextEdit::textChanged, this, &SubtitleEditingPanel::ContentChanged);
    connect(contentEdit, &QTextEdit::textChanged, [this]() {
        autoSaveTimer->stop();
        autoSaveTimer->start();
    });
}

void SubtitleEditingPanel::SetSubtitleManager(SubtitleManager *manager)
{
    if (subtitleManager) {
        disconnect(subtitleManager, nullptr, this, nullptr);
    }
    
    subtitleManager = manager;
    
    if (subtitleManager) {
        connect(subtitleManager, &SubtitleManager::SubtitleChanged, this, &SubtitleEditingPanel::OnSubtitleChanged);
        connect(subtitleManager, &SubtitleManager::SubtitleListChanged, this, &SubtitleEditingPanel::OnSubtitleListChanged);
        
        blog(LOG_INFO, "[SubtitleEditingPanel] Subtitle manager connected");
    }
}

void SubtitleEditingPanel::SetEditMode(bool enabled, int index)
{
    isEditing = enabled;
    editingIndex = index;
    
    titleEdit->setEnabled(enabled);
    contentEdit->setEnabled(enabled);
    saveButton->setEnabled(enabled);
    cancelButton->setEnabled(enabled);
    bibleSearchButton->setEnabled(enabled);
    hymnSearchButton->setEnabled(enabled);
    
    if (!enabled) {
        titleEdit->clear();
        contentEdit->clear();
        editingIndex = -1;
        editGroup->setTitle("자막 편집");
    } else {
        if (index >= 0) {
            editGroup->setTitle(QString("자막 편집 (항목 %1)").arg(index + 1));
        } else {
            editGroup->setTitle("새 자막 편집");
        }
    }
    
    emit ContentChanged();
}

void SubtitleEditingPanel::StartEditing(int index)
{
    if (!subtitleManager || index < 0 || index >= subtitleManager->GetSubtitleCount()) {
        return;
    }
    
    SubtitleItem item = subtitleManager->GetSubtitle(index);
    titleEdit->setText(item.title);
    contentEdit->setPlainText(item.content);
    
    SetEditMode(true, index);
    titleEdit->setFocus();
    
    emit EditingStarted(index);
    blog(LOG_INFO, "[SubtitleEditingPanel] Started editing subtitle at index %d", index);
}

void SubtitleEditingPanel::StartNewSubtitle()
{
    titleEdit->clear();
    contentEdit->clear();
    
    SetEditMode(true, -1);
    titleEdit->setFocus();
    
    emit EditingStarted(-1);
    blog(LOG_INFO, "[SubtitleEditingPanel] Started creating new subtitle");
}

void SubtitleEditingPanel::StopEditing()
{
    SetEditMode(false);
    emit EditingFinished();
    blog(LOG_INFO, "[SubtitleEditingPanel] Stopped editing");
}

QString SubtitleEditingPanel::GetCurrentTitle() const
{
    return titleEdit->text().trimmed();
}

QString SubtitleEditingPanel::GetCurrentContent() const
{
    return contentEdit->toPlainText().trimmed();
}

void SubtitleEditingPanel::SetCurrentContent(const QString &title, const QString &content)
{
    titleEdit->setText(title);
    contentEdit->setPlainText(content);
}

void SubtitleEditingPanel::ClearContent()
{
    titleEdit->clear();
    contentEdit->clear();
}

void SubtitleEditingPanel::OnSaveSubtitle()
{
    if (!subtitleManager || !isEditing) return;
    
    QString title = GetCurrentTitle();
    QString content = GetCurrentContent();
    
    if (content.isEmpty()) {
        QMessageBox::warning(this, "경고", "자막 내용이 비어있습니다.");
        contentEdit->setFocus();
        return;
    }
    
    try {
        if (editingIndex >= 0) {
            // 기존 자막 수정
            subtitleManager->UpdateSubtitle(editingIndex, title, content);
            blog(LOG_INFO, "[SubtitleEditingPanel] Updated subtitle at index %d", editingIndex);
        } else {
            // 새 자막 추가
            subtitleManager->AddSubtitle(title, content);
            editingIndex = subtitleManager->GetSubtitleCount() - 1;
            blog(LOG_INFO, "[SubtitleEditingPanel] Added new subtitle at index %d", editingIndex);
        }
        
        emit SubtitleSaved(editingIndex);
        StopEditing();
        
        QMessageBox::information(this, "저장 완료", "자막이 성공적으로 저장되었습니다.");
        
    } catch (const std::exception &e) {
        blog(LOG_ERROR, "[SubtitleEditingPanel] Error saving subtitle: %s", e.what());
        QMessageBox::critical(this, "저장 오류", "자막 저장 중 오류가 발생했습니다.");
    }
}

void SubtitleEditingPanel::OnCancelEdit()
{
    if (isEditing) {
        int ret = QMessageBox::question(this, "편집 취소", 
                                      "편집 중인 내용이 있습니다. 정말 취소하시겠습니까?",
                                      QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            StopEditing();
            emit EditingCancelled();
            blog(LOG_INFO, "[SubtitleEditingPanel] Editing cancelled by user");
        }
    }
}

void SubtitleEditingPanel::OnBibleSearch()
{
    if (!subtitleManager) return;
    
    // 성경 검색 다이얼로그 열기
    BibleSearchDialog dialog(subtitleManager, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString selectedText = dialog.GetSelectedText();
        QString selectedTitle = dialog.GetSelectedTitle();
        
        if (!selectedText.isEmpty()) {
            // 제목이 비어있다면 성경 구절 제목을 사용
            if (titleEdit->text().trimmed().isEmpty()) {
                titleEdit->setText(selectedTitle);
            }
            
            // 현재 커서 위치에 텍스트 삽입
            QTextCursor cursor = contentEdit->textCursor();
            cursor.insertText(selectedText);
            contentEdit->setFocus();
        }
    }
}

void SubtitleEditingPanel::OnHymnSearch()
{
    // 찬송가 검색 다이얼로그 열기
    HymnSearchDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString selectedText = dialog.GetSelectedText();
        QString selectedTitle = dialog.GetSelectedTitle();
        
        if (!selectedText.isEmpty()) {
            // 제목이 비어있다면 찬송가 제목을 사용
            if (titleEdit->text().trimmed().isEmpty()) {
                titleEdit->setText(selectedTitle);
            }
            
            // 현재 커서 위치에 텍스트 삽입
            QTextCursor cursor = contentEdit->textCursor();
            cursor.insertText(selectedText);
            contentEdit->setFocus();
        }
    }
}

void SubtitleEditingPanel::OnAutoSave()
{
    if (isEditing && !GetCurrentContent().isEmpty()) {
        OnSaveSubtitle();
        blog(LOG_INFO, "[SubtitleEditingPanel] Auto-saved subtitle");
    }
}

void SubtitleEditingPanel::OnSubtitleChanged(int index)
{
    // 현재 편집 중인 자막이 외부에서 변경된 경우 알림
    if (isEditing && editingIndex == index) {
        blog(LOG_INFO, "[SubtitleEditingPanel] Currently editing subtitle was changed externally");
    }
}

void SubtitleEditingPanel::OnSubtitleListChanged()
{
    // 자막 리스트가 변경된 경우 편집 인덱스 유효성 검사
    if (isEditing && subtitleManager) {
        if (editingIndex >= subtitleManager->GetSubtitleCount()) {
            blog(LOG_WARNING, "[SubtitleEditingPanel] Editing index out of range, stopping edit");
            StopEditing();
        }
    }
}

// SubtitleEditingDock 구현
SubtitleEditingDock::SubtitleEditingDock(QWidget *parent)
    : QDockWidget("자막 편집", parent)
{
    editingPanel = new SubtitleEditingPanel(this);
    setWidget(editingPanel);
    
    // 도킹 설정
    setFeatures(QDockWidget::DockWidgetMovable | 
                QDockWidget::DockWidgetFloatable | 
                QDockWidget::DockWidgetClosable);
    
    setMinimumSize(320, 280);
    resize(400, 350);
    
    connect(editingPanel, &SubtitleEditingPanel::EditingStarted, 
            [](int index) {
                blog(LOG_INFO, "[SubtitleEditingDock] Editing started for index %d", index);
            });
    
    connect(editingPanel, &SubtitleEditingPanel::EditingFinished, 
            []() {
                blog(LOG_INFO, "[SubtitleEditingDock] Editing finished");
            });
    
    blog(LOG_INFO, "[SubtitleEditingDock] Subtitle editing dock created");
}

void SubtitleEditingDock::SetSubtitleManager(SubtitleManager *manager)
{
    if (editingPanel) {
        editingPanel->SetSubtitleManager(manager);
    }
}

void SubtitleEditingDock::closeEvent(QCloseEvent *event)
{
    emit EditingDockClosed();
    QDockWidget::closeEvent(event);
}