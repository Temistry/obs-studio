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
    
    // 내용 편집
    contentLabel = new QLabel("자막 내용:", editScrollWidget);
    contentEdit = new QTextEdit(editScrollWidget);
    contentEdit->setPlaceholderText("자막 내용을 입력하세요");
    contentEdit->setMinimumHeight(100);
    contentEdit->setMaximumHeight(200);
    
    // 편집 버튼들
    editButtonLayout = new QHBoxLayout();
    bibleSearchButton = new QPushButton("성경 검색", editScrollWidget);
    hymnSearchButton = new QPushButton("찬송가 검색", editScrollWidget);
    autoSplitButton = new QPushButton("자동분리", editScrollWidget);
    saveButton = new QPushButton("저장", editScrollWidget);
    cancelButton = new QPushButton("취소", editScrollWidget);
    
    // 버튼 아이콘 설정
    saveButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogSaveButton));
    cancelButton->setIcon(QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton));
    
    editButtonLayout->addWidget(bibleSearchButton);
    editButtonLayout->addWidget(hymnSearchButton);
    editButtonLayout->addWidget(autoSplitButton);
    editButtonLayout->addStretch();
    editButtonLayout->addWidget(saveButton);
    editButtonLayout->addWidget(cancelButton);
    
    // 스크롤 위젯에 레이아웃 추가
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
    connect(autoSplitButton, &QPushButton::clicked, this, &SubtitleEditingPanel::OnAutoSplit);
    
    // 내용 변경 감지
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
    
    contentEdit->setEnabled(enabled);
    saveButton->setEnabled(enabled);
    cancelButton->setEnabled(enabled);
    bibleSearchButton->setEnabled(enabled);
    hymnSearchButton->setEnabled(enabled);
    autoSplitButton->setEnabled(enabled);
    
    if (!enabled) {
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
    contentEdit->setPlainText(item.content);
    
    SetEditMode(true, index);
    contentEdit->setFocus();
    
    emit EditingStarted(index);
    blog(LOG_INFO, "[SubtitleEditingPanel] Started editing subtitle at index %d", index);
}

void SubtitleEditingPanel::StartNewSubtitle()
{
    contentEdit->clear();
    
    SetEditMode(true, -1);
    contentEdit->setFocus();
    
    emit EditingStarted(-1);
    blog(LOG_INFO, "[SubtitleEditingPanel] Started creating new subtitle");
}

void SubtitleEditingPanel::StopEditing()
{
    SetEditMode(false);
    emit EditingFinished();
    blog(LOG_INFO, "[SubtitleEditingPanel] Stopped editing");
}

QString SubtitleEditingPanel::GetCurrentContent() const
{
    return contentEdit->toPlainText().trimmed();
}

void SubtitleEditingPanel::SetCurrentContent(const QString &content)
{
    contentEdit->setPlainText(content);
}

void SubtitleEditingPanel::ClearContent()
{
    contentEdit->clear();
}

void SubtitleEditingPanel::OnSaveSubtitle()
{
    if (!subtitleManager || !isEditing) return;
    
    QString content = GetCurrentContent();
    
    if (content.isEmpty()) {
        QMessageBox::warning(this, "경고", "자막 내용이 비어있습니다.");
        contentEdit->setFocus();
        return;
    }
    
    // 내용의 첫 줄을 제목으로 사용 (최대 50자)
    QString title = content.split('\n')[0].left(50).trimmed();
    if (title.length() == 50) {
        title += "...";
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
            // 현재 커서 위치에 텍스트 삽입
            QTextCursor cursor = contentEdit->textCursor();
            cursor.insertText(selectedText);
            contentEdit->setFocus();
        }
    }
}

void SubtitleEditingPanel::OnAutoSplit()
{
    if (!subtitleManager || !isEditing) {
        QMessageBox::information(this, "알림", "편집 중인 자막이 없습니다.");
        return;
    }
    
    QString content = GetCurrentContent();
    if (content.isEmpty()) {
        QMessageBox::information(this, "알림", "분리할 내용이 없습니다.");
        return;
    }
    
    // 빈 줄을 기준으로 내용 분리
    QStringList sections = content.split("\n\n", Qt::SkipEmptyParts);
    
    if (sections.size() <= 1) {
        QMessageBox::information(this, "알림", "분리할 구간이 없습니다.\n빈 줄(엔터 2번)로 구분된 문단이 필요합니다.");
        return;
    }
    
    // 사용자에게 확인 요청
    int ret = QMessageBox::question(this, "자동분리", 
                                   QString("현재 내용을 %1개의 자막으로 분리하시겠습니까?")
                                   .arg(sections.size()),
                                   QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    try {
        // 각 구간의 내용에서 제목을 자동 생성
        for (int i = 0; i < sections.size(); ++i) {
            QString sectionContent = sections[i].trimmed();
            if (sectionContent.isEmpty()) continue;
            
            // 내용의 첫 줄을 제목으로 사용 (최대 50자)
            QString sectionTitle = sectionContent.split('\n')[0].left(50).trimmed();
            if (sectionTitle.length() == 50) {
                sectionTitle += "...";
            }
            
            if (i == 0) {
                // 현재 자막을 첫 번째 구간으로 업데이트
                subtitleManager->UpdateSubtitle(editingIndex, sectionTitle, sectionContent);
            } else {
                // 나머지 구간들을 새로운 자막으로 추가
                subtitleManager->AddSubtitle(sectionTitle, sectionContent);
            }
        }
        
        // 편집 종료
        StopEditing();
        
        QMessageBox::information(this, "자동분리 완료", 
                                QString("%1개의 자막으로 분리되었습니다.")
                                .arg(sections.size()));
        
        blog(LOG_INFO, "[SubtitleEditingPanel] Auto-split completed: %d sections created", sections.size());
        
    } catch (const std::exception &e) {
        blog(LOG_ERROR, "[SubtitleEditingPanel] Error during auto-split: %s", e.what());
        QMessageBox::critical(this, "자동분리 오류", "자동분리 중 오류가 발생했습니다.");
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