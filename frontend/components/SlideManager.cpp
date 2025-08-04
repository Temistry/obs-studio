#include "SlideManager.hpp"
#include <QTimer>
#include <QTextStream>
#include <QStringConverter>
#include <QCoreApplication>
#include <QDebug>
#include <algorithm>

// SlideData 구현
QJsonObject SlideData::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["title"] = title;
    json["width"] = width;
    json["height"] = height;
    json["backgroundColor"] = backgroundColor;
    json["backgroundImage"] = backgroundImage;
    json["createdDate"] = createdDate.toString(Qt::ISODate);
    json["modifiedDate"] = modifiedDate.toString(Qt::ISODate);
    
    QJsonArray textBoxArray;
    for (const TextBoxData &textBox : textBoxes) {
        textBoxArray.append(textBox.toJson());
    }
    json["textBoxes"] = textBoxArray;
    
    return json;
}

void SlideData::fromJson(const QJsonObject &json) {
    id = json["id"].toString();
    title = json["title"].toString();
    width = json["width"].toInt(1920);
    height = json["height"].toInt(1080);
    backgroundColor = json["backgroundColor"].toString("#000000");
    backgroundImage = json["backgroundImage"].toString();
    createdDate = QDateTime::fromString(json["createdDate"].toString(), Qt::ISODate);
    modifiedDate = QDateTime::fromString(json["modifiedDate"].toString(), Qt::ISODate);
    
    textBoxes.clear();
    QJsonArray textBoxArray = json["textBoxes"].toArray();
    for (const QJsonValue &value : textBoxArray) {
        TextBoxData textBox;
        textBox.fromJson(value.toObject());
        textBoxes.append(textBox);
    }
}

// SlideProject 구현
QJsonObject SlideProject::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name"] = name;
    json["description"] = description;
    json["currentSlideIndex"] = currentSlideIndex;
    json["createdDate"] = createdDate.toString(Qt::ISODate);
    json["modifiedDate"] = modifiedDate.toString(Qt::ISODate);
    
    QJsonArray slideArray;
    for (const SlideData &slide : slides) {
        slideArray.append(slide.toJson());
    }
    json["slides"] = slideArray;
    
    return json;
}

void SlideProject::fromJson(const QJsonObject &json) {
    id = json["id"].toString();
    name = json["name"].toString();
    description = json["description"].toString();
    currentSlideIndex = json["currentSlideIndex"].toInt(-1);
    createdDate = QDateTime::fromString(json["createdDate"].toString(), Qt::ISODate);
    modifiedDate = QDateTime::fromString(json["modifiedDate"].toString(), Qt::ISODate);
    
    slides.clear();
    QJsonArray slideArray = json["slides"].toArray();
    for (const QJsonValue &value : slideArray) {
        SlideData slide;
        slide.fromJson(value.toObject());
        slides.append(slide);
    }
}

// SlideManager 구현
SlideManager::SlideManager(QObject *parent)
    : QObject(parent), m_currentSlideIndex(-1)
{
    // 프로젝트 저장 경로 설정
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    m_projectsPath = appDataPath + "/slide-projects";
    ensureProjectsDirectory();
    
    loadAllProjects();
    
    // 자동 저장 기본 활성화 (30초)
    enableAutoSave(true);
}

SlideManager::~SlideManager() {
    saveAllProjects();
}

void SlideManager::ensureProjectsDirectory() {
    QDir dir;
    if (!dir.exists(m_projectsPath)) {
        dir.mkpath(m_projectsPath);
    }
}

QString SlideManager::getProjectFilePath(const QString &projectId) const {
    return m_projectsPath + "/" + projectId + ".json";
}

QString SlideManager::createProject(const QString &name, const QString &description) {
    SlideProject project(name);
    project.description = description;
    
    // 기본 슬라이드 하나 추가
    SlideData defaultSlide;
    defaultSlide.title = "슬라이드 1";
    project.slides.append(defaultSlide);
    project.currentSlideIndex = 0;
    
    m_projects.append(project);
    
    // 파일로 저장
    saveProject(project.id);
    
    qDebug() << "[SlideManager] Created project:" << name << "ID:" << project.id;
    emit projectsChanged();
    
    return project.id;
}

