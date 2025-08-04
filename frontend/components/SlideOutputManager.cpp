#include "SlideOutputManager.hpp"
#include <QApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTextEdit>
#include <QDesktopServices>
#include <QTcpServer>
#include <QTcpSocket>
#include <qt-wrappers.hpp>

// SlideOutputManager 구현
SlideOutputManager::SlideOutputManager(SlideManager *slideManager, QObject *parent)
    : QObject(parent), m_slideManager(slideManager), m_httpServer(nullptr),
      m_serverPort(0), m_outputActive(false),
      m_currentSlideIndex(-1), m_refreshTimer(nullptr), m_refreshIntervalMs(1000)
{
    setupTempDirectory();
    
    // SlideManager 시그널 연결
    connect(m_slideManager, &SlideManager::slideDataChanged, this, &SlideOutputManager::onSlideDataChanged);
    connect(m_slideManager, &SlideManager::currentSlideChanged, this, &SlideOutputManager::onCurrentSlideChanged);
    
    // 자동 새로고침 타이머 설정
    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &SlideOutputManager::onAutoRefresh);
}

SlideOutputManager::~SlideOutputManager() {
    stopServer();
    // 임시 파일 정리
    if (!m_currentHtmlFile.isEmpty() && QFile::exists(m_currentHtmlFile)) {
        QFile::remove(m_currentHtmlFile);
    }
}

void SlideOutputManager::setupTempDirectory() {
    // 시스템 임시 디렉토리 사용
    m_tempDirPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    
    // 서브디렉토리 생성
    QString slideOutputDir = m_tempDirPath + "/obs-slide-output";
    QDir dir;
    if (!dir.exists(slideOutputDir)) {
        if (!dir.mkpath(slideOutputDir)) {
            qWarning() << "[SlideOutputManager] Failed to create temp directory:" << slideOutputDir;
            return;
        }
    }
    
    m_tempDirPath = slideOutputDir;
    qDebug() << "[SlideOutputManager] Temp directory:" << m_tempDirPath;
}

bool SlideOutputManager::startServer(quint16 port) {
    if (m_httpServer) {
        stopServer();
    }
    
    m_httpServer = new QTcpServer(this);
    
    // 새 연결 시그널 연결
    connect(m_httpServer, &QTcpServer::newConnection, this, &SlideOutputManager::onNewConnection);
    
    // 서버 시작
    bool success = m_httpServer->listen(QHostAddress::LocalHost, port);
    if (!success) {
        qWarning() << "[SlideOutputManager] Failed to start HTTP server";
        delete m_httpServer;
        m_httpServer = nullptr;
        return false;
    }
    
    m_serverPort = m_httpServer->serverPort();
    if (m_serverPort == 0) {
        qWarning() << "[SlideOutputManager] Invalid server port";
        stopServer();
        return false;
    }
    
    m_serverUrl = QString("http://localhost:%1").arg(m_serverPort);
    qDebug() << "[SlideOutputManager] HTTP server started:" << m_serverUrl;
    
    emit serverStarted(m_serverUrl);
    return true;
}

void SlideOutputManager::stopServer() {
    if (m_httpServer) {
        delete m_httpServer;
        m_httpServer = nullptr;
        m_serverPort = 0;
        m_serverUrl.clear();
        
        qDebug() << "[SlideOutputManager] HTTP server stopped";
        emit serverStopped();
    }
}

bool SlideOutputManager::isServerRunning() const {
    return m_httpServer != nullptr && m_httpServer->isListening();
}

void SlideOutputManager::onNewConnection() {
    while (m_httpServer->hasPendingConnections()) {
        QTcpSocket *socket = m_httpServer->nextPendingConnection();
        connect(socket, &QTcpSocket::readyRead, this, &SlideOutputManager::onClientReadyRead);
        connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
    }
}

