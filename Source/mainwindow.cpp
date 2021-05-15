#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "constants.h"
#include "emitter.h"
#include "receiver.h"
#include "building.h"
#include "emitterdialog.h"
#include "receiverdialog.h"
#include "buildingdialog.h"
#include "simsetupdialog.h"

#include <QDebug>
#include <QMessageBox>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QFileDialog>
#include <QLabel>

#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneWheelEvent>
#include <QKeyEvent>

#define ERASER_SIZE 20
#define ALIGN_THRESHOLD 16
#define PROXIMITY_SIZE 16

#define BUILDING_GRID_SIZE 5 // meters

// Extension for saved files (Ray-Tracing Small-Cells MAP)
#define FILE_EXTENSION "rtscmap"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // This attribute will store the type of item we are drawing (a building, an emitter,...)
    m_draw_action = DrawActions::None;

    // This attribute will store the item we are drawing (a line, a rectangle,...)
    m_drawing_item = nullptr;

    // This attribute is true when we are dragging the scene view with the mouse
    m_dragging_view = false;

    // The default mode for UI is the EditorMode
    m_ui_mode = UIMode::EditorMode;

    // Create the graphics scene
    m_scene = new SimulationScene();
    ui->graphicsView->setScene(m_scene);
    ui->graphicsView->setMouseTracking(true);

    // Dimensions of the scene
    QRect scene_rect(QPoint(0,0), ui->graphicsView->size());
    ui->graphicsView->setSceneRect(scene_rect);
    ui->graphicsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->graphicsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Enable antialiasing for the graphics view
    ui->graphicsView->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    // The simulation handler manages the simulation's data
    m_simulation_handler = new SimulationHandler();

    // Hide the simulation group by default
    ui->group_simulation->hide();

    // Hide the antenna type combobox by default
    ui->group_antenna_type->hide();

    // Add items to the antenna type combobox
    for (AntennaType::AntennaType type : AntennaType::AntennaTypeList) {
        // Get an antenna's instance of this type
        Antenna *ant = Antenna::createAntenna(type, 1.0);

        // Add item for each type
        ui->combobox_antennas_type->addItem(ant->getAntennaName(), type);

        // We don't need the antenna's instance anymore
        delete ant;
    }

    // Create a button group with result type radio buttons
    m_result_radio_grp = new QButtonGroup(this);
    m_result_radio_grp->addButton(ui->radio_result_power, ResultType::Power);
    m_result_radio_grp->addButton(ui->radio_result_SNR,   ResultType::SNR);
    m_result_radio_grp->addButton(ui->radio_result_delay, ResultType::DelaySpread);
    m_result_radio_grp->addButton(ui->radio_result_rice,  ResultType::RiceFactor);

    // Create an action group with map editing actions
    m_map_edit_act_grp = new QActionGroup(this);
    m_map_edit_act_grp->addAction(ui->actionAddBuilding);
    m_map_edit_act_grp->addAction(ui->actionAddEmitter);
    m_map_edit_act_grp->addAction(ui->actionAddReceiver);
    m_map_edit_act_grp->addAction(ui->actionEraseObject);
    m_map_edit_act_grp->addAction(ui->actionEraseAll);

    // Initialize simulation area to nullptr
    m_sim_area_item = nullptr;

    // Window File menu actions
    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionOpen, SIGNAL(triggered()), this, SLOT(actionOpen()));
    connect(ui->actionSave, SIGNAL(triggered()), this, SLOT(actionSave()));

    // Window Edit menu actions
    connect(ui->actionAddBuilding,      SIGNAL(triggered()),     this, SLOT(addBuilding()));
    connect(ui->actionAddEmitter,       SIGNAL(triggered()),     this, SLOT(addEmitter()));
    connect(ui->actionAddReceiver,      SIGNAL(triggered()),     this, SLOT(addReceiver()));
    connect(ui->actionEraseObject,      SIGNAL(toggled(bool)),   this, SLOT(toggleEraseMode(bool)));
    connect(ui->actionEraseAll,         SIGNAL(triggered()),     this, SLOT(eraseAll()));
    connect(ui->actionSimulation_setup, SIGNAL(triggered()),     this, SLOT(simulationSetupAction()));

    // Window View menu actions
    connect(ui->actionZoomIn,       SIGNAL(triggered()), this, SLOT(actionZoomIn()));
    connect(ui->actionZoomOut,      SIGNAL(triggered()), this, SLOT(actionZoomOut()));
    connect(ui->actionZoomReset,    SIGNAL(triggered()), this, SLOT(actionZoomReset()));
    connect(ui->actionZoomFit,      SIGNAL(triggered()), this, SLOT(actionZoomBest()));

    // Right-panel buttons
    // Scene edition buttons group
    connect(ui->button_addBuilding,     SIGNAL(clicked()),      this, SLOT(addBuilding()));
    connect(ui->button_addEmitter,      SIGNAL(clicked()),      this, SLOT(addEmitter()));
    connect(ui->button_addReceiver,     SIGNAL(clicked()),      this, SLOT(addReceiver()));
    connect(ui->button_eraseObject,     SIGNAL(clicked(bool)),  this, SLOT(toggleEraseMode(bool)));
    connect(ui->button_eraseAll,        SIGNAL(clicked()),      this, SLOT(eraseAll()));
    connect(ui->button_simulation,      SIGNAL(clicked()),      this, SLOT(switchSimulationMode()));


    // Simulation buttons group
    connect(ui->combobox_simType,  SIGNAL(currentIndexChanged(int)),
            this, SLOT(simulationTypeChanged()));
    connect(ui->combobox_antennas_type, SIGNAL(currentIndexChanged(int)),
            this, SLOT(receiversAntennaChanged()));

    connect(ui->button_simSetup,   SIGNAL(clicked()),         this, SLOT(simulationSetupAction()));
    connect(ui->button_simControl, SIGNAL(clicked()),         this, SLOT(simulationControlAction()));
    connect(ui->button_simReset,   SIGNAL(clicked()),         this, SLOT(simulationResetAction()));
    connect(ui->button_editScene,  SIGNAL(clicked()),         this, SLOT(switchEditSceneMode()));
    connect(ui->button_simExport,  SIGNAL(clicked()),         this, SLOT(exportSimulationAction()));

    connect(ui->checkbox_rays,     SIGNAL(toggled(bool)),     this, SLOT(raysCheckboxToggled(bool)));
    connect(ui->slider_threshold,  SIGNAL(valueChanged(int)), this, SLOT(raysThresholdChanged(int)));

    connect(m_result_radio_grp, SIGNAL(idToggled(int,bool)),  this, SLOT(resultTypeSelectionChanged(int,bool)));

    // Simulation handler signals
    connect(m_simulation_handler, SIGNAL(simulationStarted()), this, SLOT(simulationStarted()));
    connect(m_simulation_handler, SIGNAL(simulationFinished()), this, SLOT(simulationFinished()));
    connect(m_simulation_handler, SIGNAL(simulationCancelled()), this, SLOT(simulationCancelled()));
    connect(m_simulation_handler, SIGNAL(simulationProgress(double)), this, SLOT(simulationProgress(double)));

    // Scene events handling
    connect(m_scene, SIGNAL(mouseRightReleased(QGraphicsSceneMouseEvent*)),
            this, SLOT(graphicsSceneRightReleased(QGraphicsSceneMouseEvent*)));
    connect(m_scene, SIGNAL(mouseLeftPressed(QGraphicsSceneMouseEvent*)),
            this, SLOT(graphicsSceneLeftPressed(QGraphicsSceneMouseEvent*)));
    connect(m_scene, SIGNAL(mouseLeftReleased(QGraphicsSceneMouseEvent*)),
            this, SLOT(graphicsSceneLeftReleased(QGraphicsSceneMouseEvent*)));
    connect(m_scene, SIGNAL(mouseMoved(QGraphicsSceneMouseEvent*)),
            this, SLOT(graphicsSceneMouseMoved(QGraphicsSceneMouseEvent*)));
    connect(m_scene, SIGNAL(mouseDoubleClicked(QGraphicsSceneMouseEvent*)),
            this, SLOT(graphicsSceneDoubleClicked(QGraphicsSceneMouseEvent*)));
    connect(m_scene, SIGNAL(mouseWheelEvent(QGraphicsSceneWheelEvent*)),
            this, SLOT(graphicsSceneWheelEvent(QGraphicsSceneWheelEvent*)));
    connect(m_scene, SIGNAL(keyPressed(QKeyEvent*)), this, SLOT(keyPressed(QKeyEvent*)));

    // Initialize the mouse tracker on the scene
    initMouseTracker();

    // Update the simulation UI to match the simulation data
    updateSimulationUI();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    int ans = QMessageBox::question(
                this,
                "Quit",
                "Unsaved changes will be lost.\n"
                "Are you sure you want to quit the simulation?");

    // Close the window only if the user clicked the Yes button
    if (ans == QMessageBox::Yes) {
        event->accept();
    }
    else {
        event->ignore();
    }
}

