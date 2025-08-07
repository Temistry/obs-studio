#pragma once

#include <QWidget>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>
#include "SubtitleManager.hpp"

class SubtitleEditingPanel : public QWidget {
    Q_OBJECT

private:
    SubtitleManager *subtitleManager;
    
    // 편집 영역 UI 구성요소
    QGroupBox *editGroup;
    QVBoxLayout *editLayout;
    QScrollArea *editScrollArea;
    QWidget *editScrollWidget;
    QVBoxLayout *editScrollLayout;
    
    
    // 내용 편집
    QLabel *contentLabel;
    QTextEdit *contentEdit;
    
    // 버튼들
    QHBoxLayout *editButtonLayout;
    QPushButton *saveButton;
    QPushButton *cancelButton;
    QPushButton *bibleSearchButton;
    QPushButton *hymnSearchButton;
    QPushButton *autoSplitButton;
    
    // 상태 관리
    int editingIndex;
    bool isEditing;
    QTimer *autoSaveTimer;
    
    void SetupUI();
    void ConnectSignals();
    void SetEditMode(bool enabled, int index = -1);

public:
    explicit SubtitleEditingPanel(QWidget *parent = nullptr);
    ~SubtitleEditingPanel();
    
    void SetSubtitleManager(SubtitleManager *manager);
    SubtitleManager* GetSubtitleManager() const { return subtitleManager; }
    
    // 편집 상태 관리
    void StartEditing(int index);
    void StartNewSubtitle();
    void StopEditing();
    bool IsEditing() const { return isEditing; }
    
    // 내용 접근
    QString GetCurrentContent() const;
    void SetCurrentContent(const QString &content);
    void ClearContent();

public slots:
    void OnSaveSubtitle();
    void OnCancelEdit();
    void OnBibleSearch();
    void OnHymnSearch();
    void OnAutoSplit();
    void OnAutoSave();
    
    // SubtitleManager 시그널 처리
    void OnSubtitleChanged(int index);
    void OnSubtitleListChanged();

signals:
    void EditingStarted(int index);
    void EditingFinished();
    void SubtitleSaved(int index);
    void EditingCancelled();
    void ContentChanged();
};

class SubtitleEditingDock : public QDockWidget {
    Q_OBJECT

private:
    SubtitleEditingPanel *editingPanel;

public:
    explicit SubtitleEditingDock(QWidget *parent = nullptr);
    
    SubtitleEditingPanel* GetEditingPanel() const { return editingPanel; }
    void SetSubtitleManager(SubtitleManager *manager);

signals:
    void EditingDockClosed();

protected:
    void closeEvent(QCloseEvent *event) override;
};