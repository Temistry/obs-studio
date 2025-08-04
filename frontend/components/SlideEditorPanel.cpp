#include "SlideEditorPanel.hpp"
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QTextStream>
#include <QStringConverter>
#include <QFormLayout>
#include <QDialogButtonBox>

SlideEditorPanel::SlideEditorPanel(QWidget *parent)
    : QWidget(parent), m_slideManager(nullptr)
{
    setupUI();
    connectSignals();
}

SlideEditorPanel::~SlideEditorPanel() {
}

void SlideEditorPanel::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    
    // 프로젝트 관리 영역
    m_projectGroup = new QGroupBox("프로젝트 관리", this);
    m_projectLayout = new QVBoxLayout(m_projectGroup);
    
    m_projectComboBox = new QComboBox(this);
    m_projectLayout->addWidget(m_projectComboBox);
    
    m_projectButtonLayout = new QHBoxLayout();
    m_newProjectButton = new QPushButton("새 프로젝트", this);
    m_saveProjectButton = new QPushButton("저장", this);
    m_deleteProjectButton = new QPushButton("삭제", this);
    
    m_projectButtonLayout->addWidget(m_newProjectButton);
    m_projectButtonLayout->addWidget(m_saveProjectButton);
    m_projectButtonLayout->addWidget(m_deleteProjectButton);
    m_projectLayout->addLayout(m_projectButtonLayout);
    
    m_mainLayout->addWidget(m_projectGroup);
    
    // 슬라이드 목록 영역
    m_slideListGroup = new QGroupBox("슬라이드 목록", this);
    m_slideListLayout = new QVBoxLayout(m_slideListGroup);
    
    m_slideListWidget = new QListWidget(this);
    m_slideListLayout->addWidget(m_slideListWidget);
    
    m_slideButtonLayout = new QHBoxLayout();
    m_addSlideButton = new QPushButton("슬라이드 추가", this);
    m_removeSlideButton = new QPushButton("삭제", this);
    
    m_slideButtonLayout->addWidget(m_addSlideButton);
    m_slideButtonLayout->addWidget(m_removeSlideButton);
    m_slideListLayout->addLayout(m_slideButtonLayout);
    
    m_mainLayout->addWidget(m_slideListGroup);
    
    // 슬라이드 에디터 영역 (간단한 텍스트 에디터로 구현)
    m_editorGroup = new QGroupBox("슬라이드 편집", this);
    m_editorLayout = new QVBoxLayout(m_editorGroup);
    
    m_editorToolLayout = new QHBoxLayout();
    m_addTextBoxButton = new QPushButton("텍스트 박스 추가", this);
    m_previewButton = new QPushButton("미리보기", this);
    
    m_editorToolLayout->addWidget(m_addTextBoxButton);
    m_editorToolLayout->addWidget(m_previewButton);
    m_editorLayout->addLayout(m_editorToolLayout);
    
    // 슬라이드 에디터 뷰 (임시로 텍스트 에디터 사용)
    m_editorView = nullptr; // 복잡한 그래픽스 뷰는 일단 제외
    QTextEdit *tempEditor = new QTextEdit(this);
    tempEditor->setPlaceholderText("슬라이드 내용을 입력하세요...");
    m_editorLayout->addWidget(tempEditor);
    
    m_mainLayout->addWidget(m_editorGroup);
    
    // 출력 제어 영역
    m_outputGroup = new QGroupBox("출력 제어", this);
    m_outputLayout = new QVBoxLayout(m_outputGroup);
    
    m_outputButtonLayout = new QHBoxLayout();
    m_sendToOBSButton = new QPushButton("OBS로 출력", this);
    m_clearOBSButton = new QPushButton("출력 지우기", this);
    
    m_outputButtonLayout->addWidget(m_sendToOBSButton);
    m_outputButtonLayout->addWidget(m_clearOBSButton);
    m_outputLayout->addLayout(m_outputButtonLayout);
    
    m_currentSlideLabel = new QLabel("현재 슬라이드: 없음", this);
    m_outputLayout->addWidget(m_currentSlideLabel);
    
    m_mainLayout->addWidget(m_outputGroup);
}

