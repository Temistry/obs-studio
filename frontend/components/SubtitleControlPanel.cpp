#include "SubtitleControlPanel.hpp"
#include <QFileDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QCloseEvent>
#include <obs.hpp>
#include <qt-wrappers.hpp>
#include "moc_SubtitleControlPanel.cpp"

SubtitleControlPanel::SubtitleControlPanel(QWidget *parent)
    : QWidget(parent), subtitleManager(nullptr), editingIndex(-1)
{
    subtitleManager = new SubtitleManager(this);
    SetupUI();
    
    // SubtitleManager 시그널 연결
    connect(subtitleManager, &SubtitleManager::SubtitleChanged, 
            this, &SubtitleControlPanel::OnSubtitleChanged);
    connect(subtitleManager, &SubtitleManager::SubtitleListChanged, 
            this, &SubtitleControlPanel::OnSubtitleListChanged);
    connect(subtitleManager, &SubtitleManager::TargetSourceChanged, 
            this, &SubtitleControlPanel::OnTargetSourceChanged);
    connect(subtitleManager, &SubtitleManager::WorshipFoldersChanged, 
            this, &SubtitleControlPanel::OnWorshipFoldersChanged);
    connect(subtitleManager, &SubtitleManager::CurrentFolderChanged, 
            this, &SubtitleControlPanel::OnCurrentFolderChanged);
    
    // 초기 상태 설정
    RefreshSourceList();
    RefreshFolderTree();
    RefreshSubtitleList();
    RefreshQuickButtons();
    UpdateCurrentLabel();
    SetEditMode(false);
    
    setWindowTitle("자막 전환 컨트롤");
    resize(650, 700);  // 스크롤을 고려한 크기 조정
    setMinimumSize(500, 400);  // 최소 크기 설정
    setAttribute(Qt::WA_DeleteOnClose, false);
}

SubtitleControlPanel::~SubtitleControlPanel()
{
}