void MainWindow::showEvent(QShowEvent *event) {
    event->accept();

    if (!isVisible()) {
        resetView();

    }
    updateSceneRect();
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    event->accept();
    updateSceneRect();
}

/**
 * @brief MainWindow::updateSceneRect
 *
 * This function updates the scene rect (when the window is resized or shown)
 */
void MainWindow::updateSceneRect() {
    // The scale factor is the diagonal components of the transformation matrix
    qreal scale_factor = ui->graphicsView->transform().m11();

    // Offset to avoid the scrollbars
    int offset = ceil(2/scale_factor);

    // Get the previous scene rect defined and extract his position from the center of the graphics view
    QRectF prev_rect = ui->graphicsView->sceneRect();
    QPointF prev_pos =
            prev_rect.topLeft() + QPointF(
                prev_rect.width() + offset, prev_rect.height() + offset) / 2.0;

    // Apply the previous position to the new graphics view size
    // Remove 10px to the new size to avoid the scrolls bars
    QPointF new_pos = prev_pos - QPointF(ui->graphicsView->width(), ui->graphicsView->height()) / scale_factor / 2.0;
    QRectF new_rect(new_pos, ui->graphicsView->size() / scale_factor - QSize(offset, offset));

    // Apply the new computed scene rect
    ui->graphicsView->setSceneRect(new_rect);

    // Send the changed of the scene rect to the scene
    m_scene->viewRectChanged(ui->graphicsView->sceneRect(), ui->graphicsView->transform().m11());
}

void MainWindow::moveSceneView(QPointF delta) {
    ui->graphicsView->setSceneRect(
                ui->graphicsView->sceneRect().x() + delta.x(),
                ui->graphicsView->sceneRect().y() + delta.y(),
                ui->graphicsView->sceneRect().width(),
                ui->graphicsView->sceneRect().height());

    // Send the changed of the scene rect to the scene
    m_scene->viewRectChanged(ui->graphicsView->sceneRect(), ui->graphicsView->transform().m11());
}

void MainWindow::scaleView(double scale, QPointF pos) {
    // Don't scale too high or too low
    if (ui->graphicsView->transform().m11() * scale > 10.0 ||
            ui->graphicsView->transform().m11() * scale < 0.1) {
        return;
    }

    QRectF scene_rect = ui->graphicsView->sceneRect();

    // Compute the position of the mouse from the center of the scene
    QPointF centered_pos = pos - scene_rect.topLeft() - (scene_rect.bottomRight() - scene_rect.topLeft()) / 2.0;

    // Compute a delta position proportionnal to the scale factor and
    // the centered mouse position
    QPointF delta_pos = (scale - 1.0) * centered_pos;

    // Apply the scaling and the delta position
    ui->graphicsView->scale(scale, scale);
    moveSceneView(delta_pos);

    // The scene dimensions changed
    updateSceneRect();
}

void MainWindow::resetView() {
    ui->graphicsView->resetTransform();
    updateSceneRect();

    QPointF view_delta(
                ui->graphicsView->sceneRect().x() + ui->graphicsView->sceneRect().width() / 2.0,
                ui->graphicsView->sceneRect().y() + ui->graphicsView->sceneRect().height() / 2.0);

    moveSceneView(-view_delta);
}

