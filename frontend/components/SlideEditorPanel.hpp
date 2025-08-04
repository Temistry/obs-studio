#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QPushButton>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QGroupBox>
#include <QSpinBox>
#include <QSlider>
#include <QColorDialog>
#include <QFontDialog>
#include <QFileDialog>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QProgressBar>
#include <QDockWidget>
#include <QTabWidget>
#include <QTreeWidget>
#include <QPropertyAnimation>
#include <QCheckBox>
#include "SlideManager.hpp"
#include "SlideTextBox.hpp"

class SlideEditorPanel : public QWidget {
    Q_OBJECT

private:
    SlideManager *m_slideManager;
    
    // 메인 레이아웃
    QVBoxLayout *m_mainLayout;
    QMenuBar *m_menuBar;
    QToolBar *m_toolBar;
    QSplitter *m_mainSplitter;
    QStatusBar *m_statusBar;
    
    // 프로젝트 관리 영역
    QGroupBox *m_projectGroup;
    QVBoxLayout *m_projectLayout;
    QComboBox *m_projectComboBox;
    QHBoxLayout *m_projectButtonLayout;
    QPushButton *m_newProjectButton;
    QPushButton *m_openProjectButton;
    QPushButton *m_saveProjectButton;
    QPushButton *m_deleteProjectButton;
    QPushButton *m_exportProjectButton;
    QPushButton *m_importProjectButton;
    
    // 슬라이드 목록 영역
    QGroupBox *m_slideListGroup;
    QVBoxLayout *m_slideListLayout;
    QListWidget *m_slideListWidget;
    QHBoxLayout *m_slideButtonLayout;
    QPushButton *m_addSlideButton;
    QPushButton *m_duplicateSlideButton;
    QPushButton *m_removeSlideButton;
    QPushButton *m_moveUpButton;
    QPushButton *m_moveDownButton;
    
    // 슬라이드 에디터 영역
    QGroupBox *m_editorGroup;
    QVBoxLayout *m_editorLayout;
    QHBoxLayout *m_editorToolLayout;
    QPushButton *m_addTextBoxButton;
    QPushButton *m_removeTextBoxButton;
    QPushButton *m_previewButton;
    QPushButton *m_exportHtmlButton;
    SlideEditorView *m_editorView;
    
    // 속성 패널 (오른쪽)
    QTabWidget *m_propertyTabs;
    
    // 슬라이드 속성 탭
    QWidget *m_slidePropertyWidget;
    QVBoxLayout *m_slidePropertyLayout;
    QLineEdit *m_slideTitleEdit;
    QSpinBox *m_slideWidthSpinBox;
    QSpinBox *m_slideHeightSpinBox;
    QPushButton *m_slideBackgroundColorButton;
    QPushButton *m_slideBackgroundImageButton;
    QLabel *m_slideInfoLabel;
    
    // 텍스트박스 속성 탭
    QWidget *m_textBoxPropertyWidget;
    QVBoxLayout *m_textBoxPropertyLayout;
    QLineEdit *m_textBoxTextEdit;
    QComboBox *m_fontFamilyComboBox;
    QSpinBox *m_fontSizeSpinBox;
    QPushButton *m_fontColorButton;
    QPushButton *m_backgroundColorButton;
    QSlider *m_backgroundOpacitySlider;
    QComboBox *m_textAlignComboBox;
    QComboBox *m_verticalAlignComboBox;
    QCheckBox *m_boldCheckBox;
    QCheckBox *m_italicCheckBox;
    QCheckBox *m_underlineCheckBox;
    QCheckBox *m_wordWrapCheckBox;
    QSpinBox *m_borderWidthSpinBox;
    QPushButton *m_borderColorButton;
    
    // 레이아웃 도구 탭
    QWidget *m_layoutToolWidget;
    QVBoxLayout *m_layoutToolLayout;
    QPushButton *m_alignLeftButton;
    QPushButton *m_alignCenterButton;
    QPushButton *m_alignRightButton;
    QPushButton *m_alignTopButton;
    QPushButton *m_alignMiddleButton;
    QPushButton *m_alignBottomButton;
    QPushButton *m_distributeHorizontalButton;
    QPushButton *m_distributeVerticalButton;
    
