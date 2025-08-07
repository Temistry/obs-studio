/*
 * SubtitleEditingPanel 사용법 예시
 * 
 * 자막 편집 영역을 별도의 UI로 분리한 SubtitleEditingPanel 컴포넌트의 
 * 통합 및 사용법을 설명하는 예시 코드입니다.
 */

#include "SubtitleEditingPanel.hpp"
#include "SubtitleControlPanel.hpp"
#include <QMainWindow>
#include <QMenuBar>
#include <QAction>

/*
 * 1. SubtitleEditingPanel 기본 사용법
 */

void BasicUsageExample() {
    /*
    // SubtitleEditingPanel을 독립 위젯으로 사용
    SubtitleEditingPanel *editPanel = new SubtitleEditingPanel();
    
    // SubtitleManager 연결
    SubtitleManager *manager = new SubtitleManager();
    editPanel->SetSubtitleManager(manager);
    
    // 창으로 표시
    editPanel->show();
    editPanel->resize(400, 350);
    editPanel->setWindowTitle("자막 편집");
    */
}

/*
 * 2. SubtitleEditingDock을 사용한 도킹 패널 구현
 */

void DockingPanelExample() {
    /*
    // 메인 윈도우에서 사용하는 경우
    QMainWindow *mainWindow = // ... 메인 윈도우 참조
    
    // 도킹 패널 생성
    SubtitleEditingDock *editDock = new SubtitleEditingDock(mainWindow);
    
    // SubtitleManager 설정
    SubtitleManager *manager = // ... SubtitleManager 인스턴스
    editDock->SetSubtitleManager(manager);
    
    // 메인 윈도우에 도킹 추가
    mainWindow->addDockWidget(Qt::RightDockWidgetArea, editDock);
    
    // 초기 상태는 숨김
    editDock->hide();
    */
}

/*
 * 3. SubtitleControlPanel과 연동하여 사용
 */

void IntegratedUsageExample() {
    /*
    // SubtitleControlPanel에서는 이미 편집 도킹이 통합되어 있습니다.
    SubtitleControlPanel *controlPanel = new SubtitleControlPanel();
    
    // "편집" 버튼을 클릭하거나 자막을 더블클릭하면
    // 자동으로 SubtitleEditingDock이 열립니다.
    
    // SubtitleControlPanel의 편집 버튼 동작:
    // 1. 자막 리스트에서 항목 선택 후 "편집" 버튼 클릭
    // 2. 자막 리스트 항목 더블클릭
    // 3. 자동으로 SubtitleEditingDock이 열리고 해당 자막이 편집 모드로 로드됨
    */
}

/*
 * 4. 프로그래밍 방식으로 편집 제어
 */

void ProgrammaticControlExample() {
    /*
    SubtitleEditingPanel *editPanel = // ... 편집 패널 인스턴스
    
    // 특정 자막 편집 시작
    editPanel->StartEditing(0); // 0번 인덱스 자막 편집
    
    // 새 자막 작성 시작
    editPanel->StartNewSubtitle();
    
    // 편집 중인지 확인
    if (editPanel->IsEditing()) {
        // 현재 편집 중인 내용 가져오기
        QString title = editPanel->GetCurrentTitle();
        QString content = editPanel->GetCurrentContent();
    }
    
    // 편집 중단
    editPanel->StopEditing();
    
    // 특정 내용으로 설정
    editPanel->SetCurrentContent("제목", "내용");
    */
}

/*
 * 5. 시그널 연결 예시
 */

void SignalConnectionExample() {
    /*
    SubtitleEditingPanel *editPanel = // ... 편집 패널 인스턴스
    
    // 편집 시작 시그널 연결
    connect(editPanel, &SubtitleEditingPanel::EditingStarted,
            [](int index) {
                qDebug() << "편집 시작: 인덱스" << index;
            });
    
    // 편집 완료 시그널 연결
    connect(editPanel, &SubtitleEditingPanel::EditingFinished,
            []() {
                qDebug() << "편집 완료";
            });
    
    // 자막 저장 시그널 연결
    connect(editPanel, &SubtitleEditingPanel::SubtitleSaved,
            [](int index) {
                qDebug() << "자막 저장됨: 인덱스" << index;
            });
    
    // 편집 취소 시그널 연결
    connect(editPanel, &SubtitleEditingPanel::EditingCancelled,
            []() {
                qDebug() << "편집 취소됨";
            });
    
    // 내용 변경 시그널 연결
    connect(editPanel, &SubtitleEditingPanel::ContentChanged,
            []() {
                qDebug() << "내용 변경됨";
            });
    */
}

/*
 * 6. 메뉴 항목으로 편집 패널 열기
 */

void MenuIntegrationExample() {
    /*
    QMainWindow *mainWindow = // ... 메인 윈도우
    SubtitleEditingDock *editDock = // ... 편집 도킹 패널
    
    // 메뉴에 편집 패널 토글 액션 추가
    QMenuBar *menuBar = mainWindow->menuBar();
    QMenu *viewMenu = menuBar->addMenu("보기(&V)");
    
    QAction *toggleEditAction = viewMenu->addAction("자막 편집 패널(&E)");
    toggleEditAction->setCheckable(true);
    toggleEditAction->setShortcut(QKeySequence("Ctrl+E"));
    
    connect(toggleEditAction, &QAction::triggered, [=](bool checked) {
        if (checked) {
            editDock->show();
            editDock->raise();
        } else {
            editDock->hide();
        }
    });
    
    // 도킹 패널이 닫힐 때 메뉴 상태 업데이트
    connect(editDock, &SubtitleEditingDock::EditingDockClosed, [=]() {
        toggleEditAction->setChecked(false);
    });
    */
}

/*
 * 주요 특징 및 장점:
 * 
 * 1. 완전 분리된 UI
 *    - 자막 편집 기능이 별도 컴포넌트로 분리됨
 *    - SubtitleControlPanel에서 편집 관련 복잡성 제거
 * 
 * 2. 유연한 배치
 *    - 도킹 패널로 사용 가능
 *    - 독립 창으로 사용 가능
 *    - 임의의 위치에 배치 가능
 * 
 * 3. 크기 조절 가능
 *    - 최소 크기: 320x280
 *    - 사용자가 자유롭게 크기 조절 가능
 *    - 스크롤 영역으로 작은 화면에서도 사용 가능
 * 
 * 4. 자동 저장 기능
 *    - 5초 후 자동 저장
 *    - 변경사항 감지 및 알림
 * 
 * 5. 통합 검색 기능
 *    - 성경 구절 검색 및 삽입
 *    - 찬송가 검색 및 삽입
 * 
 * 6. 사용자 친화적 인터페이스
 *    - 직관적인 버튼 배치
 *    - 아이콘으로 기능 구분
 *    - 상태 표시 및 피드백
 * 
 * 사용 시나리오:
 * 
 * 1. 메인 컨트롤 패널 + 별도 편집 패널
 *    - SubtitleControlPanel: 자막 목록 관리 및 빠른 전환
 *    - SubtitleEditingPanel: 상세 편집 작업
 * 
 * 2. 듀얼 모니터 환경
 *    - 메인 화면: OBS 스튜디오 + 컨트롤 패널
 *    - 보조 화면: 자막 편집 패널
 * 
 * 3. 워크플로우 최적화
 *    - 실시간 방송 중: 컨트롤 패널로 빠른 전환
 *    - 사전 준비: 편집 패널로 상세 작업
 */