bool SlideManager::loadProject(const QString &projectId) {
    QString filePath = getProjectFilePath(projectId);
    QFile file(filePath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[SlideManager] Failed to open project file:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) {
        qWarning() << "[SlideManager] Invalid project file format:" << filePath;
        return false;
    }
    
    SlideProject project;
    project.fromJson(doc.object());
    
    // 기존 프로젝트가 있다면 교체
    for (int i = 0; i < m_projects.size(); ++i) {
        if (m_projects[i].id == projectId) {
            m_projects[i] = project;
            emit projectLoaded(projectId);
            return true;
        }
    }
    
    // 새 프로젝트라면 추가
    m_projects.append(project);
    emit projectsChanged();
    emit projectLoaded(projectId);
    
    return true;
}

bool SlideManager::saveProject(const QString &projectId) {
    SlideProject *project = nullptr;
    for (SlideProject &p : m_projects) {
        if (p.id == projectId) {
            project = &p;
            break;
        }
    }
    
    if (!project) {
        qWarning() << "[SlideManager] Project not found:" << projectId;
        return false;
    }
    
    project->modifiedDate = QDateTime::currentDateTime();
    
    QString filePath = getProjectFilePath(projectId);
    QFile file(filePath);
    
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[SlideManager] Failed to save project file:" << filePath;
        return false;
    }
    
    QJsonDocument doc(project->toJson());
    file.write(doc.toJson());
    
    qDebug() << "[SlideManager] Saved project:" << project->name;
    emit projectSaved(projectId);
    
    return true;
}

bool SlideManager::deleteProject(const QString &projectId) {
    // 메모리에서 제거
    for (int i = 0; i < m_projects.size(); ++i) {
        if (m_projects[i].id == projectId) {
            QString projectName = m_projects[i].name;
            m_projects.removeAt(i);
            
            // 현재 프로젝트가 삭제되는 경우
            if (m_currentProjectId == projectId) {
                m_currentProjectId.clear();
                m_currentSlideIndex = -1;
                emit currentProjectChanged("");
                emit currentSlideChanged(-1);
            }
            
            // 파일 삭제
            QString filePath = getProjectFilePath(projectId);
            QFile::remove(filePath);
            
            qDebug() << "[SlideManager] Deleted project:" << projectName;
            emit projectsChanged();
            return true;
        }
    }
    
    return false;
}

bool SlideManager::duplicateProject(const QString &projectId, const QString &newName) {
    const SlideProject *original = nullptr;
    for (const SlideProject &p : m_projects) {
        if (p.id == projectId) {
            original = &p;
            break;
        }
    }
    
    if (!original) return false;
    
    SlideProject duplicate = *original;
    duplicate.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    duplicate.name = newName;
    duplicate.createdDate = QDateTime::currentDateTime();
    duplicate.modifiedDate = QDateTime::currentDateTime();
    
    m_projects.append(duplicate);
    saveProject(duplicate.id);
    
    emit projectsChanged();
    return true;
}

void SlideManager::setCurrentProject(const QString &projectId) {
    if (m_currentProjectId != projectId) {
        // 이전 프로젝트 저장
        if (!m_currentProjectId.isEmpty()) {
            saveProject(m_currentProjectId);
        }
        
        m_currentProjectId = projectId;
        
        // 새 프로젝트 로드
        if (!projectId.isEmpty()) {
            loadProject(projectId);
            SlideProject *project = getCurrentProject();
            if (project) {
                m_currentSlideIndex = project->currentSlideIndex;
            }
        } else {
            m_currentSlideIndex = -1;
        }
        
        emit currentProjectChanged(projectId);
        emit currentSlideChanged(m_currentSlideIndex);
    }
}

SlideProject* SlideManager::getCurrentProject() {
    if (m_currentProjectId.isEmpty()) return nullptr;
    
    for (SlideProject &project : m_projects) {
        if (project.id == m_currentProjectId) {
            return &project;
        }
    }
    return nullptr;
}

const SlideProject* SlideManager::getCurrentProject() const {
    if (m_currentProjectId.isEmpty()) return nullptr;
    
    for (const SlideProject &project : m_projects) {
        if (project.id == m_currentProjectId) {
            return &project;
        }
    }
    return nullptr;
}