void SlideOutputManager::onClientReadyRead() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;
    
    QByteArray requestData = socket->readAll();
    QString request = QString::fromUtf8(requestData);
    
    // 간단한 HTTP 요청 파싱
    QStringList lines = request.split("\r\n");
    if (lines.isEmpty()) {
        socket->close();
        return;
    }
    
    QString requestLine = lines[0];
    QStringList parts = requestLine.split(" ");
    if (parts.size() < 2) {
        socket->close();
        return;
    }
    
    QString method = parts[0];
    QString path = parts[1];
    
    if (method != "GET") {
        // 405 Method Not Allowed
        QString response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
        socket->write(response.toUtf8());
        socket->close();
        return;
    }
    
    QString htmlContent;
    QString contentType = "text/html; charset=utf-8";
    
    if (path == "/" || path == "/slide.html") {
        if (!m_outputActive || m_currentHtmlFile.isEmpty()) {
            // 기본 빈 페이지
            htmlContent = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>Slide Output</title>
    <style>
        body {
            margin: 0;
            padding: 0;
            background: black;
            color: white;
            font-family: Arial, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }
        .message {
            text-align: center;
            font-size: 24px;
        }
    </style>
</head>
<body>
    <div class="message">
        <h1>슬라이드 출력 대기중</h1>
        <p>슬라이드를 선택하여 출력하세요.</p>
    </div>
</body>
</html>)";
        } else {
            // 현재 HTML 파일 내용 읽기
            QFile file(m_currentHtmlFile);
            if (file.open(QIODevice::ReadOnly)) {
                htmlContent = QString::fromUtf8(file.readAll());
            } else {
                htmlContent = "<html><body><h1>Error: Could not load slide</h1></body></html>";
            }
        }
    } else if (path == "/api/current") {
        QJsonObject response;
        response["slideIndex"] = m_currentSlideIndex;
        response["slideId"] = m_currentSlideId;
        response["active"] = m_outputActive;
        
        QJsonDocument doc(response);
        htmlContent = QString::fromUtf8(doc.toJson());
        contentType = "application/json";
    } else {
        // 404 Not Found
        QString response = "HTTP/1.1 404 Not Found\r\n\r\n<html><body><h1>404 Not Found</h1></body></html>";
        socket->write(response.toUtf8());
        socket->close();
        return;
    }
    
    // HTTP 응답 생성
    QString httpResponse = QString(
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %1\r\n"
        "Content-Length: %2\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%3"
    ).arg(contentType).arg(htmlContent.toUtf8().size()).arg(htmlContent);
    
    socket->write(httpResponse.toUtf8());
    socket->close();
}

void SlideOutputManager::setTargetBrowserSource(const QString &sourceName) {
    if (m_targetBrowserSourceName != sourceName) {
        m_targetBrowserSourceName = sourceName;
        
        if (!sourceName.isEmpty()) {
            OBSSourceAutoRelease source = obs_get_source_by_name(sourceName.toUtf8().constData());
            if (source) {
                m_targetBrowserSource = OBSGetWeakRef(source);
                updateBrowserSource();
                qDebug() << "[SlideOutputManager] Target browser source set:" << sourceName;
            } else {
                m_targetBrowserSource = nullptr;
                qWarning() << "[SlideOutputManager] Browser source not found:" << sourceName;
            }
        } else {
            m_targetBrowserSource = nullptr;
        }
        
        emit browserSourceChanged(sourceName);
    }
}

bool SlideOutputManager::isBrowserSourceValid() const {
    if (m_targetBrowserSourceName.isEmpty() || !m_targetBrowserSource) {
        return false;
    }
    
    OBSSourceAutoRelease source = obs_get_source_by_name(m_targetBrowserSourceName.toUtf8().constData());
    return source != nullptr;
}

void SlideOutputManager::updateBrowserSource() {
    if (!isBrowserSourceValid() || !isServerRunning()) {
        return;
    }
    
    OBSSourceAutoRelease source = obs_get_source_by_name(m_targetBrowserSourceName.toUtf8().constData());
    if (!source) return;
    
    // 브라우저 소스 URL 업데이트
    OBSDataAutoRelease settings = obs_data_create();
    obs_data_set_string(settings, "url", (m_serverUrl + "/slide.html").toUtf8().constData());
    obs_data_set_int(settings, "width", 1920);
    obs_data_set_int(settings, "height", 1080);
    obs_data_set_bool(settings, "restart_when_active", true);
    
    obs_source_update(source, settings);
    
    qDebug() << "[SlideOutputManager] Browser source updated with URL:" << m_serverUrl + "/slide.html";
}

