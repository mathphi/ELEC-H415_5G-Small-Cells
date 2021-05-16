#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QButtonGroup>
#include <QActionGroup>

#include "simulationscene.h"
#include "simulationhandler.h"
#include "analysisline.h"

namespace DrawActions {
enum DrawActions {
    None,
    Erase,
    Building,
    Emitter,
    Receiver,
    AnalysisLine,
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

    void switchSimulationMode();
    void switchEditSceneMode();

    void simulationTypeChanged();
    void receiversAntennaChanged();

    void drawAnalysisLine();
    void simulationSetupAction();
    void simulationControlAction();
    void simulationResetAction();
    void exportSimulationAction();

    void raysCheckboxToggled(bool);
    void raysThresholdChanged(int val);

    void resultTypeSelectionChanged(int, bool checked);

    void simulationStarted();
    void simulationFinished();
    void simulationCancelled();
    void simulationProgress(double);

private:
    void updateSimulationUI();
    void updateSimulationScene();
    void updateResultTypeRadios();
    void setPointReceiversVisible(bool visible);
    void createSimArea();
    void deleteSimArea();
    void deleteAnalysisLine();

    void simulationReset();
    bool askSimulationReset();
    void filterRaysThreshold();
    void showReceiversResult();
    void showResultsRays();
    void showResultHeatMap();
    void showResultPlot1D();


    QPoint moveAligned(QPoint start, QPoint actual);
    QPoint attractivePoint(QPoint actual);

    Ui::MainWindow *ui;
    UIMode::UIMode m_ui_mode;

    SimulationScene *m_scene;
    SimulationHandler *m_simulation_handler;

    DrawActions::DrawActions m_draw_action;
    QGraphicsItem *m_drawing_item;

    bool m_mouse_tracker_visible;
    QGraphicsLineItem *m_mouse_tracker_x;
    QGraphicsLineItem *m_mouse_tracker_y;

    bool m_dragging_view;

    ReceiversArea *m_sim_area_item;
    AnalysisLine *m_analysis_line;

    QButtonGroup *m_result_radio_grp;
    QActionGroup *m_map_edit_act_grp;
};
#endif // MAINWINDOW_H