    // 출력 제어 영역
    QGroupBox *m_outputGroup;
    QVBoxLayout *m_outputLayout;
    QHBoxLayout *m_outputButtonLayout;
    QPushButton *m_sendToOBSButton;
    QPushButton *m_clearOBSButton;
    QPushButton *m_prevSlideButton;
    QPushButton *m_nextSlideButton;
    QLabel *m_currentSlideLabel;
    QProgressBar *m_slideProgressBar;
    
    void setupUI();
    void setupMenus();
    void setupToolBar();
    void setupStatusBar();
    void connectSignals();
    
    void refreshProjectList();
    void refreshSlideList();
    void refreshSlideProperties();
    void refreshTextBoxProperties();
    void updateCurrentSlideLabel();
    void updateStatusBar();
    
    // 프로젝트 관련
    void createNewProject();
    void openProject();
    void saveCurrentProject();
    void deleteCurrentProject();
    void exportCurrentProject();
    void importProject();
    
    // 슬라이드 관련
    void addNewSlide();
    void duplicateCurrentSlide();
    void removeCurrentSlide();
    void moveSlideUp();
    void moveSlideDown();
    
    // 텍스트박스 관련
    void addNewTextBox();
    void removeSelectedTextBox();
    void updateTextBoxFromProperties();
    void resetTextBoxProperties();
    
    // 출력 관련
    void sendCurrentSlideToOBS();
    void clearOBSOutput();
    void goToPrevSlide();
    void goToNextSlide();
    void previewCurrentSlide();
    void exportCurrentSlideAsHTML();
    
    // 레이아웃 도구
    void alignSelectedTextBoxes(const QString &alignment);
    void distributeSelectedTextBoxes(const QString &direction);

public:
    explicit SlideEditorPanel(QWidget *parent = nullptr);
    ~SlideEditorPanel();
    
    SlideManager* getSlideManager() const { return m_slideManager; }
    void setSlideManager(SlideManager* manager) { m_slideManager = manager; }

private slots:
    // SlideManager 시그널 처리
    void onProjectsChanged();
    void onCurrentProjectChanged(const QString &projectId);
    void onCurrentSlideChanged(int index);
    void onSlideDataChanged();
    
    // UI 이벤트 처리
    void onProjectSelectionChanged();
    void onSlideSelectionChanged();
    void onTextBoxSelected(DraggableTextBox *textBox);
    void onTextBoxDeselected();
    
    // 속성 변경 처리
    void onSlidePropertyChanged();
    void onTextBoxPropertyChanged();
    
    // 색상 선택
    void onSelectSlideBackgroundColor();
    void onSelectSlideBackgroundImage();
    void onSelectFontColor();
    void onSelectBackgroundColor();
    void onSelectBorderColor();

signals:
    void slideOutputRequested(const QString &htmlContent);
    void outputCleared();
};

// 새 프로젝트 생성 다이얼로그
class NewProjectDialog : public QDialog {
    Q_OBJECT

private:
    QVBoxLayout *m_layout;
    QLineEdit *m_nameEdit;
    QTextEdit *m_descriptionEdit;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

public:
    explicit NewProjectDialog(QWidget *parent = nullptr);
    
    QString getProjectName() const;
    QString getProjectDescription() const;
    
    void setProjectName(const QString &name);
    void setProjectDescription(const QString &description);

private slots:
    void validateInput();
};

// 슬라이드 미리보기 다이얼로그
class SlidePreviewDialog : public QDialog {
    Q_OBJECT

private:
    QVBoxLayout *m_layout;
    QLabel *m_previewLabel;
    QHBoxLayout *m_buttonLayout;
    QPushButton *m_prevButton;
    QPushButton *m_nextButton;
    QPushButton *m_sendButton;
    QPushButton *m_closeButton;
    
    SlideManager *m_slideManager;
    int m_currentIndex;
    
    void updatePreview();
    void renderSlideToPixmap(int slideIndex);

public:
    explicit SlidePreviewDialog(SlideManager *manager, QWidget *parent = nullptr);
    
    void showSlide(int slideIndex);

private slots:
    void onPrevSlide();
    void onNextSlide();
    void onSendSlide();
};