void SlideOutputManager::refreshBrowserSource() {
    if (!isBrowserSourceValid()) return;
    
    OBSSourceAutoRelease source = obs_get_source_by_name(m_targetBrowserSourceName.toUtf8().constData());
    if (!source) return;
    
    // 브라우저 소스 새로고침
    obs_source_media_restart(source);
}

bool SlideOutputManager::outputSlide(int slideIndex) {
    if (!m_slideManager) {
        emit error("SlideManager not available");
        return false;
    }
    
    if (slideIndex < 0 || slideIndex >= m_slideManager->getSlideCount()) {
        emit error("Invalid slide index");
        return false;
    }
    
    // HTML 생성
    QString html = generateSlideHTML(slideIndex);
    if (html.isEmpty()) {
        emit error("Failed to generate slide HTML");
        return false;
    }
    
    // HTML 파일 생성
    if (!createHtmlFile(html)) {
        emit error("Failed to create HTML file");
        return false;
    }
    
    // 서버가 실행중이 아니면 시작
    if (!isServerRunning()) {
        if (!startServer()) {
            emit error("Failed to start HTTP server");
            return false;
        }
    }
    
    // 상태 업데이트
    m_outputActive = true;
    m_currentSlideIndex = slideIndex;
    
    SlideData slide = m_slideManager->getSlide(slideIndex);
    m_currentSlideId = slide.id;
    
    // 브라우저 소스 업데이트
    updateBrowserSource();
    
    qDebug() << "[SlideOutputManager] Output slide:" << slideIndex;
    emit outputStarted(slideIndex);
    emit slideChanged(slideIndex);
    
    return true;
}

bool SlideOutputManager::outputCurrentSlide() {
    if (!m_slideManager) return false;
    
    int currentIndex = m_slideManager->getCurrentSlideIndex();
    if (currentIndex < 0) {
        emit error("No slide selected");
        return false;
    }
    
    return outputSlide(currentIndex);
}

void SlideOutputManager::clearOutput() {
    m_outputActive = false;
    m_currentSlideIndex = -1;
    m_currentSlideId.clear();
    
    // 빈 HTML 파일 생성
    QString emptyHtml = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body { margin: 0; padding: 0; background: black; }
    </style>
</head>
<body></body>
</html>)";
    
    createHtmlFile(emptyHtml);
    refreshBrowserSource();
    
    qDebug() << "[SlideOutputManager] Output cleared";
    emit outputStopped();
}

QString SlideOutputManager::generateSlideHTML(int slideIndex) const {
    if (!m_slideManager) return "";
    
    return m_slideManager->generateSlideHTML(slideIndex);
}

QString SlideOutputManager::generateCurrentSlideHTML() const {
    return generateSlideHTML(m_currentSlideIndex);
}

