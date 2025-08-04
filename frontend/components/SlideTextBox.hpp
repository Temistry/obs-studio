#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsProxyWidget>
#include <QTextEdit>
#include <QFontDialog>
#include <QColorDialog>
#include <QMenu>
#include <QContextMenuEvent>
#include <QJsonObject>
#include <QJsonDocument>
#include <QResizeEvent>
#include <QGraphicsSceneMouseEvent>

// 슬라이드 내 텍스트 박스 데이터 구조
struct TextBoxData {
    int x = 100;
    int y = 100;
    int width = 400;
    int height = 150;
    QString text;
    QString fontFamily = "Arial";
    int fontSize = 24;
    QString fontColor = "#FFFFFF";
    QString backgroundColor = "#000000";
    int backgroundOpacity = 80;  // 0-100
    QString textAlign = "center";  // left, center, right
    QString verticalAlign = "middle";  // top, middle, bottom
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool wordWrap = true;
    int borderWidth = 0;
    QString borderColor = "#FFFFFF";
    
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
};

// 드래그 가능한 텍스트 박스 아이템
class DraggableTextBox : public QObject, public QGraphicsRectItem {
    Q_OBJECT
public:
    explicit DraggableTextBox(const TextBoxData &data, QGraphicsItem *parent = nullptr);
    
    void updateFromData(const TextBoxData &data);
    TextBoxData getData() const;
    void setData(const TextBoxData &data);
    
    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }
    
    // 편집 모드
    void startEditing();
    void stopEditing();
    bool isEditing() const { return m_editing; }

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    TextBoxData m_data;
    QGraphicsTextItem *m_textItem;
    QGraphicsProxyWidget *m_editProxy;
    QTextEdit *m_editWidget;
    bool m_selected;
    bool m_editing;
    bool m_dragging;
    QPointF m_dragStart;
    
    void updateTextDisplay();
    void updateGeometry();
    void showResizeHandles(bool show);

signals:
    void dataChanged();
    void selectionChanged(bool selected);
};

// 슬라이드 에디터 뷰
class SlideEditorView : public QGraphicsView {
    Q_OBJECT

public:
    explicit SlideEditorView(QWidget *parent = nullptr);
    
    void setSlideSize(int width, int height);
    QSize getSlideSize() const { return m_slideSize; }
    
    void addTextBox(const TextBoxData &data);
    void removeSelectedTextBox();
    void clearTextBoxes();
    
    QList<TextBoxData> getAllTextBoxData() const;
    void setAllTextBoxData(const QList<TextBoxData> &dataList);
    
    DraggableTextBox* getSelectedTextBox() const { return m_selectedBox; }
    void selectTextBox(DraggableTextBox *box);
    void deselectAll();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTextBoxSelectionChanged(bool selected);
    void onTextBoxDataChanged();

private:
    QGraphicsScene *m_scene;
    QSize m_slideSize;
    QList<DraggableTextBox*> m_textBoxes;
    DraggableTextBox *m_selectedBox;
    
    void updateSceneRect();

signals:
    void textBoxSelected(DraggableTextBox *box);
    void textBoxDeselected();
    void slideDataChanged();
};