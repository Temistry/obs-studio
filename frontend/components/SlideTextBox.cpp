#include "SlideTextBox.hpp"
#include <QPainter>
#include <QApplication>
#include <QKeyEvent>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFontMetrics>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextOption>
#include <QDebug>

// TextBoxData 구현
QJsonObject TextBoxData::toJson() const {
    QJsonObject json;
    json["x"] = x;
    json["y"] = y;
    json["width"] = width;
    json["height"] = height;
    json["text"] = text;
    json["fontFamily"] = fontFamily;
    json["fontSize"] = fontSize;
    json["fontColor"] = fontColor;
    json["backgroundColor"] = backgroundColor;
    json["backgroundOpacity"] = backgroundOpacity;
    json["textAlign"] = textAlign;
    json["verticalAlign"] = verticalAlign;
    json["bold"] = bold;
    json["italic"] = italic;
    json["underline"] = underline;
    json["wordWrap"] = wordWrap;
    json["borderWidth"] = borderWidth;
    json["borderColor"] = borderColor;
    return json;
}

void TextBoxData::fromJson(const QJsonObject &json) {
    x = json["x"].toInt(100);
    y = json["y"].toInt(100);
    width = json["width"].toInt(400);
    height = json["height"].toInt(150);
    text = json["text"].toString();
    fontFamily = json["fontFamily"].toString("Arial");
    fontSize = json["fontSize"].toInt(24);
    fontColor = json["fontColor"].toString("#FFFFFF");
    backgroundColor = json["backgroundColor"].toString("#000000");
    backgroundOpacity = json["backgroundOpacity"].toInt(80);
    textAlign = json["textAlign"].toString("center");
    verticalAlign = json["verticalAlign"].toString("middle");
    bold = json["bold"].toBool(false);
    italic = json["italic"].toBool(false);
    underline = json["underline"].toBool(false);
    wordWrap = json["wordWrap"].toBool(true);
    borderWidth = json["borderWidth"].toInt(0);
    borderColor = json["borderColor"].toString("#FFFFFF");
}

// DraggableTextBox 구현
DraggableTextBox::DraggableTextBox(const TextBoxData &data, QGraphicsItem *parent)
    : QObject(), QGraphicsRectItem(parent), m_data(data), m_textItem(nullptr), 
      m_editProxy(nullptr), m_editWidget(nullptr), m_selected(false), 
      m_editing(false), m_dragging(false)
{
    setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsFocusable);
    setAcceptHoverEvents(true);
    
    // 텍스트 아이템 생성
    m_textItem = new QGraphicsTextItem(this);
    
    updateFromData(data);
}

void DraggableTextBox::updateFromData(const TextBoxData &data) {
    m_data = data;
    updateGeometry();
    updateTextDisplay();
}

void DraggableTextBox::updateGeometry() {
    setRect(0, 0, m_data.width, m_data.height);
    setPos(m_data.x, m_data.y);
    
    if (m_textItem) {
        m_textItem->setPos(0, 0);
        m_textItem->setTextWidth(m_data.width);
    }
}

void DraggableTextBox::updateTextDisplay() {
    if (!m_textItem) return;
    
    // 폰트 설정
    QFont font(m_data.fontFamily, m_data.fontSize);
    font.setBold(m_data.bold);
    font.setItalic(m_data.italic);
    font.setUnderline(m_data.underline);
    
    // HTML로 텍스트 스타일 적용
    QString htmlText = QString("<div style=\"color: %1; text-align: %2; line-height: 1.2;\">%3</div>")
                       .arg(m_data.fontColor)
                       .arg(m_data.textAlign)
                       .arg(m_data.text.toHtmlEscaped().replace("\n", "<br>"));
    
    m_textItem->setHtml(htmlText);
    m_textItem->setFont(font);
    m_textItem->setTextWidth(m_data.width);
    
    // 수직 정렬을 위한 위치 조정
    QRectF textBounds = m_textItem->boundingRect();
    qreal yOffset = 0;
    if (m_data.verticalAlign == "middle") {
        yOffset = (m_data.height - textBounds.height()) / 2.0;
    } else if (m_data.verticalAlign == "bottom") {
        yOffset = m_data.height - textBounds.height();
    }
    m_textItem->setPos(0, yOffset);
}

TextBoxData DraggableTextBox::getData() const {
    TextBoxData data = m_data;
    data.x = pos().x();
    data.y = pos().y();
    return data;
}