void SlideEditorPanel::connectSignals() {
    connect(m_newProjectButton, &QPushButton::clicked, this, &SlideEditorPanel::createNewProject);
    connect(m_saveProjectButton, &QPushButton::clicked, this, &SlideEditorPanel::saveCurrentProject);
    connect(m_deleteProjectButton, &QPushButton::clicked, this, &SlideEditorPanel::deleteCurrentProject);
    
    connect(m_addSlideButton, &QPushButton::clicked, this, &SlideEditorPanel::addNewSlide);
    connect(m_removeSlideButton, &QPushButton::clicked, this, &SlideEditorPanel::removeCurrentSlide);
    
    connect(m_sendToOBSButton, &QPushButton::clicked, this, &SlideEditorPanel::sendCurrentSlideToOBS);
    connect(m_clearOBSButton, &QPushButton::clicked, this, &SlideEditorPanel::clearOBSOutput);
    
    connect(m_previewButton, &QPushButton::clicked, this, &SlideEditorPanel::previewCurrentSlide);
    
    connect(m_projectComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SlideEditorPanel::onProjectSelectionChanged);
    connect(m_slideListWidget, &QListWidget::currentRowChanged,
            this, &SlideEditorPanel::onSlideSelectionChanged);
}

void SlideEditorPanel::refreshProjectList() {
    if (!m_slideManager) return;
    
    m_projectComboBox->clear();
    QList<SlideProject> projects = m_slideManager->getAllProjects();
    
    for (const SlideProject &project : projects) {
        m_projectComboBox->addItem(project.getDisplayName(), project.id);
    }
}

void SlideEditorPanel::refreshSlideList() {
    if (!m_slideManager) return;
    
    m_slideListWidget->clear();
    int slideCount = m_slideManager->getSlideCount();
    
    for (int i = 0; i < slideCount; ++i) {
        SlideData slide = m_slideManager->getSlide(i);
        QString displayText = QString("슬라이드 %1: %2").arg(i + 1).arg(slide.title);
        m_slideListWidget->addItem(displayText);
    }
}

void SlideEditorPanel::updateCurrentSlideLabel() {
    if (!m_slideManager) {
        m_currentSlideLabel->setText("현재 슬라이드: 없음");
        return;
    }
    
    int currentIndex = m_slideManager->getCurrentSlideIndex();
    if (currentIndex >= 0) {
        SlideData slide = m_slideManager->getSlide(currentIndex);
        m_currentSlideLabel->setText(QString("현재 슬라이드: %1 - %2").arg(currentIndex + 1).arg(slide.title));
    } else {
        m_currentSlideLabel->setText("현재 슬라이드: 없음");
    }
}

void SlideEditorPanel::createNewProject() {
    bool ok;
    QString name = QInputDialog::getText(this, "새 프로젝트", "프로젝트 이름:", QLineEdit::Normal, "", &ok);
    
    if (ok && !name.isEmpty() && m_slideManager) {
        QString projectId = m_slideManager->createProject(name);
        if (!projectId.isEmpty()) {
            refreshProjectList();
            m_slideManager->setCurrentProject(projectId);
        }
    }
}

void SlideEditorPanel::saveCurrentProject() {
    if (m_slideManager) {
        QString currentProjectId = m_slideManager->getCurrentProjectId();
        if (!currentProjectId.isEmpty()) {
            m_slideManager->saveProject(currentProjectId);
            QMessageBox::information(this, "저장 완료", "프로젝트가 저장되었습니다.");
        }
    }
}

void SlideEditorPanel::deleteCurrentProject() {
    if (!m_slideManager) return;
    
    QString currentProjectId = m_slideManager->getCurrentProjectId();
    if (currentProjectId.isEmpty()) return;
    
    int ret = QMessageBox::question(this, "프로젝트 삭제", "정말로 이 프로젝트를 삭제하시겠습니까?",
                                    QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_slideManager->deleteProject(currentProjectId);
        refreshProjectList();
    }
}

void SlideEditorPanel::addNewSlide() {
    if (m_slideManager) {
        bool ok;
        QString title = QInputDialog::getText(this, "새 슬라이드", "슬라이드 제목:", QLineEdit::Normal, "새 슬라이드", &ok);
        
        if (ok && !title.isEmpty()) {
            m_slideManager->addSlide(title);
            refreshSlideList();
        }
    }
}

void SlideEditorPanel::removeCurrentSlide() {
    if (!m_slideManager) return;
    
    int currentRow = m_slideListWidget->currentRow();
    if (currentRow >= 0) {
        int ret = QMessageBox::question(this, "슬라이드 삭제", "정말로 이 슬라이드를 삭제하시겠습니까?",
                                        QMessageBox::Yes | QMessageBox::No);
        
        if (ret == QMessageBox::Yes) {
            m_slideManager->removeSlide(currentRow);
            refreshSlideList();
            updateCurrentSlideLabel();
        }
    }
}

void SlideEditorPanel::sendCurrentSlideToOBS() {
    if (m_slideManager) {
        QString html = m_slideManager->generateCurrentSlideHTML();
        if (!html.isEmpty()) {
            emit slideOutputRequested(html);
        } else {
            QMessageBox::warning(this, "오류", "출력할 슬라이드가 없습니다.");
        }
    }
}

void SlideEditorPanel::clearOBSOutput() {
    emit outputCleared();
}