void SubtitleControlPanel::SetupUI()
{
    mainLayout = new QVBoxLayout(this);
    
    // 메인 스크롤 영역 생성
    mainScrollArea = new QScrollArea(this);
    mainScrollWidget = new QWidget();
    
    mainSplitter = new QSplitter(Qt::Vertical, mainScrollWidget);
    
    // 메인 스크롤 위젯에 스플리터 추가
    QVBoxLayout *scrollLayout = new QVBoxLayout(mainScrollWidget);
    scrollLayout->addWidget(mainSplitter);
    scrollLayout->setContentsMargins(0, 0, 0, 0);
    
    // 스크롤 영역 설정
    mainScrollArea->setWidget(mainScrollWidget);
    mainScrollArea->setWidgetResizable(true);
    mainScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mainScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    mainLayout->addWidget(mainScrollArea);
    
    // 타겟 소스 선택
    sourceGroup = new QGroupBox("타겟 텍스트 소스", this);
    sourceLayout = new QHBoxLayout(sourceGroup);
    sourceLabel = new QLabel("소스:", sourceGroup);
    sourceComboBox = new QComboBox(sourceGroup);
    refreshSourceButton = new QPushButton("새로고침", sourceGroup);
    
    sourceLayout->addWidget(sourceLabel);
    sourceLayout->addWidget(sourceComboBox, 1);
    sourceLayout->addWidget(refreshSourceButton);
    
    // 연결 상태 표시 레이블 추가
    QLabel *statusLabel = new QLabel("상태: 연결안됨", sourceGroup);
    statusLabel->setObjectName("connectionStatus");
    statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
    sourceLayout->addWidget(statusLabel);
    
    connect(sourceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SubtitleControlPanel::OnSourceChanged);
    connect(refreshSourceButton, &QPushButton::clicked,
            this, &SubtitleControlPanel::OnRefreshSource);
    
    mainSplitter->addWidget(sourceGroup);
    
    // 예배 폴더 관리
    folderGroup = new QGroupBox("예배 폴더", this);
    folderLayout = new QVBoxLayout(folderGroup);
    folderTreeWidget = new QTreeWidget(folderGroup);
    folderButtonLayout = new QHBoxLayout();
    
    folderTreeWidget->setHeaderLabels(QStringList() << "예배 폴더");
    folderTreeWidget->setRootIsDecorated(false);
    folderTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    
    addFolderButton = new QPushButton("폴더 추가", folderGroup);
    editFolderButton = new QPushButton("폴더 편집", folderGroup);
    removeFolderButton = new QPushButton("폴더 삭제", folderGroup);
    
    folderButtonLayout->addWidget(addFolderButton);
    folderButtonLayout->addWidget(editFolderButton);
    folderButtonLayout->addWidget(removeFolderButton);
    folderButtonLayout->addStretch();
    
    folderLayout->addWidget(folderTreeWidget);
    folderLayout->addLayout(folderButtonLayout);
    
    connect(folderTreeWidget, &QTreeWidget::itemSelectionChanged,
            this, &SubtitleControlPanel::OnFolderSelectionChanged);
    connect(addFolderButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnAddFolder);
    connect(editFolderButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnEditFolder);
    connect(removeFolderButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnRemoveFolder);
    
    mainSplitter->addWidget(folderGroup);
    
    // 자막 리스트 관리
    listGroup = new QGroupBox("자막 리스트", this);
    listLayout = new QVBoxLayout(listGroup);
    subtitleList = new QListWidget(listGroup);
    listButtonLayout = new QHBoxLayout();
    
    addButton = new QPushButton("추가", listGroup);
    editButton = new QPushButton("편집", listGroup);
    removeButton = new QPushButton("삭제", listGroup);
    clearButton = new QPushButton("전체삭제", listGroup);
    importButton = new QPushButton("가져오기", listGroup);
    exportButton = new QPushButton("내보내기", listGroup);
    
    listButtonLayout->addWidget(addButton);
    listButtonLayout->addWidget(editButton);
    listButtonLayout->addWidget(removeButton);
    listButtonLayout->addWidget(clearButton);
    listButtonLayout->addStretch();
    listButtonLayout->addWidget(importButton);
    listButtonLayout->addWidget(exportButton);
    
    listLayout->addWidget(subtitleList);
    listLayout->addLayout(listButtonLayout);
    
    connect(subtitleList, &QListWidget::itemSelectionChanged,
            this, &SubtitleControlPanel::OnSubtitleSelectionChanged);
    connect(subtitleList, &QListWidget::itemDoubleClicked,
            this, &SubtitleControlPanel::OnEditSubtitle);
    connect(addButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnAddSubtitle);
    connect(editButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnEditSubtitle);
    connect(removeButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnRemoveSubtitle);
    connect(clearButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnClearSubtitles);
    connect(importButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnImportSubtitles);
    connect(exportButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnExportSubtitles);
    
    mainSplitter->addWidget(listGroup);
    
    // 자막 편집
    editGroup = new QGroupBox("자막 편집", this);
    editLayout = new QVBoxLayout(editGroup);
    
    // 스크롤 영역 생성
    editScrollArea = new QScrollArea(editGroup);
    editScrollWidget = new QWidget();
    editScrollLayout = new QVBoxLayout(editScrollWidget);
    
    titleLayout = new QHBoxLayout();
    titleLabel = new QLabel("제목:", editScrollWidget);
    titleEdit = new QLineEdit(editScrollWidget);
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(titleEdit);
    
    contentLabel = new QLabel("내용:", editScrollWidget);
    contentEdit = new QTextEdit(editScrollWidget);
    contentEdit->setMinimumHeight(100);  // 최소 높이 설정
    contentEdit->setMaximumHeight(200);  // 최대 높이 증가
    
    editButtonLayout = new QHBoxLayout();
    saveButton = new QPushButton("저장", editScrollWidget);
    cancelButton = new QPushButton("취소", editScrollWidget);
    bibleSearchButton = new QPushButton("성경 검색", editScrollWidget);
    editButtonLayout->addWidget(bibleSearchButton);
    editButtonLayout->addStretch();
    editButtonLayout->addWidget(saveButton);
    editButtonLayout->addWidget(cancelButton);
    
    // 스크롤 위젯에 레이아웃 추가
    editScrollLayout->addLayout(titleLayout);
    editScrollLayout->addWidget(contentLabel);
    editScrollLayout->addWidget(contentEdit);
    editScrollLayout->addLayout(editButtonLayout);
    editScrollLayout->addStretch();  // 여백 추가
    
    // 스크롤 영역 설정
    editScrollArea->setWidget(editScrollWidget);
    editScrollArea->setWidgetResizable(true);
    editScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    editScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    editScrollArea->setMinimumHeight(200);  // 스크롤 영역 최소 높이
    
    // 메인 레이아웃에 스크롤 영역 추가
    editLayout->addWidget(editScrollArea);
    
    connect(saveButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnSaveSubtitle);
    connect(cancelButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnCancelEdit);
    connect(bibleSearchButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnBibleSearch);
    
    mainSplitter->addWidget(editGroup);
    
    // 빠른 전환 컨트롤
    controlGroup = new QGroupBox("전환 컨트롤", this);
    controlLayout = new QVBoxLayout(controlGroup);
    currentLabel = new QLabel("현재: 없음", controlGroup);
    quickButtonLayout = new QHBoxLayout();
    
    prevButton = new QPushButton("이전", controlGroup);
    clearCurrentButton = new QPushButton("지우기", controlGroup);
    nextButton = new QPushButton("다음", controlGroup);
    
    quickButtonLayout->addWidget(prevButton);
    quickButtonLayout->addWidget(clearCurrentButton);
    quickButtonLayout->addWidget(nextButton);
    
    controlLayout->addWidget(currentLabel);
    controlLayout->addLayout(quickButtonLayout);
    
    connect(prevButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnPreviousSubtitle);
    connect(clearCurrentButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnClearCurrent);
    connect(nextButton, &QPushButton::clicked, this, &SubtitleControlPanel::OnNextSubtitle);
    
    mainSplitter->addWidget(controlGroup);
    
    // 빠른 액세스 버튼들
    quickGroup = new QGroupBox("빠른 전환", this);
    quickLayout = new QGridLayout(quickGroup);
    
    // 3x4 그리드로 12개 버튼 배치
    for (int i = 0; i < 12; ++i) {
        QPushButton *button = new QPushButton(QString::number(i + 1), quickGroup);
        button->setMinimumHeight(40);
        button->setProperty("subtitleIndex", i);
        connect(button, &QPushButton::clicked, this, &SubtitleControlPanel::OnQuickButtonClicked);
        quickButtons.append(button);
        quickLayout->addWidget(button, i / 4, i % 4);
    }
    
    mainSplitter->addWidget(quickGroup);
    
    // 스플리터 크기 비율 설정 (스크롤 환경에 맞게 조정)
    mainSplitter->setSizes({80, 150, 180, 200, 80, 150});
    
    // 메인 스크롤 위젯의 최소 크기 설정
    mainScrollWidget->setMinimumSize(600, 1000);
}