void DraggableTextBox::setData(const TextBoxData &data) {
    updateFromData(data);
    emit dataChanged();
}

void DraggableTextBox::setSelected(bool selected) {
    if (m_selected != selected) {
        m_selected = selected;
        update();
        emit selectionChanged(selected);
    }
}

void DraggableTextBox::startEditing() {
    if (m_editing) return;
    
    m_editing = true;
    
    // 편집 위젯 생성
    m_editWidget = new QTextEdit();
    m_editWidget->setPlainText(m_data.text);
    m_editWidget->setFont(QFont(m_data.fontFamily, m_data.fontSize));
    m_editWidget->setStyleSheet(QString("QTextEdit { background-color: %1; color: %2; border: 2px solid #00ff00; }")
                               .arg(m_data.backgroundColor).arg(m_data.fontColor));
    
    // 프록시 위젯으로 씬에 추가
    m_editProxy = new QGraphicsProxyWidget(this);
    m_editProxy->setWidget(m_editWidget);
    m_editProxy->setPos(0, 0);
    m_editProxy->resize(m_data.width, m_data.height);
    
    // 텍스트 아이템 숨기기
    m_textItem->setVisible(false);
    
    m_editWidget->setFocus();
    m_editWidget->selectAll();
}

void DraggableTextBox::stopEditing() {
    if (!m_editing) return;
    
    m_editing = false;
    
    // 편집된 텍스트 저장
    if (m_editWidget) {
        m_data.text = m_editWidget->toPlainText();
    }
    
    // 편집 위젯 제거
    if (m_editProxy) {
        m_editProxy->setParent(nullptr);
        delete m_editProxy;
        m_editProxy = nullptr;
    }
    m_editWidget = nullptr;
    
    // 텍스트 아이템 다시 표시
    m_textItem->setVisible(true);
    updateTextDisplay();
    
    emit dataChanged();
}

void DraggableTextBox::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragStart = event->pos();
        setSelected(true);
    }
    QGraphicsRectItem::mousePressEvent(event);
}

void DraggableTextBox::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    if (m_dragging && !m_editing) {
        QPointF delta = event->pos() - m_dragStart;
        setPos(pos() + delta);
        m_data.x = pos().x();
        m_data.y = pos().y();
        emit dataChanged();
    }
    QGraphicsRectItem::mouseMoveEvent(event);
}

void DraggableTextBox::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    m_dragging = false;
    QGraphicsRectItem::mouseReleaseEvent(event);
}

void DraggableTextBox::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {
    Q_UNUSED(event)
    startEditing();
}

void DraggableTextBox::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {
    QMenu menu;
    
    QAction *editAction = menu.addAction("텍스트 편집 (&E)");
    QAction *fontAction = menu.addAction("글꼴 설정 (&F)");
    QAction *colorAction = menu.addAction("색상 설정 (&C)");
    menu.addSeparator();
    QAction *deleteAction = menu.addAction("삭제 (&D)");
    
    QAction *selectedAction = menu.exec(event->screenPos());
    
    if (selectedAction == editAction) {
        startEditing();
    } else if (selectedAction == fontAction) {
        bool ok;
        QFont currentFont(m_data.fontFamily, m_data.fontSize);
        currentFont.setBold(m_data.bold);
        currentFont.setItalic(m_data.italic);
        currentFont.setUnderline(m_data.underline);
        
        QFont newFont = QFontDialog::getFont(&ok, currentFont);
        if (ok) {
            m_data.fontFamily = newFont.family();
            m_data.fontSize = newFont.pointSize();
            m_data.bold = newFont.bold();
            m_data.italic = newFont.italic();
            m_data.underline = newFont.underline();
            updateTextDisplay();
            emit dataChanged();
        }
    } else if (selectedAction == colorAction) {
        QColor currentColor(m_data.fontColor);
        QColor newColor = QColorDialog::getColor(currentColor);
        if (newColor.isValid()) {
            m_data.fontColor = newColor.name();
            updateTextDisplay();
            emit dataChanged();
        }
    } else if (selectedAction == deleteAction) {
        // 삭제는 부모에서 처리
        deleteLater();
    }
}