void MainWindow::bestView() {
    // Get the bounding rectangle of all items of the scene
    QRectF bounding_rect = m_scene->simulationBoundingRect();

    // If there is nothing on the scene, reset the view
    if (bounding_rect.isNull()) {
        resetView();
        return;
    }

    // Add a margin to this rectangle
    bounding_rect.adjust(-50.0, -50.0, 50.0, 50.0);

    // The view scale is the diagonal components of the transformation matrix
    qreal view_scale = ui->graphicsView->transform().m11();

    // Get the most limiting scale factor
    qreal scale_factor = qMin(
                ui->graphicsView->width() / bounding_rect.width(),
                ui->graphicsView->height() / bounding_rect.height());

    scale_factor /= view_scale;

    // Offset to avoid the scrollbars
    int offset = ceil(2/view_scale);

    // Get the new rect
    QRectF view_rect(
                bounding_rect.x() + bounding_rect.width() / 2.0 - ui->graphicsView->width() / view_scale / 2.0,
                bounding_rect.y() + bounding_rect.height() / 2.0 - ui->graphicsView->height() / view_scale / 2.0,
                ui->graphicsView->width() / view_scale - offset,
                ui->graphicsView->height() / view_scale - offset);

    // Scale the view to fit the bounding rect in the view
    scaleView(scale_factor);

    // Apply the new rect scaled by the scale_factor
    ui->graphicsView->setSceneRect(QRectF(view_rect.topLeft(), view_rect.size()));

    // Clean the scene rect dimensions
    updateSceneRect();
}

void MainWindow::toggleEraseMode(bool state) {
    // Set both button and menu's action state
    ui->button_eraseObject->setChecked(state);
    ui->actionEraseObject->setChecked(state);

    if (state) {
        cancelCurrentDrawing();

        // Begin erasing
        m_draw_action = DrawActions::Erase;

        // Draw a dashed rectangle that will follow the mouse cursor (erasing area)
        QPen pen(QBrush(Qt::gray), 1, Qt::DashLine);
        QGraphicsRectItem *rect_item = new QGraphicsRectItem(0, 0, ERASER_SIZE, ERASER_SIZE);
        rect_item->hide();
        rect_item->setPen(pen);

        m_drawing_item = rect_item;
        m_scene->addItem(m_drawing_item);
    }
    else {
        // Stop erasing
        cancelCurrentDrawing();
    }
}

void MainWindow::eraseAll() {
    int answer = QMessageBox::question(
                    this,
                    "Confirmation of erasing",
                    "Are you sure you want to erase everything?");

    if (answer == QMessageBox::Yes) {
        clearAllItems();
    }
}


/**
 * @brief MainWindow::addBuilding
 * Slot called when the button "Add a building" is clicked
 */
void MainWindow::addBuilding() {
    cancelCurrentDrawing();

    BuildingDialog building_dialog(this);
    int ans = building_dialog.exec();

    if (ans == QDialog::Rejected)
        return;

    QSize building_size = building_dialog.getBuildingSize();

    // Create an building of the configured size
    m_drawing_item = new Building(building_size * m_scene->simulationScale());

    // We are placing a building
    m_draw_action = DrawActions::Building;

    // Hide the item until the mouse come on the scene
    m_drawing_item->setVisible(false);
    m_scene->addItem(m_drawing_item);
}

void MainWindow::configureEmitter(Emitter *em) {
    // Dialog to configure the emitter
    EmitterDialog emitter_dialog(em, this);
    int ans = emitter_dialog.exec();

    if (ans == QDialog::Rejected)
        return;

    AntennaType::AntennaType type = emitter_dialog.getAntennaType();
    double eirp       = emitter_dialog.getEIRP();
    double frequency  = emitter_dialog.getFrequency();
    double efficiency = emitter_dialog.getEfficiency();

    // Update the emitter
    em->setEIRP(eirp);
    em->setFrequency(frequency);
    em->setAntenna(type, efficiency);
}

void MainWindow::configureReceiver(Receiver *re) {
    // Dialog to configure the emitter
    ReceiverDialog receiver_dialog(re, this);
    int ans = receiver_dialog.exec();

    if (ans == QDialog::Rejected)
        return;

    AntennaType::AntennaType type = receiver_dialog.getAntennaType();
    double efficiency = receiver_dialog.getEfficiency();

    // Update the receiver
    re->setAntenna(type, efficiency);
}

void MainWindow::addEmitter() {
    cancelCurrentDrawing();

    // Dialog to configure the emitter
    EmitterDialog emitter_dialog(this);
    int ans = emitter_dialog.exec();

    if (ans == QDialog::Rejected)
        return;

    AntennaType::AntennaType type = emitter_dialog.getAntennaType();
    double eirp      = emitter_dialog.getEIRP();
    double frequency  = emitter_dialog.getFrequency();
    double efficiency = emitter_dialog.getEfficiency();

    // Create an emitter of the selected type to place on the scene
    m_drawing_item = new Emitter(frequency, eirp, efficiency, type);

    // We are placing an emitter
    m_draw_action = DrawActions::Emitter;

    // Hide the item until the mouse come on the scene
    m_drawing_item->setVisible(false);
    m_scene->addItem(m_drawing_item);
}

void MainWindow::addReceiver() {
    cancelCurrentDrawing();

    // Dialog to configure the emitter
    ReceiverDialog receiver_dialog(this);
    int ans = receiver_dialog.exec();

    if (ans == QDialog::Rejected)
        return;

    AntennaType::AntennaType type = receiver_dialog.getAntennaType();
    double efficiency = receiver_dialog.getEfficiency();

    // Create an receiver of the selected type to place on the scene
    m_drawing_item = new Receiver(type, efficiency);

    // Create an Receiver to place on the scene
    m_draw_action = DrawActions::Receiver;

    // Hide the item until the mouse come on the scene
    m_drawing_item->setVisible(false);
    m_scene->addItem(m_drawing_item);
}

/**
 * @brief MainWindow::clearAllItems
 *
 * This function resets all the scene, lists and actions
 */
void MainWindow::clearAllItems() {
    // Cancel the current drawing (if one)
    cancelCurrentDrawing();

    // Clear the lists
    m_simulation_handler->simulationData()->reset();

    // Remove all SimulationItem from the scene
    foreach (QGraphicsItem *item, m_scene->items()) {
        // Don't remove other items (ie: mouse tracker lines or
        // eraser rectancgle) than the type SimulationItem
        if (!(dynamic_cast<SimulationItem*>(item))) {
            continue;
        }

        m_scene->removeItem(item);
        delete item;
    }
}