void SubtitleControlPanel::RefreshSourceList()
{
    sourceComboBox->clear();
    sourceComboBox->addItem("", "");
    
    obs_enum_sources([](void *data, obs_source_t *source) {
        QComboBox *combo = static_cast<QComboBox*>(data);
        const char *id = obs_source_get_id(source);
        const char *name = obs_source_get_name(source);
        
        // 텍스트 소스만 추가 (더 많은 타입 지원)
        if (strcmp(id, "text_gdiplus") == 0 || 
            strcmp(id, "text_ft2_source") == 0 ||
            strcmp(id, "text_pango_source") == 0 ||
            strstr(id, "text") != nullptr) {
            combo->addItem(QString::fromUtf8(name), QString::fromUtf8(name));
        }
        return true;
    }, sourceComboBox);
    
    // 현재 타겟 소스 선택
    QString targetSource = subtitleManager->GetTargetSource();
    if (!targetSource.isEmpty()) {
        int index = sourceComboBox->findData(targetSource);
        if (index >= 0) {
            sourceComboBox->setCurrentIndex(index);
        }
    }
}

void SubtitleControlPanel::RefreshFolderTree()
{
    folderTreeWidget->clear();
    
    QList<WorshipFolder> folders = subtitleManager->GetAllWorshipFolders();
    QString currentFolderId = subtitleManager->GetCurrentFolderId();
    
    for (const WorshipFolder &folder : folders) {
        QTreeWidgetItem *item = new QTreeWidgetItem(folderTreeWidget);
        item->setText(0, folder.displayName);
        item->setData(0, Qt::UserRole, folder.id);
        
        // 현재 선택된 폴더 표시
        if (folder.id == currentFolderId) {
            item->setSelected(true);
            item->setBackground(0, QColor(200, 230, 255));
        }
        
        // 자막 개수 표시
        item->setToolTip(0, QString("자막 %1개\n생성일: %2\n수정일: %3")
                               .arg(folder.subtitles.size())
                               .arg(folder.createdDate.toString("yyyy-MM-dd hh:mm"))
                               .arg(folder.modifiedDate.toString("yyyy-MM-dd hh:mm")));
    }
    
    // 버튼 상태 업데이트
    QList<QTreeWidgetItem*> selected = folderTreeWidget->selectedItems();
    editFolderButton->setEnabled(!selected.isEmpty());
    removeFolderButton->setEnabled(!selected.isEmpty());
}

void SubtitleControlPanel::RefreshSubtitleList()
{
    subtitleList->clear();
    
    QList<SubtitleItem> subtitles = subtitleManager->GetAllSubtitles();
    for (int i = 0; i < subtitles.size(); ++i) {
        const SubtitleItem &item = subtitles[i];
        QString displayText = QString("%1. %2").arg(i + 1).arg(item.title);
        if (!item.enabled) {
            displayText += " (비활성)";
        }
        subtitleList->addItem(displayText);
    }
}

void SubtitleControlPanel::RefreshQuickButtons()
{
    QList<SubtitleItem> subtitles = subtitleManager->GetAllSubtitles();
    
    for (int i = 0; i < quickButtons.size(); ++i) {
        QPushButton *button = quickButtons[i];
        
        if (i < subtitles.size()) {
            const SubtitleItem &item = subtitles[i];
            QString buttonText = QString("%1\n%2").arg(i + 1).arg(item.title);
            button->setText(buttonText);
            button->setEnabled(item.enabled);
            button->setToolTip(item.content);
        } else {
            button->setText(QString::number(i + 1));
            button->setEnabled(false);
            button->setToolTip("");
        }
    }
}