void DraggableTextBox::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    Q_UNUSED(option)
    Q_UNUSED(widget)
    
    // 배경 그리기
    QColor bgColor(m_data.backgroundColor);
    bgColor.setAlpha(m_data.backgroundOpacity * 255 / 100);
    painter->fillRect(rect(), bgColor);
    
    // 테두리 그리기
    if (m_data.borderWidth > 0) {
        QPen borderPen(QColor(m_data.borderColor), m_data.borderWidth);
        painter->setPen(borderPen);
        painter->drawRect(rect());
    }
    
    // 선택 표시
    if (m_selected) {
        QPen selectionPen(Qt::yellow, 2, Qt::DashLine);
        painter->setPen(selectionPen);
        painter->drawRect(rect().adjusted(-1, -1, 1, 1));
    }
}

// SlideEditorView 구현
SlideEditorView::SlideEditorView(QWidget *parent)
    : QGraphicsView(parent), m_scene(nullptr), m_slideSize(1920, 1080), m_selectedBox(nullptr)
{
    m_scene = new QGraphicsScene(this);
    setScene(m_scene);
    
    // 뷰 설정
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    
    updateSceneRect();
}

void SlideEditorView::setSlideSize(int width, int height) {
    m_slideSize = QSize(width, height);
    updateSceneRect();
}

void SlideEditorView::updateSceneRect() {
    m_scene->setSceneRect(0, 0, m_slideSize.width(), m_slideSize.height());
    
    // 씬 배경 설정
    m_scene->setBackgroundBrush(QBrush(Qt::black));
}

void SlideEditorView::addTextBox(const TextBoxData &data) {
    DraggableTextBox *textBox = new DraggableTextBox(data);
    m_textBoxes.append(textBox);
    m_scene->addItem(textBox);
    
    connect(textBox, &DraggableTextBox::selectionChanged, this, &SlideEditorView::onTextBoxSelectionChanged);
    connect(textBox, &DraggableTextBox::dataChanged, this, &SlideEditorView::onTextBoxDataChanged);
    
    selectTextBox(textBox);
    emit slideDataChanged();
}

void SlideEditorView::removeSelectedTextBox() {
    if (m_selectedBox) {
        m_textBoxes.removeOne(m_selectedBox);
        m_scene->removeItem(m_selectedBox);
        delete m_selectedBox;
        m_selectedBox = nullptr;
        emit textBoxDeselected();
        emit slideDataChanged();
    }
}

void SlideEditorView::clearTextBoxes() {
    for (DraggableTextBox *box : m_textBoxes) {
        m_scene->removeItem(box);
        delete box;
    }
    m_textBoxes.clear();
    m_selectedBox = nullptr;
    emit textBoxDeselected();
    emit slideDataChanged();
}

QList<TextBoxData> SlideEditorView::getAllTextBoxData() const {
    QList<TextBoxData> dataList;
    for (const DraggableTextBox *box : m_textBoxes) {
        dataList.append(box->getData());
    }
    return dataList;
}

void SlideEditorView::setAllTextBoxData(const QList<TextBoxData> &dataList) {
    clearTextBoxes();
    for (const TextBoxData &data : dataList) {
        addTextBox(data);
    }
}

void SlideEditorView::selectTextBox(DraggableTextBox *box) {
    if (m_selectedBox != box) {
        if (m_selectedBox) {
            m_selectedBox->setSelected(false);
        }
        m_selectedBox = box;
        if (m_selectedBox) {
            m_selectedBox->setSelected(true);
            emit textBoxSelected(box);
        } else {
            emit textBoxDeselected();
        }
    }
}

void SlideEditorView::deselectAll() {
    selectTextBox(nullptr);
}

void SlideEditorView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}

void SlideEditorView::mousePressEvent(QMouseEvent *event) {
    QGraphicsItem *item = itemAt(event->pos());
    if (!item) {
        deselectAll();
    }
    QGraphicsView::mousePressEvent(event);
}

void SlideEditorView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Delete && m_selectedBox) {
        removeSelectedTextBox();
    } else if (event->key() == Qt::Key_Escape && m_selectedBox && m_selectedBox->isEditing()) {
        m_selectedBox->stopEditing();
    }
    QGraphicsView::keyPressEvent(event);
}

void SlideEditorView::onTextBoxSelectionChanged(bool selected) {
    DraggableTextBox *sender = qobject_cast<DraggableTextBox*>(QObject::sender());
    if (selected && sender) {
        selectTextBox(sender);
    }
}

void SlideEditorView::onTextBoxDataChanged() {
    emit slideDataChanged();
}

#include "moc_SlideTextBox.cpp"