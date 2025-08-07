#include "SubtitleManager.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QSet>
#include <algorithm>
#include <obs.hpp>
#include <OBSApp.hpp>
#include <qt-wrappers.hpp>

#include "moc_SubtitleManager.cpp"

SubtitleManager::SubtitleManager(QObject *parent)
    : QObject(parent), 
      currentIndex(-1),
      currentFolderId(""),
      targetSourceName(""),
      settings(nullptr),
      bibleDataLoaded(false)
{
    // 설정 파일 경로 설정
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    settings = new QSettings(configPath + "/subtitle-manager.ini", QSettings::IniFormat, this);
    
    LoadSettings();
    LoadWorshipFolders();
    LoadBibleData();
    
    // 주기적으로 타겟 소스 확인
    QTimer *checkTimer = new QTimer(this);
    connect(checkTimer, &QTimer::timeout, this, &SubtitleManager::CheckTargetSource);
    checkTimer->start(1000); // 1초마다 체크
    
    // OBS 시그널 연결 (소스 이름 변경, 제거 감지)
    auto signalHandler = obs_get_signal_handler();
    signal_handler_connect(signalHandler, "source_rename", 
                          [](void *data, calldata_t *cd) {
                              auto manager = static_cast<SubtitleManager*>(data);
                              const char *oldName = calldata_string(cd, "prev_name");
                              const char *newName = calldata_string(cd, "new_name");
                              QMetaObject::invokeMethod(manager, "OnSourceRename",
                                                      Qt::QueuedConnection,
                                                      Q_ARG(QString, QString::fromUtf8(oldName)),
                                                      Q_ARG(QString, QString::fromUtf8(newName)));
                          }, this);
    
    signal_handler_connect(signalHandler, "source_remove", 
                          [](void *data, calldata_t *cd) {
                              auto manager = static_cast<SubtitleManager*>(data);
                              auto source = static_cast<obs_source_t*>(calldata_ptr(cd, "source"));
                              const char *name = obs_source_get_name(source);
                              QMetaObject::invokeMethod(manager, "OnSourceRemoved",
                                                      Qt::QueuedConnection,
                                                      Q_ARG(QString, QString::fromUtf8(name)));
                          }, this);
}

SubtitleManager::~SubtitleManager()
{
    // OBS 시그널 연결 해제
    auto signalHandler = obs_get_signal_handler();
    signal_handler_disconnect(signalHandler, "source_rename", nullptr, this);
    signal_handler_disconnect(signalHandler, "source_remove", nullptr, this);
    
    SaveSettings();
    SaveWorshipFolders();
}

void SubtitleManager::SaveSettings()
{
    if (!settings) return;
    
    settings->beginGroup("SubtitleManager");
    settings->setValue("targetSource", targetSourceName);
    settings->setValue("currentIndex", currentIndex);
    settings->setValue("currentFolderId", currentFolderId);
    
    settings->beginWriteArray("subtitles");
    for (int i = 0; i < subtitles.size(); ++i) {
        settings->setArrayIndex(i);
        settings->setValue("title", subtitles[i].title);
        settings->setValue("content", subtitles[i].content);
        settings->setValue("enabled", subtitles[i].enabled);
    }
    settings->endArray();
    settings->endGroup();
    
    settings->sync();
}

void SubtitleManager::LoadSettings()
{
    if (!settings) return;
    
    settings->beginGroup("SubtitleManager");
    targetSourceName = settings->value("targetSource", "").toString();
    currentIndex = settings->value("currentIndex", -1).toInt();
    currentFolderId = settings->value("currentFolderId", "").toString();
    
    int size = settings->beginReadArray("subtitles");
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        QString title = settings->value("title", "").toString();
        QString content = settings->value("content", "").toString();
        bool enabled = settings->value("enabled", true).toBool();
        subtitles.append(SubtitleItem(title, content, enabled));
    }
    settings->endArray();
    settings->endGroup();
    
    // 현재 인덱스 유효성 검사
    if (currentIndex >= subtitles.size()) {
        currentIndex = -1;
    }
}