void SubtitleControlPanel::UpdateCurrentLabel()
{
    int currentIndex = subtitleManager->GetCurrentIndex();
    if (currentIndex >= 0) {
        SubtitleItem item = subtitleManager->GetSubtitle(currentIndex);
        currentLabel->setText(QString("현재: %1. %2").arg(currentIndex + 1).arg(item.title));
        
        // 빠른 액세스 버튼 하이라이트
        for (int i = 0; i < quickButtons.size(); ++i) {
            quickButtons[i]->setChecked(i == currentIndex);
        }
    } else {
        currentLabel->setText("현재: 없음");
        for (QPushButton *button : quickButtons) {
            button->setChecked(false);
        }
    }
}

void SubtitleControlPanel::SetEditMode(bool enabled, int index)
{
    editingIndex = index;
    editGroup->setEnabled(enabled);
    
    // 성경 검색 버튼은 편집 모드일 때만 활성화하고, 성경 데이터가 로드되었을 때만 활성화
    bibleSearchButton->setEnabled(enabled && subtitleManager->IsBibleDataLoaded());
    
    if (enabled && index >= 0) {
        SubtitleItem item = subtitleManager->GetSubtitle(index);
        titleEdit->setText(item.title);
        contentEdit->setPlainText(item.content);
        titleEdit->setFocus();
    } else if (enabled) {
        // 새 자막 추가 모드
        titleEdit->clear();
        contentEdit->clear();
        titleEdit->setFocus();
    } else {
        titleEdit->clear();
        contentEdit->clear();
    }
}

void SubtitleControlPanel::OnSourceChanged()
{
    QString sourceName = sourceComboBox->currentData().toString();
    subtitleManager->SetTargetSource(sourceName);
}

void SubtitleControlPanel::OnRefreshSource()
{
    RefreshSourceList();
}

void SubtitleControlPanel::OnFolderSelectionChanged()
{
    QList<QTreeWidgetItem*> selected = folderTreeWidget->selectedItems();
    editFolderButton->setEnabled(!selected.isEmpty());
    removeFolderButton->setEnabled(!selected.isEmpty());
    
    if (!selected.isEmpty()) {
        QString folderId = selected.first()->data(0, Qt::UserRole).toString();
        subtitleManager->SetCurrentFolder(folderId);
    }
}

void SubtitleControlPanel::OnAddFolder()
{
    WorshipFolderDialog dialog(this);
    dialog.SetData(QDate::currentDate().toString("yyyy-MM-dd"), "");
    
    if (dialog.exec() == QDialog::Accepted) {
        QString date = dialog.GetDate();
        QString theme = dialog.GetTheme();
        
        if (!theme.isEmpty()) {
            QString folderId = subtitleManager->CreateWorshipFolder(date, theme);
            subtitleManager->SetCurrentFolder(folderId);
        }
    }
}

void SubtitleControlPanel::OnEditFolder()
{
    QList<QTreeWidgetItem*> selected = folderTreeWidget->selectedItems();
    if (selected.isEmpty()) return;
    
    QString folderId = selected.first()->data(0, Qt::UserRole).toString();
    WorshipFolder folder = subtitleManager->GetWorshipFolder(folderId);
    
    WorshipFolderDialog dialog(this);
    dialog.SetData(folder.date, folder.theme);
    
    if (dialog.exec() == QDialog::Accepted) {
        QString date = dialog.GetDate();
        QString theme = dialog.GetTheme();
        
        if (!theme.isEmpty()) {
            subtitleManager->UpdateWorshipFolder(folderId, date, theme);
        }
    }
}

void SubtitleControlPanel::OnRemoveFolder()
{
    QList<QTreeWidgetItem*> selected = folderTreeWidget->selectedItems();
    if (selected.isEmpty()) return;
    
    QString folderId = selected.first()->data(0, Qt::UserRole).toString();
    WorshipFolder folder = subtitleManager->GetWorshipFolder(folderId);
    
    int ret = QMessageBox::question(this, "폴더 삭제", 
                                   QString("'%1' 폴더를 삭제하시겠습니까?\n폴더 안의 모든 자막도 함께 삭제됩니다.")
                                   .arg(folder.displayName),
                                   QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        subtitleManager->RemoveWorshipFolder(folderId);
    }
}

void SubtitleControlPanel::OnSubtitleSelectionChanged()
{
    editButton->setEnabled(subtitleList->currentRow() >= 0);
    removeButton->setEnabled(subtitleList->currentRow() >= 0);
}