/**
 * @brief MainWindow::cancelCurrentDrawing
 *
 * This function cancels the current drawing action
 */
void MainWindow::cancelCurrentDrawing() {
    // If we were erasing, uncheck the "Erase object" button
    if (m_draw_action == DrawActions::Erase) {
        ui->button_eraseObject->setChecked(false);
        ui->actionEraseObject->setChecked(false);
    }

    // Remove the current placing object from the scene and delete it
    if (m_drawing_item) {
        m_scene->removeItem(m_drawing_item);
        delete m_drawing_item;
    }

    m_draw_action = DrawActions::None;
    m_drawing_item = nullptr;

    // Hide the mouse tracker
    setMouseTrackerVisible(false);
}


/**
 * @brief MainWindow::keyPressed
 * @param e
 *
 * Slot called when the used presses any key on the keyboard
 */
void MainWindow::keyPressed(QKeyEvent *e) {
    // Cancel current drawing on Escape pressed
    if (e->key() == Qt::Key_Escape) {
        cancelCurrentDrawing();
    }

    //////////////////// Keyboard controls of the scene view ////////////////////
    else if (e->key() == Qt::Key_Left) {
        moveSceneView(QPointF(-10, 0));
    }
    else if (e->key() == Qt::Key_Right) {
        moveSceneView(QPointF(10, 0));
    }
    else if (e->key() == Qt::Key_Up) {
        moveSceneView(QPointF(0, -10));
    }
    else if (e->key() == Qt::Key_Down) {
        moveSceneView(QPointF(0, 10));
    }
    /////////////////////////////////////////////////////////////////////////////
}

/**
 * @brief MainWindow::graphicsSceneWheelEvent
 * @param pos
 * @param delta
 * @param mod_keys
 *
 * Slot called when the user use the mouse wheel.
 * It is used to zoom in/out the scene.
 */
void MainWindow::graphicsSceneWheelEvent(QGraphicsSceneWheelEvent *event) {
    qreal scale_factor = 1.0 - event->delta() / 5000.0;
    scaleView(scale_factor, event->scenePos());
}

/**
 * @brief MainWindow::graphicsSceneDoubleClicked
 * @param event
 *
 * Slot called when the user double click on the graphics scene
 */
void MainWindow::graphicsSceneDoubleClicked(QGraphicsSceneMouseEvent *event) {
    // If we are placing something -> nothing to do
    if (m_drawing_item != nullptr) {
        return;
    }

    // Don't edit if we aren't in editor mode
    if (m_ui_mode != UIMode::EditorMode)
        return;

    // Search area for a double click
    QRectF click_rect(event->scenePos() - QPointF(5,5), QSize(10,10));

    // Loop over the items under the mouse position
    foreach(QGraphicsItem *item, m_scene->items(click_rect)) {
        // Try to cast this item
        Emitter *em = dynamic_cast<Emitter*>(item);
        Receiver *re = dynamic_cast<Receiver*>(item);

        // If one of them is an Emitter -> configure it
        if (em != nullptr) {
            configureEmitter(em);
            break;
        }
        // If one of them is an Receiver -> configure it
        else if (re != nullptr) {
            configureReceiver(re);
            break;
        }
    }
}

/**
 * @brief MainWindow::graphicsSceneRightReleased
 *
 * Slot called when the user releases the right button on the graphics scene
 */
void MainWindow::graphicsSceneRightReleased(QGraphicsSceneMouseEvent *) {
    // Right click = cancel the current action
    cancelCurrentDrawing();
}

/**
 * @brief MainWindow::graphicsSceneLeftReleased
 * @param pos
 *
 * Slot called when the user presses the left button on the graphics scene
 */
void MainWindow::graphicsSceneLeftPressed(QGraphicsSceneMouseEvent *event) {
    Q_UNUSED(event);

    // If no draw action pending -> start view dragging
    if (m_draw_action == DrawActions::None) {
        m_dragging_view = true;
        ui->graphicsView->setCursor(Qt::ClosedHandCursor);
    }
}

/**
 * @brief MainWindow::graphicsSceneLeftReleased
 * @param pos
 *
 * Slot called when the user releases the left button on the graphics scene
 */