void SubtitleManager::UpdateTextSource()
{
    if (targetSourceName.isEmpty()) return;
    
    // 안전한 소스 접근을 위해 weak reference 사용
    OBSSourceAutoRelease source = obs_get_source_by_name(targetSourceName.toUtf8().constData());
    if (!source) {
        blog(LOG_WARNING, "[SubtitleManager] Source '%s' not found", 
             targetSourceName.toUtf8().constData());
        return;
    }
    
    // 소스가 유효한지 다시 한번 확인
    if (!obs_source_get_ref(source)) {
        blog(LOG_WARNING, "[SubtitleManager] Source '%s' is being destroyed", 
             targetSourceName.toUtf8().constData());
        return;
    }
    
    QString text = "";
    if (currentIndex >= 0 && currentIndex < subtitles.size()) {
        if (subtitles[currentIndex].enabled) {
            text = subtitles[currentIndex].content;
        }
    }
    
    const char *sourceId = obs_source_get_id(source);
    if (!sourceId) {
        blog(LOG_WARNING, "[SubtitleManager] Source ID is null for '%s'", 
             targetSourceName.toUtf8().constData());
        return;
    }
    
    OBSDataAutoRelease settings = obs_data_create();
    
    // 소스 타입에 따라 올바른 속성명 사용
    if (strcmp(sourceId, "text_gdiplus") == 0) {
        obs_data_set_string(settings, "text", text.toUtf8().constData());
    } else if (strcmp(sourceId, "text_ft2_source") == 0) {
        obs_data_set_string(settings, "text", text.toUtf8().constData());
    } else {
        // 기타 텍스트 소스들에 대한 fallback
        obs_data_set_string(settings, "text", text.toUtf8().constData());
    }
    
    // 안전한 업데이트 수행
    try {
        obs_source_update(source, settings);
        
        // 소스가 활성화되어 있는지 확인하고 로그 출력
        bool sourceActive = obs_source_active(source);
        bool sourceShowing = obs_source_showing(source);
        
        blog(LOG_INFO, "[SubtitleManager] Source '%s' updated with text: '%s' (Active: %s, Showing: %s)", 
             targetSourceName.toUtf8().constData(), 
             text.toUtf8().constData(),
             sourceActive ? "Yes" : "No",
             sourceShowing ? "Yes" : "No");
    } catch (...) {
        blog(LOG_ERROR, "[SubtitleManager] Exception occurred while updating source '%s'", 
             targetSourceName.toUtf8().constData());
    }
}

void SubtitleManager::AddSubtitle(const QString &title, const QString &content)
{
    subtitles.append(SubtitleItem(title, content, true));
    SaveSettings();
    emit SubtitleListChanged();
}

void SubtitleManager::UpdateSubtitle(int index, const QString &title, const QString &content)
{
    if (index >= 0 && index < subtitles.size()) {
        subtitles[index].title = title;
        subtitles[index].content = content;
        
        // 현재 선택된 자막이라면 즉시 업데이트
        if (index == currentIndex) {
            UpdateTextSource();
        }
        
        SaveSettings();
        emit SubtitleListChanged();
    }
}

void SubtitleManager::RemoveSubtitle(int index)
{
    if (index >= 0 && index < subtitles.size()) {
        subtitles.removeAt(index);
        
        // 현재 인덱스 조정
        if (currentIndex == index) {
            currentIndex = -1;
            UpdateTextSource();
        } else if (currentIndex > index) {
            currentIndex--;
        }
        
        SaveSettings();
        emit SubtitleListChanged();
        emit SubtitleChanged(currentIndex);
    }
}

void SubtitleManager::ClearSubtitles()
{
    subtitles.clear();
    currentIndex = -1;
    UpdateTextSource();
    SaveSettings();
    emit SubtitleListChanged();
    emit SubtitleChanged(currentIndex);
}

void SubtitleManager::SetCurrentSubtitle(int index)
{
    if (index >= -1 && index < subtitles.size()) {
        currentIndex = index;
        UpdateTextSource();
        SaveSettings();
        emit SubtitleChanged(currentIndex);
    }
}

void SubtitleManager::NextSubtitle()
{
    if (subtitles.isEmpty()) return;
    
    int nextIndex = currentIndex + 1;
    if (nextIndex >= subtitles.size()) {
        nextIndex = 0;
    }
    SetCurrentSubtitle(nextIndex);
}