void SubtitleControlPanel::OnAddSubtitle()
{
    // 현재 폴더가 선택되어 있는지 확인
    if (subtitleManager->GetCurrentFolderId().isEmpty()) {
        QMessageBox::information(this, "알림", "먼저 예배 폴더를 선택하거나 생성해주세요.");
        return;
    }
    SetEditMode(true, -1);
}

void SubtitleControlPanel::OnEditSubtitle()
{
    int row = subtitleList->currentRow();
    if (row >= 0) {
        SetEditMode(true, row);
    }
}

void SubtitleControlPanel::OnRemoveSubtitle()
{
    int row = subtitleList->currentRow();
    if (row >= 0) {
        SubtitleItem item = subtitleManager->GetSubtitle(row);
        int ret = QMessageBox::question(this, "자막 삭제", 
                                       QString("'%1' 자막을 삭제하시겠습니까?").arg(item.title),
                                       QMessageBox::Yes | QMessageBox::No);
        if (ret == QMessageBox::Yes) {
            if (!subtitleManager->GetCurrentFolderId().isEmpty()) {
                subtitleManager->RemoveSubtitleFromCurrentFolder(row);
            } else {
                subtitleManager->RemoveSubtitle(row);
            }
        }
    }
}

void SubtitleControlPanel::OnClearSubtitles()
{
    int ret = QMessageBox::question(this, "전체 삭제", 
                                   "모든 자막을 삭제하시겠습니까?",
                                   QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (!subtitleManager->GetCurrentFolderId().isEmpty()) {
            subtitleManager->ClearCurrentFolderSubtitles();
        } else {
            subtitleManager->ClearSubtitles();
        }
    }
}

void SubtitleControlPanel::OnImportSubtitles()
{
    QString fileName = QFileDialog::getOpenFileName(this, "자막 파일 가져오기", 
                                                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
                                                   "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        subtitleManager->ImportFromFile(fileName);
    }
}

void SubtitleControlPanel::OnExportSubtitles()
{
    QString fileName = QFileDialog::getSaveFileName(this, "자막 파일 내보내기", 
                                                   QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/subtitles.json",
                                                   "JSON Files (*.json)");
    if (!fileName.isEmpty()) {
        subtitleManager->ExportToFile(fileName);
    }
}

void SubtitleControlPanel::OnSaveSubtitle()
{
    QString title = titleEdit->text().trimmed();
    QString content = contentEdit->toPlainText().trimmed();
    
    if (title.isEmpty()) {
        QMessageBox::warning(this, "경고", "제목을 입력해주세요.");
        return;
    }
    
    if (editingIndex >= 0) {
        // 기존 자막 수정
        if (!subtitleManager->GetCurrentFolderId().isEmpty()) {
            subtitleManager->UpdateSubtitleInCurrentFolder(editingIndex, title, content);
        } else {
            subtitleManager->UpdateSubtitle(editingIndex, title, content);
        }
    } else {
        // 새 자막 추가
        if (!subtitleManager->GetCurrentFolderId().isEmpty()) {
            subtitleManager->AddSubtitleToCurrentFolder(title, content);
        } else {
            subtitleManager->AddSubtitle(title, content);
        }
    }
    
    SetEditMode(false);
}

void SubtitleControlPanel::OnCancelEdit()
{
    SetEditMode(false);
}

void SubtitleControlPanel::OnBibleSearch()
{
    if (!subtitleManager->IsBibleDataLoaded()) {
        QMessageBox::warning(this, "경고", "성경 데이터가 로드되지 않았습니다.");
        return;
    }
    
    BibleSearchDialog dialog(subtitleManager, this);
    if (dialog.exec() == QDialog::Accepted) {
        QString title = dialog.GetSelectedTitle();
        QString content = dialog.GetSelectedText();
        
        if (!title.isEmpty() && !content.isEmpty()) {
            titleEdit->setText(title);
            contentEdit->setPlainText(content);
        }
    }
}

void SubtitleControlPanel::OnPreviousSubtitle()
{
    subtitleManager->PreviousSubtitle();
}

void SubtitleControlPanel::OnNextSubtitle()
{
    subtitleManager->NextSubtitle();
}

void SubtitleControlPanel::OnClearCurrent()
{
    subtitleManager->ClearCurrentSubtitle();
}

void SubtitleControlPanel::OnQuickButtonClicked()
{
    QPushButton *button = qobject_cast<QPushButton*>(sender());
    if (button) {
        int index = button->property("subtitleIndex").toInt();
        if (index < subtitleManager->GetSubtitleCount()) {
            subtitleManager->SetCurrentSubtitle(index);
        }
    }
}

void SubtitleControlPanel::OnSubtitleChanged(int index)
{
    UpdateCurrentLabel();
}