bool SlideOutputManager::saveSlideHTML(int slideIndex, const QString &filePath) {
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

bool SlideOutputManager::createHtmlFile(const QString &htmlContent) {
    if (m_tempDirPath.isEmpty()) {
        return false;
    }
    
    m_currentHtmlFile = m_tempDirPath + "/slide.html";
    
    QFile file(m_currentHtmlFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "[SlideOutputManager] Failed to create HTML file:" << m_currentHtmlFile;
        return false;
    }
    
    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << htmlContent;
    
    return true;
}

void SlideOutputManager::enableAutoRefresh(bool enabled, int intervalMs) {
    if (enabled) {
        m_refreshIntervalMs = intervalMs;
        m_refreshTimer->start(intervalMs);
    } else {
        m_refreshTimer->stop();
    }
}

void SlideOutputManager::nextSlide() {
    if (!m_outputActive || !m_slideManager) return;
    
    int nextIndex = m_currentSlideIndex + 1;
    if (nextIndex < m_slideManager->getSlideCount()) {
        outputSlide(nextIndex);
    }
}

void SlideOutputManager::previousSlide() {
    if (!m_outputActive || !m_slideManager) return;
    
    int prevIndex = m_currentSlideIndex - 1;
    if (prevIndex >= 0) {
        outputSlide(prevIndex);
    }
}

void SlideOutputManager::goToSlide(int index) {
    if (!m_outputActive || !m_slideManager) return;
    
    if (index >= 0 && index < m_slideManager->getSlideCount()) {
        outputSlide(index);
    }
}

void SlideOutputManager::onSlideDataChanged() {
    if (m_outputActive && m_currentSlideIndex >= 0) {
        // 현재 출력중인 슬라이드가 변경되면 자동 업데이트
        outputSlide(m_currentSlideIndex);
    }
}

void SlideOutputManager::onCurrentSlideChanged(int index) {
    Q_UNUSED(index)
    // 필요시 자동 출력 로직 추가
}

void SlideOutputManager::onAutoRefresh() {
    if (m_outputActive) {
        refreshBrowserSource();
    }
}

// BrowserSourcePreview 구현
BrowserSourcePreview::BrowserSourcePreview(SlideOutputManager *outputManager, QWidget *parent)
    : QWidget(parent), m_outputManager(outputManager)
{
    m_layout = new QVBoxLayout(this);
    
    // HTML 코드 미리보기 텍스트 에디터
    m_previewText = new QTextEdit(this);
    m_previewText->setReadOnly(true);
    m_previewText->setFont(QFont("Consolas", 10));
    m_layout->addWidget(m_previewText);
    
    // 컨트롤 버튼들
    m_controlLayout = new QHBoxLayout();
    
    m_refreshButton = new QPushButton("새로고침", this);
    m_openExternalButton = new QPushButton("외부 브라우저에서 열기", this);
    m_urlLabel = new QLabel(this);
    
    m_controlLayout->addWidget(m_refreshButton);
    m_controlLayout->addWidget(m_openExternalButton);
    m_controlLayout->addStretch();
    m_controlLayout->addWidget(m_urlLabel);
    
    m_layout->addLayout(m_controlLayout);
    
    // 시그널 연결
    connect(m_refreshButton, &QPushButton::clicked, this, &BrowserSourcePreview::onRefreshClicked);
    connect(m_openExternalButton, &QPushButton::clicked, this, &BrowserSourcePreview::onOpenExternalClicked);
    
    // 출력 매니저 시그널 연결
    if (m_outputManager) {
        connect(m_outputManager, &SlideOutputManager::serverStarted, this, &BrowserSourcePreview::setUrl);
    }
}

void BrowserSourcePreview::setUrl(const QString &url) {
    QString fullUrl = url + "/slide.html";
    m_urlLabel->setText(fullUrl);
    refresh();
}

void BrowserSourcePreview::refresh() {
    if (m_outputManager) {
        QString html = m_outputManager->generateCurrentSlideHTML();
        if (html.isEmpty()) {
            m_previewText->setPlainText("슬라이드가 선택되지 않았습니다.");
        } else {
            m_previewText->setPlainText(html);
        }
    }
}

void BrowserSourcePreview::onRefreshClicked() {
    refresh();
}

void BrowserSourcePreview::onOpenExternalClicked() {
    QString urlText = m_urlLabel->text();
    if (!urlText.isEmpty()) {
        QDesktopServices::openUrl(QUrl(urlText));
    }
}

// HTMLTemplateManager 구현
HTMLTemplateManager::HTMLTemplateManager(QObject *parent)
    : QObject(parent)
{
    loadDefaultTemplates();
}

void HTMLTemplateManager::loadDefaultTemplates() {
    // 기본 템플릿
    m_templates[BasicTemplate] = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body {
            margin: 0;
            padding: 0;
            width: %WIDTHpx;
            height: %HEIGHTpx;
            background-color: %BACKGROUND_COLOR%;
            overflow: hidden;
            font-family: Arial, sans-serif;
        }
        %TEXT_BOX_STYLES%
    </style>
</head>
<body>
    %TEXT_BOX_ELEMENTS%
</body>
</html>)";

    // 그림자 효과 템플릿
    m_templates[ShadowTemplate] = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        body {
            margin: 0;
            padding: 0;
            width: %WIDTHpx;
            height: %HEIGHTpx;
            background-color: %BACKGROUND_COLOR%;
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
            text-shadow: 2px 2px 4px rgba(0,0,0,0.7);
        }
        %TEXT_BOX_STYLES%
    </style>
</head>
<body>
    %TEXT_BOX_ELEMENTS%
</body>
</html>)";
}

