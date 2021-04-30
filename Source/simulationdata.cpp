#include "simulationdata.h"

// The max amplitude for the data's color
#define PEAK_COLOR_LIGHT 255
#define PEAK_COLOR_DARK 240

// The default max number of reflections
#define MAX_REFLECTIONS_COUNT_DEFAULT 3


SimulationData::SimulationData() : QObject()
{
    m_reflections_count = MAX_REFLECTIONS_COUNT_DEFAULT;
    m_simulation_type = SimType::PointReceiver;
}

SimulationData::SimulationData(QList<Building*> b_l, QList<Emitter*> e_l, QList<Receiver*> r_l) : SimulationData()
{
    setInitData(b_l, e_l, r_l);
}

void SimulationData::setInitData(QList<Building*> b_l, QList<Emitter*> e_l, QList<Receiver*> r_l) {
    m_building_list = b_l;
    m_emitter_list = e_l;
    m_receiver_list = r_l;
}

// ---------------------------------------------------------------------------------------------- //

// +++++++++++++++++++++++++++++++++ DATA CONVERSION FUNCTIONS ++++++++++++++++++++++++++++++++++ //

/**
 * @brief Emitter::convertPowerToWatts
 * @param power_dbm
 * @return
 *
 * This function returns the converted power in watts from a dBm value
 * (formula from the specifications document of the project)
 */
double SimulationData::convertPowerToWatts(double power_dbm) {
    // Compute the power in Watts from the dBm
    return pow(10.0, power_dbm/10.0) / 1000.0;
}

/**
 * @brief Emitter::convertPowerToWatts
 * @param power_dbm
 * @return
 *
 * This function returns the converted power in dBm from a Watts value
 */
double SimulationData::convertPowerTodBm(double power_watts) {
    // Compute the power in dBm from the Watts
    return 10 * log10(power_watts / 0.001);
}

/**
 * @brief SimulationData::ratioToColor
 * @param ratio
 * @return
 *
 * This function converts a ratio [0,1] to a color from red to blue
 */
QRgb SimulationData::ratioToColor(qreal ratio, bool light) {
    int r, g, b;

    const int peak_color = (light ? PEAK_COLOR_LIGHT : PEAK_COLOR_DARK);

    // Ratio limited from 0 to 1
    ratio = max(0.0, min(1.0, ratio));

    if (ratio > 0.75) {         // Go from red to yellow
        r = peak_color;
        g = peak_color * (4 - ratio/0.25);
        b = 0;
    }
    else if (ratio > 0.5) {     // Go from yellow to green
        r = peak_color * (ratio/0.25 - 2);
        g = peak_color;
        b = 0;
    }
    else if (ratio > 0.25) {    // Go from green to turquoise
        r = 0;
        g = peak_color;
        b = peak_color * (2 - ratio/0.25);
    }
    else {                      // Go from turquoise to blue
        r = 0;
        g = peak_color * ratio/0.25;
        b = peak_color;
    }

    return qRgb(r, g, b);
}

// ---------------------------------------------------------------------------------------------- //

// +++++++++++++++++ BUILDINGS / EMITTERS / RECEIVER LISTS MANAGEMENT FUNCTIONS +++++++++++++++++ //

void SimulationData::attachBuilding(Building *b) {
    m_building_list.append(b);
}

void SimulationData::attachEmitter(Emitter *e) {
    m_emitter_list.append(e);
}

void SimulationData::attachReceiver(Receiver *r) {
    m_receiver_list.append(r);
}

void SimulationData::detachBuilding(Building *b) {
    m_building_list.removeAll(b);
}

void SimulationData::detachEmitter(Emitter *e) {
    m_emitter_list.removeAll(e);
}

void SimulationData::detachReceiver(Receiver *r) {
    m_receiver_list.removeAll(r);
}

void SimulationData::reset() {
    m_building_list.clear();
    m_emitter_list.clear();
    m_receiver_list.clear();
}

QList<Wall*> SimulationData::getBuildingWallsList() {
    QPainterPath buildings_path;

    // Unite all building rectangles into one path
    foreach (Building *b, m_building_list) {
        QPainterPath pth;
        pth.addRect(b->getRect());

        buildings_path = buildings_path.united(pth);
    }

    // Simplify the path
    buildings_path = buildings_path.simplified();

    // Get the independent polygons
    QList<QPolygonF> poly_list = buildings_path.toSubpathPolygons();

    // Divide all the polygons in walls
    QList<Wall*> wall_list;

    foreach(QPolygonF p, poly_list) {
        QList<QPointF> poly_points = p.toList();

        // Loop from 0 to Size - 2 since the last point is the same as the first one (closed path)
        for (int i = 0 ; i < poly_points.size() - 1 ; i++) {
            QLineF line(poly_points[i], poly_points[i+1]);
            Wall *w = new Wall(line);
            wall_list.append(w);
        }
    }

    return wall_list;
}

QList<Building*> SimulationData::getBuildingsList() {
    return m_building_list;
}

QList<Emitter*> SimulationData::getEmittersList() {
    return m_emitter_list;
}

QList<Receiver*> SimulationData::getReceiverList() {
    return m_receiver_list;
}

int SimulationData::maxReflectionsCount() {
    return m_reflections_count;
}

void SimulationData::setReflectionsCount(int cnt) {
    if (cnt < 0 || cnt > 99) {
        cnt = 3;
    }

    m_reflections_count = cnt;
}

SimType::SimType SimulationData::simulationType() {
    return m_simulation_type;
}

void SimulationData::setSimulationType(SimType::SimType t) {
    m_simulation_type = t;
}

// ---------------------------------------------------------------------------------------------- //

// +++++++++++++++++++++++++++ SIMULATION DATA FILE WRITING FUNCTIONS +++++++++++++++++++++++++++ //

QDataStream &operator>>(QDataStream &in, SimulationData *sd) {
    sd->reset();

    QList<Building*> buildings_list;
    QList<Emitter*> emitters_list;
    QList<Receiver*> receiver_list;
    int max_refl_count;
    int sim_type;

    in >> buildings_list;
    in >> emitters_list;
    in >> receiver_list;
    in >> max_refl_count;
    in >> sim_type;

    sd->setInitData(buildings_list, emitters_list, receiver_list);
    sd->setReflectionsCount(max_refl_count);
    sd->setSimulationType((SimType::SimType) sim_type);

    return in;
}

QDataStream &operator<<(QDataStream &out, SimulationData *sd) {
    out << sd->getBuildingsList();
    out << sd->getEmittersList();
    out << sd->getReceiverList();
    out << sd->maxReflectionsCount();
    out << (int) sd->simulationType();

    return out;
}

// ---------------------------------------------------------------------------------------------- //