void SubtitleControlPanel::OnSubtitleListChanged()
{
    RefreshSubtitleList();
    RefreshQuickButtons();
    OnSubtitleSelectionChanged();
}

void SubtitleControlPanel::OnTargetSourceChanged(const QString &sourceName)
{
    // 소스 콤보박스 업데이트
    int index = sourceComboBox->findData(sourceName);
    if (index >= 0) {
        sourceComboBox->setCurrentIndex(index);
    }
    
    // 연결 상태 업데이트
    QLabel *statusLabel = findChild<QLabel*>("connectionStatus");
    if (statusLabel) {
        if (sourceName.isEmpty()) {
            statusLabel->setText("상태: 연결안됨");
            statusLabel->setStyleSheet("QLabel { color: red; font-weight: bold; }");
        } else {
            OBSSourceAutoRelease source = obs_get_source_by_name(sourceName.toUtf8().constData());
            if (source) {
                statusLabel->setText("상태: 연결됨");
                statusLabel->setStyleSheet("QLabel { color: green; font-weight: bold; }");
            } else {
                statusLabel->setText("상태: 소스없음");
                statusLabel->setStyleSheet("QLabel { color: orange; font-weight: bold; }");
            }
        }
    }
}

void SubtitleControlPanel::OnWorshipFoldersChanged()
{
    RefreshFolderTree();
}

void SubtitleControlPanel::OnCurrentFolderChanged(const QString &folderId)
{
    RefreshFolderTree();
    RefreshSubtitleList();
    RefreshQuickButtons();
    UpdateCurrentLabel();
}

void SubtitleControlPanel::closeEvent(QCloseEvent *event)
{
    emit SubtitleControlPanelClosed();
    QWidget::closeEvent(event);
}

SubtitleControlDock::SubtitleControlDock(QWidget *parent)
    : QDockWidget("자막 전환 컨트롤", parent)
{
    controlPanel = new SubtitleControlPanel(this);
    setWidget(controlPanel);
    
    setAllowedAreas(Qt::AllDockWidgetAreas);
    setFeatures(QDockWidget::DockWidgetMovable | 
                QDockWidget::DockWidgetFloatable | 
                QDockWidget::DockWidgetClosable);
    
    connect(controlPanel, &SubtitleControlPanel::SubtitleControlPanelClosed,
            this, &SubtitleControlDock::SubtitleControlDockClosed);
}

SubtitleManager* SubtitleControlDock::GetSubtitleManager() const
{
    return controlPanel ? controlPanel->GetSubtitleManager() : nullptr;
}

void SubtitleControlDock::closeEvent(QCloseEvent *event)
{
    emit SubtitleControlDockClosed();
    QDockWidget::closeEvent(event);
}

// WorshipFolderDialog 구현
WorshipFolderDialog::WorshipFolderDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("예배 폴더 편집");
    setModal(true);
    resize(400, 150);
    
    formLayout = new QFormLayout(this);
    
    dateEdit = new QDateEdit(this);
    dateEdit->setDate(QDate::currentDate());
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    dateEdit->setCalendarPopup(true);
    
    themeEdit = new QLineEdit(this);
    themeEdit->setPlaceholderText("예: 이곳에 주제말씀을 입력하세요.");
    
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    
    formLayout->addRow("날짜:", dateEdit);
    formLayout->addRow("주제 말씀:", themeEdit);
    formLayout->addRow(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    // 주제 입력 시 OK 버튼 활성화 상태 확인
    connect(themeEdit, &QLineEdit::textChanged, [this](const QString &text) {
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!text.trimmed().isEmpty());
    });
    
    // 초기 상태에서 OK 버튼 비활성화
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    
    themeEdit->setFocus();
}

void WorshipFolderDialog::SetData(const QString &date, const QString &theme)
{
    dateEdit->setDate(QDate::fromString(date, "yyyy-MM-dd"));
    themeEdit->setText(theme);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!theme.trimmed().isEmpty());
}

QString WorshipFolderDialog::GetDate() const
{
    return dateEdit->date().toString("yyyy-MM-dd");
}

QString WorshipFolderDialog::GetTheme() const
{
    return themeEdit->text().trimmed();
}

// BibleSearchDialog 구현
BibleSearchDialog::BibleSearchDialog(SubtitleManager *manager, QWidget *parent)
    : QDialog(parent), subtitleManager(manager)
{
    setWindowTitle("성경 구절 검색");
    setModal(true);
    resize(800, 600);
    
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    tabWidget = new QTabWidget(this);
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    
    // 초기 상태에서 OK 버튼 비활성화
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setEnabled(false);
    }
    
    // 장절 검색 탭 설정 (시그널 연결 없이)
    SetupReferenceTab();
    if (referenceTab) {
        tabWidget->addTab(referenceTab, "장절 검색");
    }
    
    // 키워드 검색 탭 설정 (시그널 연결 없이)
    SetupKeywordTab();
    if (keywordTab) {
        tabWidget->addTab(keywordTab, "키워드 검색");
    }
    
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    // 시그널 연결 (UI가 완전히 초기화된 후)
    ConnectSignals();
}