SlideProject SlideManager::getProject(const QString &projectId) const {
    for (const SlideProject &project : m_projects) {
        if (project.id == projectId) {
            return project;
        }
    }
    return SlideProject();
}

bool SlideManager::updateProject(const QString &projectId, const QString &name, const QString &description) {
    for (SlideProject &project : m_projects) {
        if (project.id == projectId) {
            project.name = name;
            project.description = description;
            project.modifiedDate = QDateTime::currentDateTime();
            saveProject(projectId);
            emit projectsChanged();
            return true;
        }
    }
    return false;
}

QString SlideManager::addSlide(const QString &title) {
    SlideProject *project = getCurrentProject();
    if (!project) return "";
    
    SlideData slide;
    slide.title = title;
    project->slides.append(slide);
    project->modifiedDate = QDateTime::currentDateTime();
    
    saveProject(project->id);
    emit slideDataChanged();
    
    return slide.id;
}

QString SlideManager::addSlideAt(int index, const QString &title) {
    SlideProject *project = getCurrentProject();
    if (!project) return "";
    
    if (index < 0 || index > project->slides.size()) {
        index = project->slides.size();
    }
    
    SlideData slide;
    slide.title = title;
    project->slides.insert(index, slide);
    project->modifiedDate = QDateTime::currentDateTime();
    
    // 현재 인덱스 조정
    if (m_currentSlideIndex >= index) {
        m_currentSlideIndex++;
        project->currentSlideIndex = m_currentSlideIndex;
    }
    
    saveProject(project->id);
    emit slideDataChanged();
    
    return slide.id;
}

bool SlideManager::removeSlide(int index) {
    SlideProject *project = getCurrentProject();
    if (!project || index < 0 || index >= project->slides.size()) {
        return false;
    }
    
    project->slides.removeAt(index);
    project->modifiedDate = QDateTime::currentDateTime();
    
    // 현재 인덱스 조정
    if (m_currentSlideIndex == index) {
        m_currentSlideIndex = -1;
        project->currentSlideIndex = -1;
        emit currentSlideChanged(-1);
    } else if (m_currentSlideIndex > index) {
        m_currentSlideIndex--;
        project->currentSlideIndex = m_currentSlideIndex;
        emit currentSlideChanged(m_currentSlideIndex);
    }
    
    saveProject(project->id);
    emit slideDataChanged();
    
    return true;
}

bool SlideManager::duplicateSlide(int index) {
    SlideProject *project = getCurrentProject();
    if (!project || index < 0 || index >= project->slides.size()) {
        return false;
    }
    
    SlideData original = project->slides[index];
    original.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    original.title += " (복사본)";
    original.createdDate = QDateTime::currentDateTime();
    original.modifiedDate = QDateTime::currentDateTime();
    
    project->slides.insert(index + 1, original);
    project->modifiedDate = QDateTime::currentDateTime();
    
    saveProject(project->id);
    emit slideDataChanged();
    
    return true;
}

bool SlideManager::moveSlide(int fromIndex, int toIndex) {
    SlideProject *project = getCurrentProject();
    if (!project || fromIndex < 0 || fromIndex >= project->slides.size() ||
        toIndex < 0 || toIndex >= project->slides.size()) {
        return false;
    }
    
    if (fromIndex == toIndex) return true;
    
    SlideData slide = project->slides.takeAt(fromIndex);
    project->slides.insert(toIndex, slide);
    project->modifiedDate = QDateTime::currentDateTime();
    
    // 현재 인덱스 조정
    if (m_currentSlideIndex == fromIndex) {
        m_currentSlideIndex = toIndex;
        project->currentSlideIndex = toIndex;
        emit currentSlideChanged(toIndex);
    } else if (fromIndex < m_currentSlideIndex && toIndex >= m_currentSlideIndex) {
        m_currentSlideIndex--;
        project->currentSlideIndex = m_currentSlideIndex;
        emit currentSlideChanged(m_currentSlideIndex);
    } else if (fromIndex > m_currentSlideIndex && toIndex <= m_currentSlideIndex) {
        m_currentSlideIndex++;
        project->currentSlideIndex = m_currentSlideIndex;
        emit currentSlideChanged(m_currentSlideIndex);
    }
    
    saveProject(project->id);
    emit slideDataChanged();
    
    return true;
}