void MainWindow::graphicsSceneLeftReleased(QGraphicsSceneMouseEvent *event) {
    //QPoint pos = event->scenePos().toPoint();

    // End the dragging action when the mouse is released
    if (m_dragging_view) {
        m_dragging_view = false;
        ui->graphicsView->setCursor(Qt::ArrowCursor);
    }

    // If we are currently placing something
    if (m_drawing_item != nullptr)
    {
        // Actions to do when we are placing an item
        switch (m_draw_action) {
        //////////////////////////////// BUILDING ACTION ////////////////////////////////
        case DrawActions::Building: {
            Building *building = (Building*) m_drawing_item;

            // Add this building to the simulation data
            m_simulation_handler->simulationData()->attachBuilding(building);

            // Repeat the last action if the control or shift key was pressed
            if (event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) {
                // Clone the last placed receiver and place it
                m_drawing_item = building->clone();
                m_drawing_item->setVisible(false);
                m_scene->addItem(m_drawing_item);
            }
            else {
                // Detach the placed building from the mouse
                m_drawing_item = nullptr;
                m_draw_action = DrawActions::None;
            }
            break;
        }
        //////////////////////////////// EMITTER ACTION /////////////////////////////////
        case DrawActions::Emitter: {
            Emitter *emitter = (Emitter*) m_drawing_item;

            // Add this emitter to the simulation data
            m_simulation_handler->simulationData()->attachEmitter(emitter);

            // Repeat the last action if the control or shift key was pressed
            if (event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) {
                // Clone the last placed receiver and place it
                m_drawing_item = emitter->clone();
                m_drawing_item->setVisible(false);
                m_scene->addItem(m_drawing_item);
            }
            else {
                // Detach the placed emitter from the mouse
                m_drawing_item = nullptr;
                m_draw_action = DrawActions::None;
            }
            break;
        }
        //////////////////////////////// RECEIVER ACTION /////////////////////////////////
        case DrawActions::Receiver: {
            Receiver *receiver = (Receiver*) m_drawing_item;

            // Add this receiver to the simulation data
            m_simulation_handler->simulationData()->attachReceiver(receiver);

            // Repeat the last action if the control or shift key was pressed
            if (event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier)) {
                // Re-create a copy of the last placed receiver
                m_drawing_item = receiver->clone();
                m_drawing_item->setVisible(false);
                m_scene->addItem(m_drawing_item);
            }
            else {
                // Detach the placed received from the mouse
                m_drawing_item = nullptr;
                m_draw_action = DrawActions::None;
            }
            break;
        }
        //////////////////////////////// ERASE ACTION /////////////////////////////////
        case DrawActions::Erase: {
            QGraphicsRectItem *rect_item = (QGraphicsRectItem*) m_drawing_item;

            // Retreive all items under the eraser rectangle
            QRectF rect (rect_item->pos(), rect_item->rect().size());
            QList<QGraphicsItem*> trash = m_scene->items(rect);

            // Remove each items from the graphics scene and delete it
            foreach (QGraphicsItem *item, trash) {
                // Don't remove other items (ie: mouse tracker lines or
                // eraser rectancgle) than the type SimulationItem
                if (!(dynamic_cast<SimulationItem*>(item))) {
                    continue;
                }

                // Remove the item from the scene
                m_scene->removeItem(item);

                // Action for some types of items
                if (dynamic_cast<Building*>(item)) {
                    // Remove it from the buildings list
                    m_simulation_handler->simulationData()->detachBuilding((Building*) item);
                }
                else if (dynamic_cast<Emitter*>(item)){
                    m_simulation_handler->simulationData()->detachEmitter((Emitter*) item);
                }
                else if (dynamic_cast<Receiver*>(item)){
                    m_simulation_handler->simulationData()->detachReceiver((Receiver*) item);
                }

                delete item;
            }
            break;
        }
        default:
            break;
        }
    }

    // Show mouse tracker only if we are placing something
    setMouseTrackerVisible(m_draw_action != DrawActions::None);
}

/**
 * @brief MainWindow::graphicsSceneMouseMoved
 * @param pos
 *
 * Slot called when the mouse move over the graphics scene
 */