QString HTMLTemplateManager::getTemplate(TemplateType type) const {
    return m_templates.value(type, m_templates[BasicTemplate]);
}

QString HTMLTemplateManager::applyTemplate(TemplateType type, const SlideData &slideData) const {
    QString template_ = getTemplate(type);
    
    // 슬라이드 기본 정보 적용
    template_.replace("%WIDTH%", QString::number(slideData.width));
    template_.replace("%HEIGHT%", QString::number(slideData.height));
    template_.replace("%BACKGROUND_COLOR%", slideData.backgroundColor);
    
    // 텍스트 박스 스타일 생성
    QString textBoxStyles;
    QString textBoxElements;
    
    for (int i = 0; i < slideData.textBoxes.size(); ++i) {
        const TextBoxData &textBox = slideData.textBoxes[i];
        
        textBoxStyles += generateTextBoxCSS(textBox, i);
        
        QString element = QString(R"(<div class="text-box text-box-%1">%2</div>)")
                         .arg(i)
                         .arg(textBox.text.toHtmlEscaped().replace("\n", "<br>"));
        textBoxElements += element + "\n";
    }
    
    template_.replace("%TEXT_BOX_STYLES%", textBoxStyles);
    template_.replace("%TEXT_BOX_ELEMENTS%", textBoxElements);
    
    return template_;
}

QString HTMLTemplateManager::generateTextBoxCSS(const TextBoxData &textBox, int index) const {
    QColor bgColor(textBox.backgroundColor);
    bgColor.setAlpha(textBox.backgroundOpacity * 255 / 100);
    
    QString textAlign = textBox.textAlign;
    if (textAlign == "center") textAlign = "center";
    else if (textAlign == "right") textAlign = "flex-end";
    else textAlign = "flex-start";
    
    QString verticalAlign = textBox.verticalAlign;
    if (verticalAlign == "middle") verticalAlign = "center";
    else if (verticalAlign == "bottom") verticalAlign = "flex-end";
    else verticalAlign = "flex-start";
    
    return QString(R"(
        .text-box-%1 {
            left: %2px;
            top: %3px;
            width: %4px;
            height: %5px;
            font-family: %6;
            font-size: %7px;
            color: %8;
            background: %9;
            text-align: %10;
            justify-content: %11;
            align-items: %12;
            font-weight: %13;
            font-style: %14;
            text-decoration: %15;
            border: %16px solid %17;
        }
    )").arg(index)
       .arg(textBox.x).arg(textBox.y).arg(textBox.width).arg(textBox.height)
       .arg(textBox.fontFamily).arg(textBox.fontSize).arg(textBox.fontColor)
       .arg(bgColor.name(QColor::HexArgb))
       .arg(textBox.textAlign).arg(textAlign).arg(verticalAlign)
       .arg(textBox.bold ? "bold" : "normal")
       .arg(textBox.italic ? "italic" : "normal")
       .arg(textBox.underline ? "underline" : "none")
       .arg(textBox.borderWidth).arg(textBox.borderColor);
}

// SlideShowController stub implementations
void SlideShowController::onAutoAdvanceTimeout() {
    // TODO: Implement auto advance functionality
}

void SlideShowController::pauseSlideShow() {
    // TODO: Implement pause functionality
}

void SlideShowController::resumeSlideShow() {
    // TODO: Implement resume functionality
}

#include "moc_SlideOutputManager.cpp"