void BibleSearchDialog::SetupReferenceTab()
{
    referenceTab = new QWidget();
    referenceFormLayout = new QFormLayout(referenceTab);
    
    // 성경 책 선택
    bookComboBox = new QComboBox(referenceTab);
    if (subtitleManager) {
        bookComboBox->addItems(subtitleManager->GetBibleBooks());
        bookComboBox->setCurrentIndex(0);
    } else {
        bookComboBox->addItem("성경 데이터 없음");
        bookComboBox->setEnabled(false);
    }
    
    // 장 선택
    chapterSpinBox = new QSpinBox(referenceTab);
    chapterSpinBox->setRange(1, 150);
    chapterSpinBox->setValue(1);
    
    // 절 범위 선택
    startVerseSpinBox = new QSpinBox(referenceTab);
    startVerseSpinBox->setRange(1, 176);
    startVerseSpinBox->setValue(1);
    
    endVerseSpinBox = new QSpinBox(referenceTab);
    endVerseSpinBox->setRange(1, 176);
    endVerseSpinBox->setValue(1);
    
    QHBoxLayout *verseLayout = new QHBoxLayout();
    verseLayout->addWidget(startVerseSpinBox);
    verseLayout->addWidget(new QLabel("~"));
    verseLayout->addWidget(endVerseSpinBox);
    verseLayout->addStretch();
    
    referenceFormLayout->addRow("책:", bookComboBox);
    referenceFormLayout->addRow("장:", chapterSpinBox);
    referenceFormLayout->addRow("절:", verseLayout);
    
    // 미리보기 영역
    previewLabel = new QLabel("미리보기:", referenceTab);
    previewText = new QTextEdit(referenceTab);
    previewText->setReadOnly(true);
    previewText->setMaximumHeight(200);
    
    referenceFormLayout->addRow(previewLabel);
    referenceFormLayout->addRow(previewText);
}

void BibleSearchDialog::SetupKeywordTab()
{
    keywordTab = new QWidget();
    keywordLayout = new QVBoxLayout(keywordTab);
    
    // 키워드 입력
    QLabel *keywordLabel = new QLabel("검색할 단어를 입력하세요:", keywordTab);
    keywordLineEdit = new QLineEdit(keywordTab);
    keywordLineEdit->setPlaceholderText("예: 하나님, 사랑, 믿음");
    
    // 결과 개수 표시
    resultCountLabel = new QLabel("검색 결과: 0개", keywordTab);
    
    // 검색 결과 리스트
    searchResultsList = new QListWidget(keywordTab);
    searchResultsList->setSelectionMode(QAbstractItemView::SingleSelection);
    
    keywordLayout->addWidget(keywordLabel);
    keywordLayout->addWidget(keywordLineEdit);
    keywordLayout->addWidget(resultCountLabel);
    keywordLayout->addWidget(searchResultsList);
}

void BibleSearchDialog::ConnectSignals()
{
    // 장절 검색 탭 시그널 연결
    if (bookComboBox) {
        connect(bookComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &BibleSearchDialog::OnReferenceChanged);
    }
    if (chapterSpinBox) {
        connect(chapterSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &BibleSearchDialog::OnReferenceChanged);
    }
    if (startVerseSpinBox) {
        connect(startVerseSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &BibleSearchDialog::OnReferenceChanged);
    }
    if (endVerseSpinBox) {
        connect(endVerseSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &BibleSearchDialog::OnReferenceChanged);
    }
    
    // 키워드 검색 탭 시그널 연결
    if (keywordLineEdit) {
        connect(keywordLineEdit, &QLineEdit::textChanged,
                this, &BibleSearchDialog::OnKeywordChanged);
    }
    if (searchResultsList) {
        connect(searchResultsList, &QListWidget::itemSelectionChanged,
                this, &BibleSearchDialog::OnSearchResultSelected);
    }
    
    // 초기 미리보기 업데이트
    OnReferenceChanged();
}

void BibleSearchDialog::OnReferenceChanged()
{
    // Null pointer safety check
    if (!subtitleManager) {
        return;
    }
    
    // UI widget safety checks
    if (!previewText || !tabWidget || !buttonBox) {
        return;
    }
    
    UpdatePreview();
    
    // 장절 검색 탭이 활성화되어 있고 미리보기에 내용이 있으면 OK 버튼 활성화
    bool hasContent = !previewText->toPlainText().isEmpty();
    bool isReferenceTab = (tabWidget->currentIndex() == 0);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setEnabled(isReferenceTab && hasContent);
    }
}

void BibleSearchDialog::OnKeywordChanged()
{
    UpdateSearchResults();
}