void SubtitleManager::PreviousSubtitle()
{
    if (subtitles.isEmpty()) return;
    
    int prevIndex = currentIndex - 1;
    if (prevIndex < 0) {
        prevIndex = subtitles.size() - 1;
    }
    SetCurrentSubtitle(prevIndex);
}

void SubtitleManager::ClearCurrentSubtitle()
{
    SetCurrentSubtitle(-1);
}

void SubtitleManager::SetTargetSource(const QString &sourceName)
{
    if (targetSourceName != sourceName) {
        targetSourceName = sourceName;
        
        // 새로운 타겟 소스 설정
        if (!sourceName.isEmpty()) {
            OBSSourceAutoRelease source = obs_get_source_by_name(sourceName.toUtf8().constData());
            if (source) {
                targetSource = OBSGetWeakRef(source);
                blog(LOG_INFO, "[SubtitleManager] Target source set to '%s'", 
                     sourceName.toUtf8().constData());
            } else {
                targetSource = nullptr;
                blog(LOG_WARNING, "[SubtitleManager] Target source '%s' not found", 
                     sourceName.toUtf8().constData());
            }
        } else {
            targetSource = nullptr;
            blog(LOG_INFO, "[SubtitleManager] Target source cleared");
        }
        
        UpdateTextSource();
        SaveSettings();
        emit TargetSourceChanged(sourceName);
    }
}

SubtitleItem SubtitleManager::GetSubtitle(int index) const
{
    if (index >= 0 && index < subtitles.size()) {
        return subtitles[index];
    }
    return SubtitleItem();
}

void SubtitleManager::ImportFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) return;
    
    QJsonObject obj = doc.object();
    QJsonArray subtitleArray = obj["subtitles"].toArray();
    
    subtitles.clear();
    for (const QJsonValue &value : subtitleArray) {
        QJsonObject subtitleObj = value.toObject();
        QString title = subtitleObj["title"].toString();
        QString content = subtitleObj["content"].toString();
        bool enabled = subtitleObj["enabled"].toBool(true);
        subtitles.append(SubtitleItem(title, content, enabled));
    }
    
    currentIndex = -1;
    UpdateTextSource();
    SaveSettings();
    emit SubtitleListChanged();
    emit SubtitleChanged(currentIndex);
}

void SubtitleManager::ExportToFile(const QString &filePath)
{
    QJsonObject obj;
    QJsonArray subtitleArray;
    
    for (const SubtitleItem &item : subtitles) {
        QJsonObject subtitleObj;
        subtitleObj["title"] = item.title;
        subtitleObj["content"] = item.content;
        subtitleObj["enabled"] = item.enabled;
        subtitleArray.append(subtitleObj);
    }
    
    obj["subtitles"] = subtitleArray;
    obj["version"] = "1.0";
    
    QJsonDocument doc(obj);
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}

void SubtitleManager::OnSourceRename(const QString &oldName, const QString &newName)
{
    if (targetSourceName == oldName) {
        targetSourceName = newName;
        SaveSettings();
        emit TargetSourceChanged(newName);
    }
}

void SubtitleManager::OnSourceRemoved(const QString &sourceName)
{
    if (targetSourceName == sourceName) {
        targetSourceName = "";
        targetSource = nullptr;
        SaveSettings();
        emit TargetSourceChanged("");
    }
}

void SubtitleManager::CheckTargetSource()
{
    if (targetSourceName.isEmpty()) return;
    
    OBSSourceAutoRelease source = obs_get_source_by_name(targetSourceName.toUtf8().constData());
    if (!source && targetSource) {
        // 소스가 삭제되었음
        blog(LOG_INFO, "[SubtitleManager] Target source '%s' was removed", 
             targetSourceName.toUtf8().constData());
        OnSourceRemoved(targetSourceName);
    } else if (source) {
        // 약한 참조로 저장
        targetSource = OBSGetWeakRef(source);
    }
}