void SlideEditorPanel::previewCurrentSlide() {
    if (m_slideManager) {
        QString html = m_slideManager->generateCurrentSlideHTML();
        if (!html.isEmpty()) {
            // 임시 HTML 파일 생성하여 브라우저에서 열기
            QString tempPath = QDir::temp().filePath("slide_preview.html");
            QFile file(tempPath);
            if (file.open(QIODevice::WriteOnly)) {
                QTextStream stream(&file);
                stream.setEncoding(QStringConverter::Utf8);
                stream << html;
                file.close();
                
                QDesktopServices::openUrl(QUrl::fromLocalFile(tempPath));
            }
        }
    }
}

void SlideEditorPanel::onProjectSelectionChanged() {
    int index = m_projectComboBox->currentIndex();
    if (index >= 0 && m_slideManager) {
        QString projectId = m_projectComboBox->itemData(index).toString();
        m_slideManager->setCurrentProject(projectId);
        refreshSlideList();
        updateCurrentSlideLabel();
    }
}

void SlideEditorPanel::onSlideSelectionChanged() {
    int index = m_slideListWidget->currentRow();
    if (index >= 0 && m_slideManager) {
        m_slideManager->setCurrentSlide(index);
        updateCurrentSlideLabel();
    }
}

// SlideManager 시그널 처리
void SlideEditorPanel::onProjectsChanged() {
    refreshProjectList();
}

void SlideEditorPanel::onCurrentProjectChanged(const QString &projectId) {
    Q_UNUSED(projectId)
    refreshSlideList();
    updateCurrentSlideLabel();
}

void SlideEditorPanel::onCurrentSlideChanged(int index) {
    Q_UNUSED(index)
    updateCurrentSlideLabel();
    m_slideListWidget->setCurrentRow(index);
}

void SlideEditorPanel::onSlideDataChanged() {
    refreshSlideList();
}

// 새 프로젝트 다이얼로그
NewProjectDialog::NewProjectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("새 프로젝트 생성");
    setModal(true);
    
    m_layout = new QVBoxLayout(this);
    
    QFormLayout *formLayout = new QFormLayout();
    
    m_nameEdit = new QLineEdit(this);
    m_descriptionEdit = new QTextEdit(this);
    m_descriptionEdit->setMaximumHeight(100);
    
    formLayout->addRow("프로젝트 이름:", m_nameEdit);
    formLayout->addRow("설명:", m_descriptionEdit);
    
    m_layout->addLayout(formLayout);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_cancelButton = buttonBox->button(QDialogButtonBox::Cancel);
    
    m_layout->addWidget(buttonBox);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &NewProjectDialog::validateInput);
    
    validateInput();
}

QString NewProjectDialog::getProjectName() const {
    return m_nameEdit->text().trimmed();
}

QString NewProjectDialog::getProjectDescription() const {
    return m_descriptionEdit->toPlainText().trimmed();
}

void NewProjectDialog::setProjectName(const QString &name) {
    m_nameEdit->setText(name);
}

void NewProjectDialog::setProjectDescription(const QString &description) {
    m_descriptionEdit->setPlainText(description);
}

void NewProjectDialog::validateInput() {
    bool valid = !m_nameEdit->text().trimmed().isEmpty();
    m_okButton->setEnabled(valid);
}

// Missing SlideEditorPanel method stubs
void SlideEditorPanel::onTextBoxSelected(DraggableTextBox *textBox) {
    // TODO: Implement text box selection
}

void SlideEditorPanel::onTextBoxDeselected() {
    // TODO: Implement text box deselection
}

void SlideEditorPanel::onSlidePropertyChanged() {
    // TODO: Implement slide property changes
}

void SlideEditorPanel::onTextBoxPropertyChanged() {
    // TODO: Implement text box property changes
}

void SlideEditorPanel::onSelectSlideBackgroundColor() {
    // TODO: Implement slide background color selection
}

void SlideEditorPanel::onSelectSlideBackgroundImage() {
    // TODO: Implement slide background image selection
}

void SlideEditorPanel::onSelectFontColor() {
    // TODO: Implement font color selection
}

void SlideEditorPanel::onSelectBackgroundColor() {
    // TODO: Implement background color selection
}

void SlideEditorPanel::onSelectBorderColor() {
    // TODO: Implement border color selection
}

// Missing SlidePreviewDialog method stubs
void SlidePreviewDialog::onPrevSlide() {
    // TODO: Implement previous slide navigation
}

void SlidePreviewDialog::onNextSlide() {
    // TODO: Implement next slide navigation
}

void SlidePreviewDialog::onSendSlide() {
    // TODO: Implement slide sending functionality
}

#include "moc_SlideEditorPanel.cpp"