#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include "SlideTextBox.hpp"

// 슬라이드 데이터 구조
struct SlideData {
    QString id;  // 고유 ID
    QString title;  // 슬라이드 제목
    QList<TextBoxData> textBoxes;  // 텍스트 박스들
    int width = 1920;  // 슬라이드 크기
    int height = 1080;
    QString backgroundColor = "#000000";
    QString backgroundImage;  // 배경 이미지 경로
    QDateTime createdDate;
    QDateTime modifiedDate;
    
    SlideData() : createdDate(QDateTime::currentDateTime()), 
                  modifiedDate(QDateTime::currentDateTime()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
};

// 슬라이드 프로젝트 (PPT 파일과 유사)
struct SlideProject {
    QString id;  // 고유 ID
    QString name;  // 프로젝트 이름
    QString description;  // 설명
    QList<SlideData> slides;  // 슬라이드들
    int currentSlideIndex = -1;  // 현재 슬라이드
    QDateTime createdDate;
    QDateTime modifiedDate;
    
    SlideProject() : createdDate(QDateTime::currentDateTime()), 
                     modifiedDate(QDateTime::currentDateTime()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    SlideProject(const QString &projectName) : name(projectName),
                                               createdDate(QDateTime::currentDateTime()),
                                               modifiedDate(QDateTime::currentDateTime()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    
    QString getDisplayName() const {
        return QString("%1 (%2 슬라이드)").arg(name).arg(slides.size());
    }
};

// 슬라이드 관리자
class SlideManager : public QObject {
    Q_OBJECT

private:
    QList<SlideProject> m_projects;  // 모든 프로젝트들
    QString m_currentProjectId;  // 현재 활성 프로젝트 ID
    int m_currentSlideIndex;  // 현재 슬라이드 인덱스
    QString m_projectsPath;  // 프로젝트 저장 경로
    
    void ensureProjectsDirectory();
    QString getProjectFilePath(const QString &projectId) const;

public:
    explicit SlideManager(QObject *parent = nullptr);
    ~SlideManager();
    
    // 프로젝트 관리
    QString createProject(const QString &name, const QString &description = "");
    bool loadProject(const QString &projectId);
    bool saveProject(const QString &projectId);
    bool deleteProject(const QString &projectId);
    bool duplicateProject(const QString &projectId, const QString &newName);
    
    void setCurrentProject(const QString &projectId);
    QString getCurrentProjectId() const { return m_currentProjectId; }
    SlideProject* getCurrentProject();
    const SlideProject* getCurrentProject() const;
    
    QList<SlideProject> getAllProjects() const { return m_projects; }
    SlideProject getProject(const QString &projectId) const;
    bool updateProject(const QString &projectId, const QString &name, const QString &description);
    
    // 슬라이드 관리
    QString addSlide(const QString &title = "새 슬라이드");
    QString addSlideAt(int index, const QString &title = "새 슬라이드");
    bool removeSlide(int index);
    bool duplicateSlide(int index);
    bool moveSlide(int fromIndex, int toIndex);
    
    void setCurrentSlide(int index);
    int getCurrentSlideIndex() const { return m_currentSlideIndex; }
    SlideData* getCurrentSlide();
    const SlideData* getCurrentSlide() const;
    
    SlideData getSlide(int index) const;
    bool updateSlide(int index, const SlideData &slideData);
    int getSlideCount() const;
    
    // 텍스트 박스 관리 (현재 슬라이드에서)
    void addTextBox(const TextBoxData &textBox);
    void updateTextBox(int textBoxIndex, const TextBoxData &textBox);
    bool removeTextBox(int textBoxIndex);
    void clearTextBoxes();
    
    // 파일 입출력
    bool exportProject(const QString &projectId, const QString &filePath);
    QString importProject(const QString &filePath);
    
    // HTML 생성 (브라우저 소스용)
    QString generateSlideHTML(int slideIndex) const;
    QString generateCurrentSlideHTML() const;
    bool saveSlideAsHTML(int slideIndex, const QString &filePath) const;
    
    // 자동 저장
    void enableAutoSave(bool enabled, int intervalMs = 30000);
    
    void loadAllProjects();
    void saveAllProjects();

private slots:
    void onAutoSave();

signals:
    void projectsChanged();
    void currentProjectChanged(const QString &projectId);
    void currentSlideChanged(int index);
    void slideDataChanged();
    void projectSaved(const QString &projectId);
    void projectLoaded(const QString &projectId);
};