// 예배 폴더 관리 구현
void SubtitleManager::SaveWorshipFolders()
{
    if (!settings) return;
    
    settings->beginGroup("WorshipFolders");
    settings->beginWriteArray("folders");
    
    for (int i = 0; i < worshipFolders.size(); ++i) {
        settings->setArrayIndex(i);
        const WorshipFolder &folder = worshipFolders[i];
        
        settings->setValue("id", folder.id);
        settings->setValue("date", folder.date);
        settings->setValue("theme", folder.theme);
        settings->setValue("displayName", folder.displayName);
        settings->setValue("createdDate", folder.createdDate);
        settings->setValue("modifiedDate", folder.modifiedDate);
        
        // 폴더 내 자막들 저장
        settings->beginWriteArray("subtitles");
        for (int j = 0; j < folder.subtitles.size(); ++j) {
            settings->setArrayIndex(j);
            settings->setValue("title", folder.subtitles[j].title);
            settings->setValue("content", folder.subtitles[j].content);
            settings->setValue("enabled", folder.subtitles[j].enabled);
        }
        settings->endArray();
    }
    
    settings->endArray();
    settings->endGroup();
    settings->sync();
}

void SubtitleManager::LoadWorshipFolders()
{
    if (!settings) return;
    
    worshipFolders.clear();
    
    settings->beginGroup("WorshipFolders");
    int size = settings->beginReadArray("folders");
    
    for (int i = 0; i < size; ++i) {
        settings->setArrayIndex(i);
        
        WorshipFolder folder;
        folder.id = settings->value("id", "").toString();
        folder.date = settings->value("date", "").toString();
        folder.theme = settings->value("theme", "").toString();
        folder.displayName = settings->value("displayName", "").toString();
        folder.createdDate = settings->value("createdDate", QDateTime::currentDateTime()).toDateTime();
        folder.modifiedDate = settings->value("modifiedDate", QDateTime::currentDateTime()).toDateTime();
        
        // 폴더 내 자막들 로드
        int subtitlesSize = settings->beginReadArray("subtitles");
        for (int j = 0; j < subtitlesSize; ++j) {
            settings->setArrayIndex(j);
            QString title = settings->value("title", "").toString();
            QString content = settings->value("content", "").toString();
            bool enabled = settings->value("enabled", true).toBool();
            folder.subtitles.append(SubtitleItem(title, content, enabled));
        }
        settings->endArray();
        
        worshipFolders.append(folder);
    }
    
    settings->endArray();
    settings->endGroup();
    
    // 현재 폴더가 설정되어 있다면 동기화
    if (!currentFolderId.isEmpty()) {
        SyncCurrentSubtitles();
    }
}

WorshipFolder* SubtitleManager::GetCurrentFolder()
{
    if (currentFolderId.isEmpty()) return nullptr;
    
    for (int i = 0; i < worshipFolders.size(); ++i) {
        if (worshipFolders[i].id == currentFolderId) {
            return &worshipFolders[i];
        }
    }
    return nullptr;
}

void SubtitleManager::SyncCurrentSubtitles()
{
    WorshipFolder* folder = GetCurrentFolder();
    if (folder) {
        subtitles = folder->subtitles;
        // 현재 인덱스 유효성 검사
        if (currentIndex >= subtitles.size()) {
            currentIndex = -1;
        }
    } else {
        subtitles.clear();
        currentIndex = -1;
    }
}

QString SubtitleManager::CreateWorshipFolder(const QString &date, const QString &theme)
{
    WorshipFolder folder(date, theme);
    worshipFolders.append(folder);
    SaveWorshipFolders();
    
    blog(LOG_INFO, "[SubtitleManager] Created worship folder: %s", 
         folder.displayName.toUtf8().constData());
    
    emit WorshipFoldersChanged();
    return folder.id;
}

void SubtitleManager::UpdateWorshipFolder(const QString &folderId, const QString &date, const QString &theme)
{
    for (int i = 0; i < worshipFolders.size(); ++i) {
        if (worshipFolders[i].id == folderId) {
            worshipFolders[i].date = date;
            worshipFolders[i].theme = theme;
            worshipFolders[i].updateDisplayName();
            worshipFolders[i].modifiedDate = QDateTime::currentDateTime();
            
            SaveWorshipFolders();
            emit WorshipFoldersChanged();
            
            blog(LOG_INFO, "[SubtitleManager] Updated worship folder: %s", 
                 worshipFolders[i].displayName.toUtf8().constData());
            break;
        }
    }
}