void MainWindow::graphicsSceneMouseMoved(QGraphicsSceneMouseEvent *event) {
    QPoint pos = event->scenePos().toPoint();

    // Move the scene to follow the drag movement of the mouse.
    // Use the screenPos that is invariant of the sceneRect.
    if (m_dragging_view) {
        qreal view_scale = ui->graphicsView->transform().m11();
        QPointF delta_mouse = event->lastScreenPos() - event->screenPos();
        moveSceneView(delta_mouse / view_scale);
    }

    // Show mouse tracker only if we are placing something
    setMouseTrackerVisible(m_draw_action != DrawActions::None);

    // Mouse tracker follows the mouse if visible
    if (m_mouse_tracker_visible) {
        setMouseTrackerPosition(pos);
    }

    // No more thing to do if we are not placing an item
    if (m_drawing_item == nullptr) {
        return;
    }

    switch (m_draw_action) {
    ////////////////////// BUILDING/EMITTER/RECEIVER ACTION ///////////////////////
    case DrawActions::Building: {
        Building *b = dynamic_cast<Building*>(m_drawing_item);

        // Scale is 1.0 * SimulationScale if the ALT modifier is pressed
        qreal b_scale = m_scene->simulationScale();

        // If ALT modifier is not pressed
        if (!(event->modifiers() & Qt::AltModifier)) {
            b_scale = m_scene->simulationScale() * BUILDING_GRID_SIZE;
        }

        // Center the building on the mouse
        QPoint centered_pos = pos - QPoint(b->getSize().width()/2, b->getSize().height()/2);

        m_drawing_item->setPos(QPoint(centered_pos / b_scale) * b_scale);

        if (!m_drawing_item->isVisible()) {
            m_drawing_item->setVisible(true);
        }
        break;
    }
    case DrawActions::Emitter:
    case DrawActions::Receiver: {
        // Scale is 1.0 * SimulationScale if the ALT modifier is pressed
        qreal scale = m_scene->simulationScale();

        // If ALT modifier is not pressed
        if (!(event->modifiers() & Qt::AltModifier)) {
            scale = m_scene->simulationScale() * BUILDING_GRID_SIZE;
        }

        m_drawing_item->setPos(QPoint(pos / scale) * scale);

        if (!m_drawing_item->isVisible()) {
            m_drawing_item->setVisible(true);
        }
        break;
    }
    //////////////////////////////// ERASE ACTION /////////////////////////////////
    case DrawActions::Erase: {
        // The rectangle of the eraser is centered on the mouse
        m_drawing_item->setPos(pos - QPoint(ERASER_SIZE/2,ERASER_SIZE/2));

        // The rectangle of the eraser starts hidden
        m_drawing_item->show();
        break;
    }
    default:
        break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////// MOUSE TRACKER SECTION ////////////////////////////////////////

void MainWindow::initMouseTracker() {
    // Add two lines to the scene that will track the mouse cursor when visible
    QPen tracker_pen(QBrush(QColor(0, 0, 255, 175)), 1.0 * devicePixelRatioF(), Qt::DotLine);
    tracker_pen.setCosmetic(true);  // Keep the same pen width even if the view is scaled

    m_mouse_tracker_x = new QGraphicsLineItem();
    m_mouse_tracker_y = new QGraphicsLineItem();

    m_mouse_tracker_x->setPen(tracker_pen);
    m_mouse_tracker_y->setPen(tracker_pen);

    setMouseTrackerVisible(false);

    m_scene->addItem(m_mouse_tracker_x);
    m_scene->addItem(m_mouse_tracker_y);
}

void MainWindow::setMouseTrackerVisible(bool visible) {
    m_mouse_tracker_visible = visible;

    m_mouse_tracker_x->setVisible(visible);
    m_mouse_tracker_y->setVisible(visible);

    // Hide the mouse cursor when we use the mouse tracker lines
    if (visible) {
        ui->graphicsView->setCursor(Qt::BlankCursor);
    }
    else if (ui->graphicsView->cursor() == Qt::BlankCursor) {
        ui->graphicsView->setCursor(Qt::ArrowCursor);
    }
}

void MainWindow::setMouseTrackerPosition(QPoint pos) {
    // Get the viewport dimensions
    QGraphicsView *view = ui->graphicsView;

    QLine x_line(pos.x(), view->sceneRect().y(),
                 pos.x(), view->sceneRect().y() + view->sceneRect().height()-1);

    QLine y_line(view->sceneRect().x(), pos.y(),
                 view->sceneRect().x() + view->sceneRect().width()-1, pos.y());

    m_mouse_tracker_x->setLine(x_line);
    m_mouse_tracker_y->setLine(y_line);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////// FILE SAVE/RESTORE HANDLING SECTION /////////////////////////////////

void MainWindow::actionOpen() {
    int answer = QMessageBox::question(
                this,
                "Confirmation",
                "The current state of the simulation will be lost.\n"
                "Do you want to continue?");

    if (answer == QMessageBox::No) {
        return;
    }

    QString file_path = QFileDialog::getOpenFileName(this, "Open a file", QString(), "*." FILE_EXTENSION);

    // If the user cancelled the dialog
    if (file_path.isEmpty()) {
        return;
    }

    // Open the file (reading)
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, "Error", "Unable to open file for reading");
        return;
    }

    // Clear all the current data
    simulationReset();

    // Delete the simulation area item and its receivers before to clear all items
    if (m_sim_area_item != nullptr) {
        delete m_sim_area_item;
        m_sim_area_item = nullptr;
    }

    // Clear the scene
    clearAllItems();

    // Read data from the file
    QDataStream in(&file);
    in >> m_simulation_handler->simulationData();

    // Update the graphics scene with read data
    foreach (Building* b, m_simulation_handler->simulationData()->getBuildingsList()) {
        m_scene->addItem(b);
    }
    foreach (Emitter* e, m_simulation_handler->simulationData()->getEmittersList()) {
        m_scene->addItem(e);
    }
    foreach (Receiver* r, m_simulation_handler->simulationData()->getReceiverList()) {
        m_scene->addItem(r);
    }

    updateSimulationUI();
    updateSimulationScene();

    // Go to the edit mode if there is no emitter in the scene
    if (m_simulation_handler->simulationData()->getEmittersList().size() < 1) {
        switchEditSceneMode();
    }

    // Close the file
    file.close();

    // Reset the view after opening the file
    resetView();
}

void MainWindow::actionSave() {
    QString file_path = QFileDialog::getSaveFileName(this, "Save to file", QString(), "*." FILE_EXTENSION);

    // If the used cancelled the dialog
    if (file_path.isEmpty()) {
        return;
    }

    // If the file hasn't the right extention -> add it
    if (file_path.split('.').last() != FILE_EXTENSION) {
        file_path.append("." FILE_EXTENSION);
    }

    QFile file(file_path);
    // Open the file (writing)
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Unable to open file for writing");
        return;
    }

    // Write current data into the file
    QDataStream out (&file);
    out << m_simulation_handler->simulationData();

    // Close the file
    file.close();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////// ZOOM ACTIONS FUNCTIONS ///////////////////////////////////////

void MainWindow::actionZoomIn() {
    scaleView(1.1);
}

void MainWindow::actionZoomOut() {
    scaleView(0.9);
}

void MainWindow::actionZoomReset() {
    resetView();
}

void MainWindow::actionZoomBest() {
    bestView();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////// PANEL SWITCHING FUNCTIONS /////////////////////////////////////


void MainWindow::switchSimulationMode() {
    if (m_ui_mode == UIMode::SimulationMode)
        return;

    // Set the current mode to SimulationMode
    m_ui_mode = UIMode::SimulationMode;

    // Hide the scene edition buttons group
    ui->group_scene_edition->hide();

    // Show the simulation buttons group
    ui->group_simulation->show();

    // Disable the map edition actions in menu bar
    m_map_edit_act_grp->setEnabled(false);

    // Cancel the current drawing (if one)
    cancelCurrentDrawing();

    // Update the simulation scene and UI according to new mode
    updateSimulationScene();
    updateSimulationUI();

    // Update the scene rect (since the view size can have changed)
    updateSceneRect();
}

void MainWindow::switchEditSceneMode() {
    if (m_ui_mode == UIMode::EditorMode)
        return;

    // This will reset the simulation data if the user accepts
    if (!askSimulationReset())
        // Don't continue if user refused
        return;

    // Set the current mode to EditorMode
    m_ui_mode = UIMode::EditorMode;

    // Hide the simulation buttons group
    ui->group_simulation->hide();

    // Show the scene edition buttons group
    ui->group_scene_edition->show();

    // Enable the map edition actions in menu bar
    m_map_edit_act_grp->setEnabled(true);

    // Update the simulation scene and UI according to new mode
    updateSimulationScene();
    updateSimulationUI();

    // Update the scene rect (since the view size can have changed)
    updateSceneRect();
}

void MainWindow::updateSimulationUI() {
    // Set the current simulation type
    ui->combobox_simType->setCurrentIndex(m_simulation_handler->simulationData()->simulationType());

    // Show/hide the progress bar
    ui->progressbar_simulation->setVisible(m_simulation_handler->isRunning());

    // Show/hide widget groups
    ui->group_show_rays->setVisible(SimulationHandler::simulationData()->simulationType() == SimType::PointReceiver);
    ui->group_result_type->setVisible(SimulationHandler::simulationData()->simulationType() == SimType::AreaReceiver);
    ui->group_antenna_type->setVisible(SimulationHandler::simulationData()->simulationType() == SimType::AreaReceiver);

    // Widgets enabling/disabling
    ui->label_threshold_msg->setEnabled(ui->checkbox_rays->isChecked());
    ui->label_threshold_val->setEnabled(ui->checkbox_rays->isChecked());
    ui->slider_threshold->setEnabled(ui->checkbox_rays->isChecked());

    // Enable/disable the UI controls if simulation is running
    ui->combobox_simType->setDisabled(m_simulation_handler->isRunning());
    ui->combobox_antennas_type->setDisabled(m_simulation_handler->isRunning());
    ui->button_simReset->setDisabled(m_simulation_handler->isRunning());
    ui->button_editScene->setDisabled(m_simulation_handler->isRunning());
    ui->actionOpen->setDisabled(m_simulation_handler->isRunning());
    ui->actionSimulation_setup->setDisabled(m_simulation_handler->isRunning());

    // Change the control button text
    ui->button_simControl->setEnabled(!m_simulation_handler->isCancelling());

    if (m_simulation_handler->isCancelling()) {
        ui->button_simControl->setText("Canceling...");
    }
    else if (m_simulation_handler->isRunning()) {
        ui->button_simControl->setText("Cancel simulation");
    }
    else  {
        ui->button_simControl->setText("Start simulation");
    }

    // Update result type buttons
    updateResultTypeRadios();
}

void MainWindow::updateSimulationScene() {
    // Hide receivers only if simulation mode and AreaReceiver simulation type
    if (m_ui_mode == UIMode::EditorMode) {
        setPointReceiversVisible(true);
        deleteSimArea();
    }
    else if (m_simulation_handler->simulationData()->simulationType() == SimType::PointReceiver) {
        setPointReceiversVisible(true);
        deleteSimArea();
    }
    else {
        setPointReceiversVisible(false);
        createSimArea();
    }
}

void MainWindow::updateResultTypeRadios() {
    // Disable some result type under certain conditions
    int em_count = m_simulation_handler->simulationData()->getEmittersList().size();

    // If there are more than one emitter -> no Delay Spread nor Rice Factor
    if (em_count > 1) {
        ui->radio_result_delay->setEnabled(false);
        ui->radio_result_rice->setEnabled(false);

        // Get the currently selected result type
        ResultType::ResultType res_type = (ResultType::ResultType) m_result_radio_grp->checkedId();

        // Select another radio if the currently checked is unavailable
        if (res_type == ResultType::DelaySpread ||
            res_type == ResultType::RiceFactor)
        {
            ui->radio_result_power->setChecked(true);
        }
    }
    else {
        ui->radio_result_delay->setEnabled(true);
        ui->radio_result_rice->setEnabled(true);
    }
}

void MainWindow::setPointReceiversVisible(bool visible) {
    foreach(Receiver *r, m_simulation_handler->simulationData()->getReceiverList()) {
        r->setVisible(visible);
    }
}

void MainWindow::createSimArea() {
    // Get the simulation bounding rect and selected antenna type
    QRectF area = m_scene->simulationBoundingRect();
    AntennaType::AntennaType type = (AntennaType::AntennaType) ui->combobox_antennas_type->currentData().toInt();

    if (m_sim_area_item == nullptr) {
        // Create the area rectangle
        m_sim_area_item = new ReceiversArea();
        m_scene->addItem((SimulationItem*) m_sim_area_item);
    }

    // Re-draw the simulation area
    // Set the area after the item is added to the scene!
    m_sim_area_item->setArea(type, area);
}

void MainWindow::deleteSimArea() {
    // Nothing to do if no sim area
    if (m_sim_area_item == nullptr)
        return;

    // Be sure the simulation is resetted
    simulationReset();

    // Remove the simulation area
    delete m_sim_area_item;
    m_sim_area_item = nullptr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////// SIMULATION PANEL FUNCTIONS /////////////////////////////////////


void MainWindow::simulationTypeChanged() {
    // Get the newly selected simulation type
    SimType::SimType sim_type = (SimType::SimType) ui->combobox_simType->currentIndex();

    // Actions if the type actually changed
    if (SimulationHandler::simulationData()->simulationType() != sim_type) {
        // Ask the user (since this will reset the simulation)
        int ans = QMessageBox::warning(
                    this,
                    "Simulation reset warning",
                    "Changing the simulation type will reset the current simulation results.\n"
                    "Are you sure you want to continue?",
                    QMessageBox::Yes | QMessageBox::No,
                    QMessageBox::No);

        // Revert the combobox if the answer is Yes
        if (ans != QMessageBox::Yes) {
            ui->combobox_simType->setCurrentIndex(SimulationHandler::simulationData()->simulationType());
            return;
        }

        // Reset the simulation results
        simulationReset();

        // Set the new simulation type in simulationData
        SimulationHandler::simulationData()->setSimulationType(sim_type);
    }

    updateSimulationUI();
    updateSimulationScene();
}

void MainWindow::receiversAntennaChanged() {
    // Nothing to do if there is no simulation area item
    if (m_sim_area_item == nullptr)
        return;

    // Get the currently selected antenna type
    AntennaType::AntennaType ant_type = (AntennaType::AntennaType) ui->combobox_antennas_type->currentData().toInt();

    // Change the antenna for all receivers in the sim_area_item
    foreach (Receiver *r, m_sim_area_item->getReceiversList()) {
        r->setAntenna(ant_type);
    }
}


void MainWindow::simulationSetupAction() {
    // Open the setup dialog
    SimSetupDialog setup_dialog(m_simulation_handler->simulationData());
    setup_dialog.exec();

    // Note that the setup dialog modifies itself the parameters in simulationData
}

void MainWindow::simulationControlAction() {
    // Get the simulation type
    SimType::SimType sim_type = SimulationHandler::simulationData()->simulationType();

    // Check the minimal requirements for this simulation type
    if ((sim_type == SimType::AreaReceiver || sim_type == SimType::PointReceiver)
            && SimulationHandler::simulationData()->getEmittersList().size() < 1)
    {
        QMessageBox::critical(
                    this,
                    "Simulation error",
                    "You need to place at least one emitter on the map in order to run this type of simulation");

        return;
    }
    else if (sim_type == SimType::PointReceiver
            && SimulationHandler::simulationData()->getReceiverList().size() < 1)
    {
        QMessageBox::critical(
                    this,
                    "Simulation error",
                    "You need to place at least one receiver on the map in order to run this type of simulation");

        return;
    }

    // If there is no simulation computation currently running
    if (!m_simulation_handler->isRunning())
    {
        // Start the computation for the current simulation type
        switch (m_simulation_handler->simulationData()->simulationType())
        {
        case SimType::PointReceiver: {
            QRectF area = m_scene->simulationBoundingRect();
            QList<Receiver*> rcv_list = m_simulation_handler->simulationData()->getReceiverList();
            m_simulation_handler->startSimulationComputation(rcv_list, area);
            break;
        }
        case SimType::AreaReceiver: {
            // If there is no simulation area
            if (m_sim_area_item == nullptr) {
                // This wouldn't happen
                return;
            }

            m_simulation_handler->startSimulationComputation(m_sim_area_item->getReceiversList(), m_sim_area_item->getArea());
            break;
        }
        }
    }
    else {
        // Cancel the current simulation
        m_simulation_handler->stopSimulationComputation();
    }

    updateSimulationUI();
}

void MainWindow::simulationResetAction() {
    askSimulationReset();
}

void MainWindow::exportSimulationAction() {
    // Cancel the (potential) current drawing
    cancelCurrentDrawing();

    // Open file selection dialog
    QString file_path = QFileDialog::getSaveFileName(
                this,
                "Export simulation result",
                QDir::homePath(),
                "JPG (*.jpg);;PNG (*.png);;TIFF (*.tiff)");

    // If the user cancelled the dialog
    if (file_path.isEmpty()) {
        return;
    }

    // Prepare an image with the double resolution of the scene
    QImage image(ui->graphicsView->sceneRect().size().toSize()*2, QImage::Format_ARGB32);

    // Fill background transparent only if PNG is selected as the destination format
    if (QFileInfo(file_path).suffix().toLower() == "png") {
        image.fill(Qt::transparent);
    }
    else {
        image.fill(Qt::white);
    }

    // Paint the scene into the image
    QPainter painter(&image);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    m_scene->render(&painter, QRectF(), ui->graphicsView->sceneRect());

    // Write the exported image into the file
    if (!image.save(file_path)) {
        QMessageBox::critical(this, "Error", "Unable to write into the selected file");
        return;
    }
}

void MainWindow::raysCheckboxToggled(bool) {
    // Update the simulation UI
    updateSimulationUI();

    // Update the rays threshold filtering
    filterRaysThreshold();
}

void MainWindow::raysThresholdChanged(int val) {
    // Set the width of the label to the size of the larger text (-200 dBm)
    if (ui->label_threshold_val->minimumWidth() == 0) {
        ui->label_threshold_val->setText("-200 dBm");
        ui->label_threshold_val->setFixedWidth(ui->label_threshold_val->sizeHint().width());
    }

    // Set the text of the label according to slider
    ui->label_threshold_val->setText(QString("%1 dBm").arg(val));

    // Update the rays threshold filtering
    filterRaysThreshold();
}

void MainWindow::resultTypeSelectionChanged(int, bool checked) {
    // Don't care about uncheched events
    if (!checked)
        return;

    // Update the view
    showReceiversResult();
}


void MainWindow::simulationStarted() {
    // Update the UI controls
    updateSimulationUI();
}

void MainWindow::simulationFinished() {
    // Enable the UI controls
    updateSimulationUI();

    if (SimulationHandler::simulationData()->simulationType() == SimType::PointReceiver) {
        // Add all computed rays to the scene
        foreach (RayPath *rp, m_simulation_handler->getRayPathsList()) {
            m_scene->addItem(rp);
        }
    }

    // Show the results
    showReceiversResult();
}

void MainWindow::simulationCancelled() {
    // Update the UI
    updateSimulationUI();

    // Reset the simulations
    simulationReset();
}

void MainWindow::simulationProgress(double p) {
    // Update the progress bar's value
    ui->progressbar_simulation->setValue(p * 100);
}


////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////// SIMULATION DATA HANDLING FUNCTIONS /////////////////////////////////


void MainWindow::simulationReset() {
    m_simulation_handler->resetComputedData();
    m_scene->hideDataLegend();
}

bool MainWindow::askSimulationReset() {
    int ans = QMessageBox::question(
                this,
                "Simulation reset",
                "Are you sure you want to reset the simulation results?\n"
                "The map will not be modified.");

    // If the user cancelled the action -> abort
    if (ans == QMessageBox::No)
        return false;

    // Reset the computation data
    simulationReset();

    return true;
}

void MainWindow::filterRaysThreshold() {
    // Convert the threshold in Watts
    const double threshold = SimulationData::convertPowerToWatts(ui->slider_threshold->value());

    // Loop over the RayPaths
    foreach (RayPath *rp, m_simulation_handler->getRayPathsList())
    {
        // Hide the RayPaths with a power lower than the threshold, or checkbox not checked, or
        // UI is not in simulation mode, or simulation type is area
        if (rp->computePower() > threshold &&
                ui->checkbox_rays->isChecked() &&
                m_ui_mode == UIMode::SimulationMode &&
                m_simulation_handler->simulationData()->simulationType() == SimType::PointReceiver)
        {
            rp->show();
        }
        else {
            rp->hide();
        }
    }
}

void MainWindow::showReceiversResult() {
    // Don't show the results if not finished
    if (m_simulation_handler->isRunning() || !m_simulation_handler->isDone())
        return;

    SimType::SimType sim_type = SimulationHandler::simulationData()->simulationType();

    // Switch over the simulation types
    switch (sim_type) {
    case SimType::PointReceiver: {
        showResultsRays();
        break;
    }
    case SimType::AreaReceiver: {
        showResultHeatMap();
        break;
    }
    }
}

void MainWindow::showResultsRays() {
    // Loop over every receiver and show its results
    foreach(Receiver *re , SimulationHandler::simulationData()->getReceiverList())
    {
        // Show the results of each receiver.
        // Note: type, min and max are not used for shaped receivers.
        re->showResults(ResultType::Power, 0, 0);
    }

    filterRaysThreshold();
}

void MainWindow::showResultHeatMap() {
    // No heat map if no simulation area item
    if (!m_sim_area_item)
        return;

    // Get the currently selected type
    ResultType::ResultType res_type = (ResultType::ResultType) m_result_radio_grp->checkedId();

    double min, max;
    m_sim_area_item->getReceivedDataBounds(res_type, &min, &max);

    // Loop over every receiver and show its results
    foreach(Receiver *re , m_sim_area_item->getReceiversList())
    {
        // Show the results of each receiver
        re->showResults(res_type, min, max);
    }

    m_scene->showDataLegend(res_type, min, max);
}
