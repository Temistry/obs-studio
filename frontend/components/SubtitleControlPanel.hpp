#pragma once

#include <QWidget>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QLineEdit>
#include <QTextEdit>
#include <QGroupBox>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QMessageBox>
#include <QCompleter>
#include <QSpinBox>
#include <QTabWidget>
#include <QScrollArea>
#include "SubtitleManager.hpp"
#include "SubtitleEditingPanel.hpp"

class SubtitleControlDock;

class SubtitleControlPanel : public QWidget {
    Q_OBJECT

private:
    SubtitleManager *subtitleManager;
    
    // 메인 레이아웃
    QVBoxLayout *mainLayout;
    QScrollArea *mainScrollArea;
    QWidget *mainScrollWidget;
    QSplitter *mainSplitter;
    
    // 타겟 소스 선택
    QGroupBox *sourceGroup;
    QHBoxLayout *sourceLayout;
    QLabel *sourceLabel;
    QComboBox *sourceComboBox;
    QPushButton *refreshSourceButton;
    
    // 예배 폴더 관리
    QGroupBox *folderGroup;
    QVBoxLayout *folderLayout;
    QTreeWidget *folderTreeWidget;
    QHBoxLayout *folderButtonLayout;
    QPushButton *addFolderButton;
    QPushButton *editFolderButton;
    QPushButton *removeFolderButton;
    
    // 자막 리스트 관리
    QGroupBox *listGroup;
    QVBoxLayout *listLayout;
    QListWidget *subtitleList;
    QHBoxLayout *listButtonLayout;
    QPushButton *addButton;
    QPushButton *editButton;
    QPushButton *removeButton;
    QPushButton *clearButton;
    QPushButton *importButton;
    QPushButton *exportButton;
    
    // 편집 패널 참조 (별도 UI로 분리됨)
    SubtitleEditingDock *editingDock;
    
    // 빠른 전환 컨트롤
    QGroupBox *controlGroup;
    QVBoxLayout *controlLayout;
    QLabel *currentLabel;
    QHBoxLayout *quickButtonLayout;
    QPushButton *prevButton;
    QPushButton *clearCurrentButton;
    QPushButton *nextButton;
    
    // 빠른 액세스 버튼들 (스크롤 가능)
    QGroupBox *quickGroup;
    QScrollArea *quickScrollArea;
    QWidget *quickScrollWidget;
    QGridLayout *quickLayout;  
    QList<QPushButton*> quickButtons;
    
    
    void SetupUI();
    void RefreshSourceList();
    void RefreshSubtitleList();
    void RefreshQuickButtons();
    void RefreshFolderTree();
    void UpdateCurrentLabel();
    void CreateEditingDock();
    void ShowEditingDock();

public:
    explicit SubtitleControlPanel(QWidget *parent = nullptr);
    ~SubtitleControlPanel();
    
    SubtitleManager* GetSubtitleManager() const { return subtitleManager; }

private slots:
    // 소스 관련
    void OnSourceChanged();
    void OnRefreshSource();
    
    // 폴더 관리
    void OnFolderSelectionChanged();
    void OnAddFolder();
    void OnEditFolder();
    void OnRemoveFolder();
    
    // 리스트 관리
    void OnSubtitleSelectionChanged();
    void OnAddSubtitle();
    void OnRemoveSubtitle();
    void OnClearSubtitles();
    void OnImportSubtitles();
    void OnExportSubtitles();
    
    // 편집 관련 (편집 도킹으로 이동됨)
    void OnOpenEditingDock();
    
    // 전환 컨트롤
    void OnPreviousSubtitle();
    void OnNextSubtitle();
    void OnClearCurrent();
    void OnQuickButtonClicked();
    
    // SubtitleManager 시그널 처리
    void OnSubtitleChanged(int index);
    void OnSubtitleListChanged();
    void OnTargetSourceChanged(const QString &sourceName);
    void OnWorshipFoldersChanged();
    void OnCurrentFolderChanged(const QString &folderId);
    

signals:
    void SubtitleControlPanelClosed();

protected:
    void closeEvent(QCloseEvent *event) override;
};

class WorshipFolderDialog : public QDialog {
    Q_OBJECT

private:
    QFormLayout *formLayout;
    QDateEdit *dateEdit;
    QLineEdit *themeEdit;
    QDialogButtonBox *buttonBox;

public:
    explicit WorshipFolderDialog(QWidget *parent = nullptr);
    
    void SetData(const QString &date, const QString &theme);
    QString GetDate() const;
    QString GetTheme() const;
};

class BibleSearchDialog : public QDialog {
    Q_OBJECT

private:
    QTabWidget *tabWidget = nullptr;
    
    // 장절 검색 탭
    QWidget *referenceTab = nullptr;
    QFormLayout *referenceFormLayout = nullptr;
    QComboBox *bookComboBox = nullptr;
    QSpinBox *chapterSpinBox = nullptr;
    QSpinBox *startVerseSpinBox = nullptr;
    QSpinBox *endVerseSpinBox = nullptr;
    QLabel *previewLabel = nullptr;
    QScrollArea *previewScrollArea = nullptr;
    QTextEdit *previewText = nullptr;
    
    // 키워드 검색 탭
    QWidget *keywordTab = nullptr;
    QVBoxLayout *keywordLayout = nullptr;
    QLineEdit *keywordLineEdit = nullptr;
    QListWidget *searchResultsList = nullptr;
    QLabel *resultCountLabel = nullptr;
    
    QDialogButtonBox *buttonBox = nullptr;
    SubtitleManager *subtitleManager;
    
    QList<BibleVerse> currentResults;
    
    void SetupReferenceTab();
    void SetupKeywordTab();
    void ConnectSignals();
    void UpdatePreview();
    void UpdateSearchResults();

public:
    explicit BibleSearchDialog(SubtitleManager *manager, QWidget *parent = nullptr);
    
    QString GetSelectedText() const;
    QString GetSelectedTitle() const;

private slots:
    void OnReferenceChanged();
    void OnKeywordChanged();
    void OnSearchResultSelected();
};

class HymnSearchDialog : public QDialog {
    Q_OBJECT

private:
    QVBoxLayout *mainLayout = nullptr;
    
    // 찬송가 번호 입력
    QHBoxLayout *numberLayout = nullptr;
    QLabel *numberLabel = nullptr;
    QSpinBox *hymnNumberSpinBox = nullptr;
    QPushButton *searchButton = nullptr;
    
    // 미리보기 영역
    QLabel *previewLabel = nullptr;
    QTextEdit *previewText = nullptr;
    
    QDialogButtonBox *buttonBox = nullptr;
    
    QString currentHymnContent;
    QString currentHymnTitle;
    
    void SetupUI();
    void LoadHymnData(int hymnNumber);
    QString GetHymnFilePath(int hymnNumber) const;

public:
    explicit HymnSearchDialog(QWidget *parent = nullptr);
    
    QString GetSelectedText() const;
    QString GetSelectedTitle() const;

private slots:
    void OnHymnNumberChanged();
    void OnSearchButtonClicked();
};

class SubtitleControlDock : public QDockWidget {
    Q_OBJECT

private:
    SubtitleControlPanel *controlPanel;

public:
    explicit SubtitleControlDock(QWidget *parent = nullptr);
    
    SubtitleControlPanel* GetControlPanel() const { return controlPanel; }
    SubtitleManager* GetSubtitleManager() const;

signals:
    void SubtitleControlDockClosed();

protected:
    void closeEvent(QCloseEvent *event) override;
};