void SubtitleManager::RemoveWorshipFolder(const QString &folderId)
{
    for (int i = 0; i < worshipFolders.size(); ++i) {
        if (worshipFolders[i].id == folderId) {
            QString displayName = worshipFolders[i].displayName;
            worshipFolders.removeAt(i);
            
            // 현재 폴더가 삭제된 폴더라면 클리어
            if (currentFolderId == folderId) {
                currentFolderId = "";
                SyncCurrentSubtitles();
                emit CurrentFolderChanged("");
                emit SubtitleListChanged();
                emit SubtitleChanged(currentIndex);
            }
            
            SaveWorshipFolders();
            emit WorshipFoldersChanged();
            
            blog(LOG_INFO, "[SubtitleManager] Removed worship folder: %s", 
                 displayName.toUtf8().constData());
            break;
        }
    }
}

void SubtitleManager::SetCurrentFolder(const QString &folderId)
{
    if (currentFolderId != folderId) {
        // 기존 폴더의 자막들을 저장
        if (!currentFolderId.isEmpty()) {
            WorshipFolder* prevFolder = GetCurrentFolder();
            if (prevFolder) {
                prevFolder->subtitles = subtitles;
                prevFolder->modifiedDate = QDateTime::currentDateTime();
            }
        }
        
        currentFolderId = folderId;
        currentIndex = -1;  // 새 폴더로 전환 시 인덱스 리셋
        
        SyncCurrentSubtitles();
        UpdateTextSource();
        SaveSettings();
        SaveWorshipFolders();
        
        emit CurrentFolderChanged(folderId);
        emit SubtitleListChanged();
        emit SubtitleChanged(currentIndex);
        
        WorshipFolder folder = GetWorshipFolder(folderId);
        blog(LOG_INFO, "[SubtitleManager] Changed to folder: %s", 
             folder.displayName.toUtf8().constData());
    }
}

WorshipFolder SubtitleManager::GetWorshipFolder(const QString &folderId) const
{
    for (const WorshipFolder &folder : worshipFolders) {
        if (folder.id == folderId) {
            return folder;
        }
    }
    return WorshipFolder();
}

void SubtitleManager::AddSubtitleToCurrentFolder(const QString &title, const QString &content)
{
    WorshipFolder* folder = GetCurrentFolder();
    if (folder) {
        folder->subtitles.append(SubtitleItem(title, content, true));
        folder->modifiedDate = QDateTime::currentDateTime();
        SyncCurrentSubtitles();
        SaveWorshipFolders();
        emit SubtitleListChanged();
    } else {
        // 폴더가 없으면 기존 방식으로 추가
        AddSubtitle(title, content);
    }
}

void SubtitleManager::UpdateSubtitleInCurrentFolder(int index, const QString &title, const QString &content)
{
    WorshipFolder* folder = GetCurrentFolder();
    if (folder && index >= 0 && index < folder->subtitles.size()) {
        folder->subtitles[index].title = title;
        folder->subtitles[index].content = content;
        folder->modifiedDate = QDateTime::currentDateTime();
        
        // 현재 선택된 자막이라면 즉시 업데이트
        if (index == currentIndex) {
            UpdateTextSource();
        }
        
        SyncCurrentSubtitles();
        SaveWorshipFolders();
        emit SubtitleListChanged();
    } else {
        // 폴더가 없으면 기존 방식으로 업데이트
        UpdateSubtitle(index, title, content);
    }
}

void SubtitleManager::RemoveSubtitleFromCurrentFolder(int index)
{
    WorshipFolder* folder = GetCurrentFolder();
    if (folder && index >= 0 && index < folder->subtitles.size()) {
        folder->subtitles.removeAt(index);
        folder->modifiedDate = QDateTime::currentDateTime();
        
        // 현재 인덱스 조정
        if (currentIndex == index) {
            currentIndex = -1;
            UpdateTextSource();
        } else if (currentIndex > index) {
            currentIndex--;
        }
        
        SyncCurrentSubtitles();
        SaveWorshipFolders();
        emit SubtitleListChanged();
        emit SubtitleChanged(currentIndex);
    } else {
        // 폴더가 없으면 기존 방식으로 삭제
        RemoveSubtitle(index);
    }
}