void BibleSearchDialog::OnSearchResultSelected()
{
    // Safety checks for UI widgets
    if (!searchResultsList || !tabWidget || !buttonBox) {
        return;
    }
    
    // 키워드 검색 탭이 활성화되어 있고 선택된 항목이 있으면 OK 버튼 활성화
    bool hasSelection = !searchResultsList->selectedItems().isEmpty();
    bool isKeywordTab = (tabWidget->currentIndex() == 1);
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    if (okButton) {
        okButton->setEnabled(isKeywordTab && hasSelection);
    }
}

void BibleSearchDialog::UpdatePreview()
{
    // Safety checks for UI widgets
    if (!bookComboBox || !chapterSpinBox || !startVerseSpinBox || !endVerseSpinBox || !previewText) {
        return;
    }
    
    // Safety check for subtitleManager
    if (!subtitleManager) {
        previewText->setPlainText("성경 데이터가 로드되지 않았습니다.");
        currentResults.clear();
        return;
    }
    
    QString book = bookComboBox->currentText();
    int chapter = chapterSpinBox->value();
    int startVerse = startVerseSpinBox->value();
    int endVerse = endVerseSpinBox->value();
    
    // 끝 절이 시작 절보다 작으면 시작 절로 설정
    if (endVerse < startVerse) {
        endVerseSpinBox->setValue(startVerse);
        endVerse = startVerse;
    }
    
    QList<BibleVerse> verses = subtitleManager->GetBibleVerses(book, chapter, startVerse, endVerse);
    
    QString previewHtml;
    for (const BibleVerse &verse : verses) {
        previewHtml += QString("<b>%1</b> %2<br>").arg(verse.getDisplayText()).arg(verse.text);
    }
    
    previewText->setHtml(previewHtml);
    currentResults = verses;
}

void BibleSearchDialog::UpdateSearchResults()
{
    // Safety checks for UI widgets
    if (!keywordLineEdit || !searchResultsList || !resultCountLabel) {
        return;
    }
    
    // Safety check for subtitleManager
    if (!subtitleManager) {
        searchResultsList->clear();
        currentResults.clear();
        resultCountLabel->setText("성경 데이터가 로드되지 않았습니다.");
        return;
    }
    
    QString keyword = keywordLineEdit->text().trimmed();
    
    searchResultsList->clear();
    currentResults.clear();
    
    if (keyword.length() < 2) {
        resultCountLabel->setText("검색 결과: 0개 (2글자 이상 입력하세요)");
        return;
    }
    
    QList<BibleVerse> results = subtitleManager->SearchBible(keyword);
    currentResults = results;
    
    // 최대 100개 결과만 표시
    int displayCount = qMin(results.size(), 100);
    
    for (int i = 0; i < displayCount; ++i) {
        const BibleVerse &verse = results[i];
        QString displayText = QString("%1 - %2").arg(verse.getDisplayText()).arg(verse.text);
        
        QListWidgetItem *item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, i);  // 인덱스 저장
        searchResultsList->addItem(item);
    }
    
    QString countText = QString("검색 결과: %1개").arg(results.size());
    if (results.size() > 100) {
        countText += " (처음 100개만 표시)";
    }
    resultCountLabel->setText(countText);
}

QString BibleSearchDialog::GetSelectedText() const
{
    if (tabWidget->currentIndex() == 0) {
        // 장절 검색 탭
        QString result;
        for (const BibleVerse &verse : currentResults) {
            if (!result.isEmpty()) result += "\n";
            result += verse.text;
        }
        return result;
    } else {
        // 키워드 검색 탭
        QList<QListWidgetItem*> selected = searchResultsList->selectedItems();
        if (!selected.isEmpty()) {
            int index = selected.first()->data(Qt::UserRole).toInt();
            if (index >= 0 && index < currentResults.size()) {
                return currentResults[index].text;
            }
        }
    }
    return QString();
}

QString BibleSearchDialog::GetSelectedTitle() const
{
    if (tabWidget->currentIndex() == 0) {
        // 장절 검색 탭
        if (!currentResults.isEmpty()) {
            if (currentResults.size() == 1) {
                return currentResults.first().getDisplayText();
            } else {
                return QString("%1 %2장 %3-%4절")
                    .arg(currentResults.first().book)
                    .arg(currentResults.first().chapter)
                    .arg(currentResults.first().verse)
                    .arg(currentResults.last().verse);
            }
        }
    } else {
        // 키워드 검색 탭
        QList<QListWidgetItem*> selected = searchResultsList->selectedItems();
        if (!selected.isEmpty()) {
            int index = selected.first()->data(Qt::UserRole).toInt();
            if (index >= 0 && index < currentResults.size()) {
                return currentResults[index].getDisplayText();
            }
        }
    }
    return QString();
}