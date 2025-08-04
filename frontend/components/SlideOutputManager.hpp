#pragma once

#include <QObject>
#include <QTimer>
#include <QUrl>
#include <QDir>
#include <QStandardPaths>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostAddress>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <obs.hpp>
#include "SlideManager.hpp"

// OBS 브라우저 소스와 연동하는 슬라이드 출력 관리자
class SlideOutputManager : public QObject {
    Q_OBJECT

private:
    SlideManager *m_slideManager;
    
    // HTTP 서버 (로컬 웹서버로 HTML 제공)
    QTcpServer *m_httpServer;
    quint16 m_serverPort;
    QString m_serverUrl;
    
    // 임시 디렉토리 (HTML 파일 저장용)
    QString m_tempDirPath;
    QString m_currentHtmlFile;
    
    // OBS 소스 관리
    QString m_targetBrowserSourceName;
    OBSWeakSource m_targetBrowserSource;
    
    // 출력 상태
    bool m_outputActive;
    QString m_currentSlideId;
    int m_currentSlideIndex;
    
    // 자동 새로고침
    QTimer *m_refreshTimer;
    int m_refreshIntervalMs;
    
    void setupHttpServer();
    void setupTempDirectory();
    bool createHtmlFile(const QString &htmlContent);
    void updateBrowserSource();
    void refreshBrowserSource();

public:
    explicit SlideOutputManager(SlideManager *slideManager, QObject *parent = nullptr);
    ~SlideOutputManager();
    
    // 서버 관리
    bool startServer(quint16 port = 0);  // 0이면 자동 포트 할당
    void stopServer();
    bool isServerRunning() const;
    QString getServerUrl() const { return m_serverUrl; }
    quint16 getServerPort() const { return m_serverPort; }
    
    // OBS 브라우저 소스 관리
    void setTargetBrowserSource(const QString &sourceName);
    QString getTargetBrowserSource() const { return m_targetBrowserSourceName; }
    bool isBrowserSourceValid() const;
    
    // 슬라이드 출력
    bool outputSlide(int slideIndex);
    bool outputCurrentSlide();
    void clearOutput();
    bool isOutputActive() const { return m_outputActive; }
    
    // HTML 생성 및 관리
    QString generateSlideHTML(int slideIndex) const;
    QString generateCurrentSlideHTML() const;
    bool saveSlideHTML(int slideIndex, const QString &filePath);
    
    // 자동 새로고침 설정
    void enableAutoRefresh(bool enabled, int intervalMs = 1000);
    void setRefreshInterval(int intervalMs) { m_refreshIntervalMs = intervalMs; }
    
    // 상태 정보
    int getCurrentSlideIndex() const { return m_currentSlideIndex; }
    QString getCurrentSlideId() const { return m_currentSlideId; }

private slots:
    void onSlideDataChanged();
    void onCurrentSlideChanged(int index);
    void onAutoRefresh();
    void onNewConnection();
    void onClientReadyRead();

public slots:
    void nextSlide();
    void previousSlide();
    void goToSlide(int index);

signals:
    void outputStarted(int slideIndex);
    void outputStopped();
    void slideChanged(int slideIndex);
    void serverStarted(const QString &url);
    void serverStopped();
    void browserSourceChanged(const QString &sourceName);
    void error(const QString &message);
};

// 브라우저 소스 미리보기 위젯 (간단한 텍스트 미리보기)
class BrowserSourcePreview : public QWidget {
    Q_OBJECT

private:
    QVBoxLayout *m_layout;
    QTextEdit *m_previewText;  // HTML 코드 미리보기
    QHBoxLayout *m_controlLayout;
    QPushButton *m_refreshButton;
    QPushButton *m_openExternalButton;
    QLabel *m_urlLabel;
    
    SlideOutputManager *m_outputManager;

public:
    explicit BrowserSourcePreview(SlideOutputManager *outputManager, QWidget *parent = nullptr);
    
    void setUrl(const QString &url);
    void refresh();

private slots:
    void onRefreshClicked();
    void onOpenExternalClicked();
};

// HTML 템플릿 관리자
class HTMLTemplateManager : public QObject {
    Q_OBJECT

public:
    enum TemplateType {
        BasicTemplate,
        ShadowTemplate,
        GradientTemplate,
        OutlineTemplate,
        CustomTemplate
    };

private:
    QMap<TemplateType, QString> m_templates;
    QString m_customTemplatePath;
    
    void loadDefaultTemplates();
    void loadCustomTemplate();

public:
    explicit HTMLTemplateManager(QObject *parent = nullptr);
    
    QString getTemplate(TemplateType type) const;
    void setCustomTemplate(const QString &templatePath);
    QString getCustomTemplatePath() const { return m_customTemplatePath; }
    
    // 템플릿에 슬라이드 데이터 적용
    QString applyTemplate(TemplateType type, const SlideData &slideData) const;
    QString applyCustomTemplate(const SlideData &slideData) const;
    
    // CSS 스타일 생성
    QString generateTextBoxCSS(const TextBoxData &textBox, int index) const;
    QString generateSlideCSS(const SlideData &slide) const;

signals:
    void customTemplateChanged(const QString &templatePath);
};

// 슬라이드 쇼 컨트롤러 (프레젠테이션 모드)
class SlideShowController : public QObject {
    Q_OBJECT

private:
    SlideOutputManager *m_outputManager;
    SlideManager *m_slideManager;
    
    // 슬라이드쇼 상태
    bool m_slideShowActive;
    int m_currentSlideIndex;
    QTimer *m_autoAdvanceTimer;
    int m_autoAdvanceInterval;  // 자동 넘김 간격 (ms)
    bool m_autoAdvanceEnabled;
    
    // 전환 효과
    QString m_transitionEffect;
    int m_transitionDuration;
    
    // 루프 재생
    bool m_loopEnabled;

public:
    explicit SlideShowController(SlideOutputManager *outputManager, QObject *parent = nullptr);
    
    // 슬라이드쇼 제어
    void startSlideShow(int startIndex = 0);
    void stopSlideShow();
    bool isSlideShowActive() const { return m_slideShowActive; }
    
    void nextSlide();
    void previousSlide();
    void goToSlide(int index);
    void goToFirstSlide();
    void goToLastSlide();
    
    // 자동 넘김 설정
    void enableAutoAdvance(bool enabled);
    void setAutoAdvanceInterval(int intervalMs);
    bool isAutoAdvanceEnabled() const { return m_autoAdvanceEnabled; }
    
    // 루프 재생 설정
    void enableLoop(bool enabled) { m_loopEnabled = enabled; }
    bool isLoopEnabled() const { return m_loopEnabled; }
    
    // 전환 효과 설정
    void setTransitionEffect(const QString &effect) { m_transitionEffect = effect; }
    void setTransitionDuration(int durationMs) { m_transitionDuration = durationMs; }
    
    // 상태 정보
    int getCurrentSlideIndex() const { return m_currentSlideIndex; }
    int getTotalSlides() const;

private slots:
    void onAutoAdvanceTimeout();

public slots:
    void pauseSlideShow();
    void resumeSlideShow();

signals:
    void slideShowStarted(int startIndex);
    void slideShowStopped();
    void slideShowPaused();
    void slideShowResumed();
    void slideChanged(int index);
    void slideShowFinished();  // 마지막 슬라이드 도달 (루프 비활성화 시)
};