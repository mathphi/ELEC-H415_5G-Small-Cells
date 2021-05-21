#include "simulationdata.h"
#include "corner.h"

// The max amplitude for the data's color
#define PEAK_COLOR_LIGHT 255
#define PEAK_COLOR_DARK 240

// The default max number of reflections
#define MAX_REFLECTIONS_COUNT_DEFAULT 3

// Default relative permittivity
#define DEFAULT_REL_PERMITIVITY 5.0;

// Default height for the simulation plane
#define DEFAULT_SIM_HEIGHT  2.0;    // Meters

// Default minimum validity radius
#define DEFAULT_VALID_RADIUS 10.0       // Meters
#define DEFAULT_PRUNE_RADIUS INFINITY   // Meters

// Default simulation parameters
#define DEFAULT_SIM_BANDWIDTH       200 // MHz
#define DEFAULT_SIM_TEMPERATURE     20  // °C
#define DEFAULT_SIM_NOISE_FIGURE    10  // dB
#define DEFAULT_SIM_TARGET_SNR      2   // dB


SimulationData::SimulationData() : QObject()
{
    // Initialize to defaults settings
    resetDefaults();
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
 * @brief SimulationData::convertKelvinToCelsius
 * @param T_k
 * @return
 *
 * This function returns the temperature converted to Celsius scale
 */
double SimulationData::convertKelvinToCelsius(double T_k) {
    return T_k - 273.15;
}

/**
 * @brief SimulationData::convertCelsiusToKelvin
 * @param T_c
 * @return
 *
 * This function returns the temperature to absolute in Kelvin
 */
double SimulationData::convertCelsiusToKelvin(double T_c) {
    return T_c + 273.15;
}

/**
 * @brief SimulationData::delayToHumanReadable
 * @param delay
 * @param units
 * @return
 *
 * This function converts a delay in seconds to a human readable delay
 */
double SimulationData::delayToHumanReadable(double delay, QString *units, double *scale_factor) {
    double hr_delay;
    double factor;

    if (delay < 1e-9) {
        *units = "ps";
        factor = 1e-12;
        hr_delay = delay / factor;
    }
    else if (delay < 1e-6) {
        *units = "ns";
        factor = 1e-9;
        hr_delay = delay / factor;
    }
    else if (delay < 1e-3) {
        *units = "µs";
        factor = 1e-6;
        hr_delay = delay / factor;
    }
    else if (delay < 1e-0) {
        *units = "ms";
        factor = 1e-3;
        hr_delay = delay / factor;
    }
    else {
        *units = "s";
        factor = 1;
        hr_delay = delay;
    }

    // If a scale factor is defined
    if (scale_factor != nullptr) {
        *scale_factor = factor;
    }

    return hr_delay;
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
    ratio = qBound(0.0, ratio, 1.0);

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

void SimulationData::resetDefaults() {
    m_simulation_type = SimType::PointReceiver;

    m_reflections_count = MAX_REFLECTIONS_COUNT_DEFAULT;
    m_nlos_refl_en      = false;

    m_rel_permitivity   = DEFAULT_REL_PERMITIVITY;
    m_simulation_height = DEFAULT_SIM_HEIGHT;
    m_sim_bandwidth     = DEFAULT_SIM_BANDWIDTH * 1e6;
    m_sim_temperature   = convertCelsiusToKelvin(DEFAULT_SIM_TEMPERATURE);
    m_sim_noise_figure  = DEFAULT_SIM_NOISE_FIGURE;
    m_sim_target_SNR    = DEFAULT_SIM_TARGET_SNR;
    m_min_valid_radius  = DEFAULT_VALID_RADIUS;
    m_pruning_radius    = DEFAULT_PRUNE_RADIUS;
}

QList<Wall*> SimulationData::makeBuildingWallsFiltered(const QRectF boundary_rect) const {
    // Split the bounding rect into 4 lines
    QLineF left_l(boundary_rect.bottomLeft(), boundary_rect.topLeft());
    QLineF top_l(boundary_rect.topLeft(), boundary_rect.topRight());
    QLineF right_l(boundary_rect.topRight(), boundary_rect.bottomRight());
    QLineF bottom_l(boundary_rect.bottomRight(), boundary_rect.bottomLeft());

    // Filtered walls
    QList<Wall*> walls_flt;

    // Check which wall is containted in a boundary
    foreach(Wall *w, makeBuildingWallsList()) {
        QLineF w_line = w->getLine();

        // Check if wall is horizontal/vertical and on the same y/x as the boundaries
        if (!(w_line.y1() == top_l.y1()    && w_line.y1() == w_line.y2()) &&
            !(w_line.y1() == bottom_l.y1() && w_line.y1() == w_line.y2()) &&
            !(w_line.x1() == left_l.x1()   && w_line.x1() == w_line.x2()) &&
            !(w_line.x1() == right_l.x1()  && w_line.x1() == w_line.x2()))
        {
            // Keep the wall if it is not in the boundaries
            walls_flt.append(w);
        }
        else {
            // Delete this wall if it is not used
            delete w;
        }
    }

    return walls_flt;
}

QList<Wall*> SimulationData::makeBuildingWallsList() const {
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

QList<Corner*> SimulationData::makeWallsCorners(QList<Wall*> walls_list) const {
    // Initialize the corners list
    QList<Corner*> corner_list;
    QLineF l_w1, l_w2;

    // Check the start/end points for each pair of wall
    for (int i = 0 ; i < walls_list.size() ; i++) {
        for (int j = i+1 ; j < walls_list.size() ; j++) {
            Wall *w1 = walls_list.at(i);
            Wall *w2 = walls_list.at(j);

            // Get the line of each wall
            l_w1 = w1->getLine();
            l_w2 = w2->getLine();

            // Check if one end matches for both walls
            if (l_w1.p1() == l_w2.p1()) {
                // In this case, there is a corner at l_w1.p1()
                Corner *corner = new Corner(l_w1.p1(), l_w1.p2(), l_w2.p2(), w1, w2);
                corner_list.append(corner);
            }
            else if (l_w1.p1() == l_w2.p2()) {
                // In this case, there is a corner at l_w1.p1()
                Corner *corner = new Corner(l_w1.p1(), l_w1.p2(), l_w2.p1(), w1, w2);
                corner_list.append(corner);
            }
            else if (l_w1.p2() == l_w2.p1()) {
                // In this case, there is a corner at l_w1.p2()
                Corner *corner = new Corner(l_w1.p2(), l_w1.p1(), l_w2.p2(), w1, w2);
                corner_list.append(corner);
            }
            else if (l_w1.p2() == l_w2.p2()) {
                // In this case, there is a corner at l_w1.p2()
                Corner *corner = new Corner(l_w1.p2(), l_w1.p1(), l_w2.p1(), w1, w2);
                corner_list.append(corner);
            }
        }
    }

    return corner_list;
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

SimType::SimType SimulationData::simulationType() {
    return m_simulation_type;
}
void SimulationData::setSimulationType(SimType::SimType t) {
    m_simulation_type = t;
}

int SimulationData::maxReflectionsCount() const {
    return m_reflections_count;
}
void SimulationData::setReflectionsCount(int cnt) {
    if (cnt < 0 || cnt > 99) {
        cnt = 3;
    }

    m_reflections_count = cnt;
}

bool SimulationData::reflectionEnabledNLOS() const {
    return m_nlos_refl_en;
}
void SimulationData::setReflectionEnabledNLOS(bool enabled) {
    m_nlos_refl_en = enabled;
}

double SimulationData::getRelPermitivity() const {
    return m_rel_permitivity;
}
void SimulationData::setRelPermitivity(double perm) {
    m_rel_permitivity = perm;
}

double SimulationData::getSimulationHeight() const {
    return m_simulation_height;
}
void SimulationData::setSimulationHeight(double height) {
    m_simulation_height = height;
}

void SimulationData::setSimulationBandwidth(double bw) {
    m_sim_bandwidth = bw;
}
double SimulationData::getSimulationBandwidth() const {
    return m_sim_bandwidth;
}

void SimulationData::setSimulationTemperature(double temp) {
    m_sim_temperature = temp;
}
double SimulationData::getSimulationTemperature() const {
    return m_sim_temperature;
}

void SimulationData::setSimulationNoiseFigure(double nf) {
    m_sim_noise_figure = nf;
}
double SimulationData::getSimulationNoiseFigure() const {
    return m_sim_noise_figure;
}

void SimulationData::setSimulationTargetSNR(double snr) {
    m_sim_target_SNR = snr;
}
double SimulationData::getSimulationTargetSNR() const {
    return m_sim_target_SNR;
}

void SimulationData::setMinimumValidRadius(double radius) {
    m_min_valid_radius = radius;
}
double SimulationData::getMinimumValidRadius() const {
    return m_min_valid_radius;
}

void SimulationData::setPruningRadius(double radius) {
    m_pruning_radius = radius;
}
double SimulationData::getPruningRadius() const {
    return m_pruning_radius;
}

// ---------------------------------------------------------------------------------------------- //

// +++++++++++++++++++++++++++ SIMULATION DATA FILE WRITING FUNCTIONS +++++++++++++++++++++++++++ //

QDataStream &operator>>(QDataStream &in, SimulationData *sd) {
    sd->reset();

    // Simulation settings
    in >> sd->m_simulation_type;
    in >> sd->m_reflections_count;
    in >> sd->m_nlos_refl_en;

    // Simulation parameters
    in >> sd->m_rel_permitivity;
    in >> sd->m_simulation_height;
    in >> sd->m_sim_bandwidth;
    in >> sd->m_sim_temperature;
    in >> sd->m_sim_noise_figure;
    in >> sd->m_sim_target_SNR;
    in >> sd->m_min_valid_radius;
    in >> sd->m_pruning_radius;

    // Get buildings lists
    in >> sd->m_building_list;
    in >> sd->m_emitter_list;
    in >> sd->m_receiver_list;

    return in;
}

QDataStream &operator<<(QDataStream &out, SimulationData *sd) {
    // Simulation settings
    out << sd->m_simulation_type;
    out << sd->m_reflections_count;
    out << sd->m_nlos_refl_en;

    // Simulation parameters
    out << sd->m_rel_permitivity;
    out << sd->m_simulation_height;
    out << sd->m_sim_bandwidth;
    out << sd->m_sim_temperature;
    out << sd->m_sim_noise_figure;
    out << sd->m_sim_target_SNR;
    out << sd->m_min_valid_radius;
    out << sd->m_pruning_radius;

    // Get buildings lists
    out << sd->m_building_list;
    out << sd->m_emitter_list;
    out << sd->m_receiver_list;

    return out;
}

// ---------------------------------------------------------------------------------------------- //
