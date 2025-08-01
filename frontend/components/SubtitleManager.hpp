#pragma once

#include <QObject>
#include <QStringList>
#include <QSettings>
#include <QTimer>
#include <QDateTime>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <obs.hpp>

struct SubtitleItem {
    QString title;
    QString content;
    bool enabled;
    
    SubtitleItem(const QString &t = "", const QString &c = "", bool e = true)
        : title(t), content(c), enabled(e) {}
};

struct WorshipFolder {
    QString id;          // 고유 ID (UUID)
    QString date;        // 날짜 (YYYY-MM-DD)
    QString theme;       // 주제 말씀
    QString displayName; // 표시명 (날짜 + 주제)
    QList<SubtitleItem> subtitles; // 폴더 내 자막들
    QDateTime createdDate;
    QDateTime modifiedDate;
    
    WorshipFolder() : createdDate(QDateTime::currentDateTime()), 
                     modifiedDate(QDateTime::currentDateTime()) {}
    
    WorshipFolder(const QString &d, const QString &t) 
        : date(d), theme(t), 
          displayName(QString("[%1 %2]").arg(d).arg(t)),
          createdDate(QDateTime::currentDateTime()),
          modifiedDate(QDateTime::currentDateTime()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    
    void updateDisplayName() {
        displayName = QString("[%1 %2]").arg(date).arg(theme);
    }
};

struct BibleVerse {
    QString reference;  // 예: "창1:1"
    QString text;       // 성경 본문
    QString book;       // 책명 (예: "창")
    int chapter;        // 장
    int verse;          // 절
    
    BibleVerse() : chapter(0), verse(0) {}
    BibleVerse(const QString &ref, const QString &txt) 
        : reference(ref), text(txt) {
        parseReference(ref);
    }
    
    void parseReference(const QString &ref) {
        // "창1:1" 형태를 파싱
        QRegularExpression regex("([^0-9]+)(\\d+):(\\d+)");
        QRegularExpressionMatch match = regex.match(ref);
        if (match.hasMatch()) {
            book = match.captured(1);
            chapter = match.captured(2).toInt();
            verse = match.captured(3).toInt();
        }
    }
    
    QString getDisplayText() const {
        return QString("%1 %2장 %3절").arg(book).arg(chapter).arg(verse);
    }
};

class SubtitleManager : public QObject {
    Q_OBJECT

private:
    QList<SubtitleItem> subtitles;  // 현재 활성 자막 리스트 (레거시 호환)
    QList<WorshipFolder> worshipFolders;  // 예배 폴더들
    QString currentFolderId;  // 현재 선택된 폴더 ID
    int currentIndex;
    QString targetSourceName;
    OBSWeakSource targetSource;
    QSettings *settings;
    
    // 성경 데이터
    QMap<QString, QString> bibleData;  // 성경 구절 데이터 (키: "창1:1", 값: 본문)
    QStringList bookNames;  // 성경 책 이름 목록
    bool bibleDataLoaded;
    
    void SaveSettings();
    void LoadSettings();
    void SaveWorshipFolders();
    void LoadWorshipFolders();
    void UpdateTextSource();
    WorshipFolder* GetCurrentFolder();
    void SyncCurrentSubtitles();
    
    // 성경 데이터 관리
    void LoadBibleData();
    void ParseBibleReference(const QString &reference, QString &book, int &chapter, int &verse);
    QList<BibleVerse> SearchBibleByKeyword(const QString &keyword);
    QList<BibleVerse> SearchBibleByRange(const QString &book, int startChapter, int startVerse, int endChapter = -1, int endVerse = -1);

public:
    explicit SubtitleManager(QObject *parent = nullptr);
    ~SubtitleManager();
    
    // 자막 관리
    void AddSubtitle(const QString &title, const QString &content);
    void UpdateSubtitle(int index, const QString &title, const QString &content);
    void RemoveSubtitle(int index);
    void ClearSubtitles();
    
    // 자막 전환
    void SetCurrentSubtitle(int index);
    void NextSubtitle();
    void PreviousSubtitle();
    void ClearCurrentSubtitle();
    
    // 소스 관리
    void SetTargetSource(const QString &sourceName);
    QString GetTargetSource() const { return targetSourceName; }
    
    // 데이터 접근
    int GetSubtitleCount() const { return subtitles.size(); }
    int GetCurrentIndex() const { return currentIndex; }
    SubtitleItem GetSubtitle(int index) const;
    QList<SubtitleItem> GetAllSubtitles() const { return subtitles; }
    
    // 파일 입출력
    void ImportFromFile(const QString &filePath);
    void ExportToFile(const QString &filePath);
    
    // 예배 폴더 관리
    QString CreateWorshipFolder(const QString &date, const QString &theme);
    void UpdateWorshipFolder(const QString &folderId, const QString &date, const QString &theme);
    void RemoveWorshipFolder(const QString &folderId);
    void SetCurrentFolder(const QString &folderId);
    QString GetCurrentFolderId() const { return currentFolderId; }
    QList<WorshipFolder> GetAllWorshipFolders() const { return worshipFolders; }
    WorshipFolder GetWorshipFolder(const QString &folderId) const;
    
    // 현재 폴더의 자막 관리
    void AddSubtitleToCurrentFolder(const QString &title, const QString &content);
    void UpdateSubtitleInCurrentFolder(int index, const QString &title, const QString &content);
    void RemoveSubtitleFromCurrentFolder(int index);
    void ClearCurrentFolderSubtitles();
    
    // 성경 검색 기능
    bool IsBibleDataLoaded() const { return bibleDataLoaded; }
    void ReloadBibleData();
    QStringList GetBibleBooks() const { return bookNames; }
    BibleVerse GetBibleVerse(const QString &reference) const;
    QList<BibleVerse> SearchBible(const QString &keyword) const;
    QList<BibleVerse> GetBibleChapter(const QString &book, int chapter) const;
    QList<BibleVerse> GetBibleVerses(const QString &book, int chapter, int startVerse, int endVerse = -1) const;

signals:
    void SubtitleChanged(int index);
    void SubtitleListChanged();
    void TargetSourceChanged(const QString &sourceName);
    void WorshipFoldersChanged();
    void CurrentFolderChanged(const QString &folderId);

public slots:
    void OnSourceRename(const QString &oldName, const QString &newName);
    void OnSourceRemoved(const QString &sourceName);

private slots:
    void CheckTargetSource();
};