void SubtitleManager::ClearCurrentFolderSubtitles()
{
    WorshipFolder* folder = GetCurrentFolder();
    if (folder) {
        folder->subtitles.clear();
        folder->modifiedDate = QDateTime::currentDateTime();
        currentIndex = -1;
        SyncCurrentSubtitles();
        UpdateTextSource();
        SaveWorshipFolders();
        emit SubtitleListChanged();
        emit SubtitleChanged(currentIndex);
    } else {
        // 폴더가 없으면 기존 방식으로 클리어
        ClearSubtitles();
    }
}

// 성경 데이터 관리 기능 구현
void SubtitleManager::LoadBibleData()
{
    bibleDataLoaded = false;
    bibleData.clear();
    bookNames.clear();
    
    // 성경 파일 경로 - 실행 파일 기준 상대경로로 설정
    QString bibleFilePath = QCoreApplication::applicationDirPath() + "/../../data/parser/bible.json";
    bibleFilePath = QDir::cleanPath(bibleFilePath);
    
    QFile file(bibleFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        blog(LOG_WARNING, "[SubtitleManager] Failed to open bible.json: %s", 
             bibleFilePath.toUtf8().constData());
        return;
    }
    
    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    
    if (!doc.isObject()) {
        blog(LOG_WARNING, "[SubtitleManager] Invalid bible.json format");
        return;
    }
    
    QJsonObject obj = doc.object();
    QSet<QString> uniqueBooks;
    
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        QString reference = it.key();
        QString text = it.value().toString();
        
        bibleData[reference] = text;
        
        // 책 이름 추출
        QRegularExpression regex("([^0-9]+)\\d+:\\d+");
        QRegularExpressionMatch match = regex.match(reference);
        if (match.hasMatch()) {
            QString book = match.captured(1);
            uniqueBooks.insert(book);
        }
    }
    
    bookNames = QStringList(uniqueBooks.begin(), uniqueBooks.end());
    bookNames.sort();
    bibleDataLoaded = true;
    
    blog(LOG_INFO, "[SubtitleManager] Bible data loaded: %d verses, %d books", 
         bibleData.size(), bookNames.size());
}

void SubtitleManager::ReloadBibleData()
{
    LoadBibleData();
}

BibleVerse SubtitleManager::GetBibleVerse(const QString &reference) const
{
    if (bibleData.contains(reference)) {
        return BibleVerse(reference, bibleData[reference]);
    }
    return BibleVerse();
}

QList<BibleVerse> SubtitleManager::SearchBible(const QString &keyword) const
{
    QList<BibleVerse> results;
    
    if (keyword.isEmpty() || !bibleDataLoaded) {
        return results;
    }
    
    for (auto it = bibleData.begin(); it != bibleData.end(); ++it) {
        QString reference = it.key();
        QString text = it.value();
        
        if (text.contains(keyword, Qt::CaseInsensitive)) {
            results.append(BibleVerse(reference, text));
        }
    }
    
    // 참조순으로 정렬 (창1:1, 창1:2, ... 순서)
    std::sort(results.begin(), results.end(), [](const BibleVerse &a, const BibleVerse &b) {
        if (a.book != b.book) return a.book < b.book;
        if (a.chapter != b.chapter) return a.chapter < b.chapter;
        return a.verse < b.verse;
    });
    
    return results;
}

QList<BibleVerse> SubtitleManager::GetBibleChapter(const QString &book, int chapter) const
{
    QList<BibleVerse> results;
    
    if (!bibleDataLoaded) return results;
    
    for (auto it = bibleData.begin(); it != bibleData.end(); ++it) {
        QString reference = it.key();
        QString text = it.value();
        
        BibleVerse verse(reference, text);
        if (verse.book == book && verse.chapter == chapter) {
            results.append(verse);
        }
    }
    
    // 절 순으로 정렬
    std::sort(results.begin(), results.end(), [](const BibleVerse &a, const BibleVerse &b) {
        return a.verse < b.verse;
    });
    
    return results;
}

QList<BibleVerse> SubtitleManager::GetBibleVerses(const QString &book, int chapter, int startVerse, int endVerse) const
{
    QList<BibleVerse> allVerses = GetBibleChapter(book, chapter);
    QList<BibleVerse> results;
    
    if (endVerse == -1) endVerse = startVerse;
    
    for (const BibleVerse &verse : allVerses) {
        if (verse.verse >= startVerse && verse.verse <= endVerse) {
            results.append(verse);
        }
    }
    
    return results;
}