void SlideManager::setCurrentSlide(int index) {
    SlideProject *project = getCurrentProject();
    if (!project) return;
    
    if (index >= -1 && index < project->slides.size() && m_currentSlideIndex != index) {
        m_currentSlideIndex = index;
        project->currentSlideIndex = index;
        emit currentSlideChanged(index);
    }
}

SlideData* SlideManager::getCurrentSlide() {
    SlideProject *project = getCurrentProject();
    if (!project || m_currentSlideIndex < 0 || m_currentSlideIndex >= project->slides.size()) {
        return nullptr;
    }
    return &project->slides[m_currentSlideIndex];
}

const SlideData* SlideManager::getCurrentSlide() const {
    const SlideProject *project = getCurrentProject();
    if (!project || m_currentSlideIndex < 0 || m_currentSlideIndex >= project->slides.size()) {
        return nullptr;
    }
    return &project->slides[m_currentSlideIndex];
}

SlideData SlideManager::getSlide(int index) const {
    const SlideProject *project = getCurrentProject();
    if (!project || index < 0 || index >= project->slides.size()) {
        return SlideData();
    }
    return project->slides[index];
}

bool SlideManager::updateSlide(int index, const SlideData &slideData) {
    SlideProject *project = getCurrentProject();
    if (!project || index < 0 || index >= project->slides.size()) {
        return false;
    }
    
    project->slides[index] = slideData;
    project->slides[index].modifiedDate = QDateTime::currentDateTime();
    project->modifiedDate = QDateTime::currentDateTime();
    
    emit slideDataChanged();
    return true;
}

int SlideManager::getSlideCount() const {
    const SlideProject *project = getCurrentProject();
    return project ? project->slides.size() : 0;
}

void SlideManager::addTextBox(const TextBoxData &textBox) {
    SlideData *slide = getCurrentSlide();
    if (slide) {
        slide->textBoxes.append(textBox);
        slide->modifiedDate = QDateTime::currentDateTime();
        emit slideDataChanged();
    }
}

void SlideManager::updateTextBox(int textBoxIndex, const TextBoxData &textBox) {
    SlideData *slide = getCurrentSlide();
    if (slide && textBoxIndex >= 0 && textBoxIndex < slide->textBoxes.size()) {
        slide->textBoxes[textBoxIndex] = textBox;
        slide->modifiedDate = QDateTime::currentDateTime();
        emit slideDataChanged();
    }
}

bool SlideManager::removeTextBox(int textBoxIndex) {
    SlideData *slide = getCurrentSlide();
    if (slide && textBoxIndex >= 0 && textBoxIndex < slide->textBoxes.size()) {
        slide->textBoxes.removeAt(textBoxIndex);
        slide->modifiedDate = QDateTime::currentDateTime();
        emit slideDataChanged();
        return true;
    }
    return false;
}

void SlideManager::clearTextBoxes() {
    SlideData *slide = getCurrentSlide();
    if (slide) {
        slide->textBoxes.clear();
        slide->modifiedDate = QDateTime::currentDateTime();
        emit slideDataChanged();
    }
}

