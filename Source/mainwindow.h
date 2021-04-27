#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>

#include "simulationscene.h"
#include "simulationhandler.h"

namespace DrawActions {
enum DrawActions {
    None,
    Erase,
    Building,
    Emitter,
    Receiver,
};
}

namespace UIMode {
enum UIMode {
    EditorMode,
    SimulationMode
};
}


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();


public slots:
    void updateSceneRect();
    void moveSceneView(QPointF delta);
    void scaleView(double scale, QPointF pos = QPointF());
    void resetView();
    void bestView();


    void addBuilding();
    void toggleEraseMode(bool state);
    void eraseAll();
    void addEmitter();
    void addReceiver();

    void actionOpen();
    void actionSave();

    void actionZoomIn();
    void actionZoomOut();
    void actionZoomReset();
    void actionZoomBest();

    void clearAllItems();
    void cancelCurrentDrawing();

    void setMouseTrackerVisible(bool visible);

protected:
    void closeEvent(QCloseEvent *event);
    void showEvent(QShowEvent *event);
    void resizeEvent(QResizeEvent *event);

private slots:
    void initMouseTracker();
    void setMouseTrackerPosition(QPoint pos);

    void graphicsSceneRightReleased(QGraphicsSceneMouseEvent *event);
    void graphicsSceneLeftPressed(QGraphicsSceneMouseEvent *event);
    void graphicsSceneLeftReleased(QGraphicsSceneMouseEvent *event);
    void graphicsSceneMouseMoved(QGraphicsSceneMouseEvent *event);
    void graphicsSceneDoubleClicked(QGraphicsSceneMouseEvent *event);

    void graphicsSceneWheelEvent(QGraphicsSceneWheelEvent *event);

    void keyPressed(QKeyEvent *e);

    void configureEmitter(Emitter *em);
    void configureReceiver(Receiver *re);


private:
    QPoint moveAligned(QPoint start, QPoint actual);
    QPoint attractivePoint(QPoint actual);

    Ui::MainWindow *ui;

    SimulationScene *m_scene;
    SimulationHandler *m_simulation_handler;

    DrawActions::DrawActions m_draw_action;
    QGraphicsItem *m_drawing_item;

    bool m_mouse_tracker_visible;
    QGraphicsLineItem *m_mouse_tracker_x;
    QGraphicsLineItem *m_mouse_tracker_y;

    bool m_dragging_view;

    UIMode::UIMode m_ui_mode;
};
#endif // MAINWINDOW_H