QString SlideManager::generateSlideHTML(int slideIndex) const {
    const SlideProject *project = getCurrentProject();
    if (!project || slideIndex < 0 || slideIndex >= project->slides.size()) {
        return "";
    }
    
    const SlideData &slide = project->slides[slideIndex];
    
    QString html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body {
            margin: 0;
            padding: 0;
            width: %1px;
            height: %2px;
            background-color: %3;
            overflow: hidden;
            font-family: Arial, sans-serif;
        }
        .text-box {
            position: absolute;
            display: flex;
            align-items: center;
            justify-content: center;
            word-wrap: break-word;
            white-space: pre-wrap;
            overflow: hidden;
        }
    </style>
</head>
<body>
)";
    
    html = html.arg(slide.width).arg(slide.height).arg(slide.backgroundColor);
    
    // 텍스트 박스들 추가
    for (int i = 0; i < slide.textBoxes.size(); ++i) {
        const TextBoxData &textBox = slide.textBoxes[i];
        
        QString textAlign = textBox.textAlign;
        if (textAlign == "center") textAlign = "center";
        else if (textAlign == "right") textAlign = "flex-end";
        else textAlign = "flex-start";
        
        QString verticalAlign = textBox.verticalAlign;
        if (verticalAlign == "middle") verticalAlign = "center";
        else if (verticalAlign == "bottom") verticalAlign = "flex-end";
        else verticalAlign = "flex-start";
        
        QColor bgColor(textBox.backgroundColor);
        bgColor.setAlpha(textBox.backgroundOpacity * 255 / 100);
        
        QString textBoxHtml = QString(R"(
    <div class="text-box" style="
        left: %1px;
        top: %2px;
        width: %3px;
        height: %4px;
        font-family: %5;
        font-size: %6px;
        color: %7;
        background: %8;
        text-align: %9;
        justify-content: %10;
        align-items: %11;
        font-weight: %12;
        font-style: %13;
        text-decoration: %14;
        border: %15px solid %16;
    ">%17</div>
)").arg(textBox.x).arg(textBox.y).arg(textBox.width).arg(textBox.height)
   .arg(textBox.fontFamily).arg(textBox.fontSize).arg(textBox.fontColor)
   .arg(bgColor.name(QColor::HexArgb))
   .arg(textBox.textAlign).arg(textAlign).arg(verticalAlign)
   .arg(textBox.bold ? "bold" : "normal")
   .arg(textBox.italic ? "italic" : "normal")
   .arg(textBox.underline ? "underline" : "none")
   .arg(textBox.borderWidth).arg(textBox.borderColor)
   .arg(textBox.text.toHtmlEscaped().replace("\n", "<br>"));
        
        html += textBoxHtml;
    }
    
    html += "\n</body>\n</html>";
    
    return html;
}

QString SlideManager::generateCurrentSlideHTML() const {
    return generateSlideHTML(m_currentSlideIndex);
}

bool SlideManager::saveSlideAsHTML(int slideIndex, const QString &filePath) const {
    QString html = generateSlideHTML(slideIndex);
    if (html.isEmpty()) return false;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << html;
    
    return true;
}

bool SlideManager::exportProject(const QString &projectId, const QString &filePath) {
    const SlideProject *project = nullptr;
    for (const SlideProject &p : m_projects) {
        if (p.id == projectId) {
            project = &p;
            break;
        }
    }
    
    if (!project) return false;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    
    QJsonDocument doc(project->toJson());
    file.write(doc.toJson());
    
    return true;
}

QString SlideManager::importProject(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return "";
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) {
        return "";
    }
    
    SlideProject project;
    project.fromJson(doc.object());
    
    // 새로운 ID 생성 (중복 방지)
    project.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    project.name += " (가져옴)";
    
    m_projects.append(project);
    saveProject(project.id);
    
    emit projectsChanged();
    
    return project.id;
}

void SlideManager::enableAutoSave(bool enabled, int intervalMs) {
    static QTimer *autoSaveTimer = nullptr;
    
    if (autoSaveTimer) {
        autoSaveTimer->stop();
        autoSaveTimer->deleteLater();
        autoSaveTimer = nullptr;
    }
    
    if (enabled) {
        autoSaveTimer = new QTimer(this);
        connect(autoSaveTimer, &QTimer::timeout, this, &SlideManager::onAutoSave);
        autoSaveTimer->start(intervalMs);
    }
}

void SlideManager::loadAllProjects() {
    QDir dir(m_projectsPath);
    QStringList filters;
    filters << "*.json";
    
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);
    for (const QFileInfo &fileInfo : files) {
        QString projectId = fileInfo.baseName();
        loadProject(projectId);
    }
    
    qDebug() << "[SlideManager] Loaded" << m_projects.size() << "projects";
}

void SlideManager::saveAllProjects() {
    for (const SlideProject &project : m_projects) {
        // const_cast는 여기서만 예외적으로 사용
        const_cast<SlideManager*>(this)->saveProject(project.id);
    }
}

void SlideManager::onAutoSave() {
    if (!m_currentProjectId.isEmpty()) {
        saveProject(m_currentProjectId);
    }
}

#include "moc_SlideManager.cpp"