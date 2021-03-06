#include "simulationhandler.h"
#include "computationunit.h"
#include "constants.h"
#include "corner.h"

#include <QDebug>

#define AREA_PER_THREAD 100


// Global variable for static access to simulation data
SimulationData *g_simulation_data = new SimulationData();


SimulationHandler::SimulationHandler()
{
    m_sim_started = false;
    m_sim_cancelling = false;
    m_sim_done = false;
    m_init_cu_count = 0;
}

/**
 * @brief SimulationHandler::simulationData
 * @return
 *
 * This function returns a pointer to the simulation data global object
 */
SimulationData *SimulationHandler::simulationData() {
    return g_simulation_data;
}

/**
 * @brief SimulationHandler::getRayPathsList
 * @return
 *
 * This function returns all the computed ray paths in the scene
 */
QList<RayPath*> SimulationHandler::getRayPathsList() const {
    QList<RayPath*> ray_paths;

    // Append the ray paths of each receivers to the ray paths list to return
    foreach(Receiver *re, m_receivers_list) {
        ray_paths.append(re->getRayPaths());
    }

    return ray_paths;
}

/**
 * @brief SimulationHandler::isDone
 * @return
 *
 * This function returns true if a simulation result has been computed
 */
bool SimulationHandler::isDone() const {
    return m_sim_done;
}

/**
 * @brief SimulationHandler::isRunning
 * @return
 *
 * This function returns true if a simulation computation is running
 */
bool SimulationHandler::isRunning() const {
    return m_sim_started;
}

/**
 * @brief SimulationHandler::isCancelling
 * @return
 *
 * This function returns true if the simulation is cancelling
 */
bool SimulationHandler::isCancelling() const {
    return m_sim_cancelling;
}


/**************************************************************************************************/
// --------------------------------- COMPUTATION FUNCTIONS -------------------------------------- //
/**************************************************************************************************/

/**
 * @brief SimulationHandler::mirror
 *
 * This function returns the coordinates of the image of the 'source' point
 * after an axial symmetry through the wall.
 *
 * WARNING: the y axis is upside down in the graphics scene !
 *
 * @param source : The position of the source whose image is calculated
 * @param wall   : The wall over which compute the image
 * @return       : The coordinates of the image
 */
QPointF SimulationHandler::mirror(const QPointF source, Wall *wall) {
    // Get the angle of the wall to the horizontal axis
    const double theta = wall->getRealLine().angle() * M_PI / 180.0 - M_PI / 2.0;

    // Translate the origin of the coordinates system on the base of the wall
    const double x = source.x() - wall->getRealLine().p1().x();
    const double y = source.y() - wall->getRealLine().p1().y();

    // We use a rotation matrix :
    //   x' =  x*cos(??) - y*sin(??)
    //   y' = -x*sin(??) - y*cos(??)
    // to set the y' axis along the wall, so the image is at the opposite of the x' coordinate
    // in the new coordinates system.
    const double x_p =  x*cos(theta) - y*sin(theta);
    const double y_p = -x*sin(theta) - y*cos(theta);

    // Rotate back to the original coordinates system
    const QPointF rel_pos = QPointF(-x_p*cos(theta) - y_p*sin(theta), x_p*sin(theta) - y_p*cos(theta));

    // Translate back to the original coordinates system
    return rel_pos + wall->getRealLine().p1();
}


/**
 * @brief SimulationHandler::checkIntersections
 *
 * This function checks if the ray intersects a wall
 *
 * @param ray         : The ray to check for clearance
 * @param origin_wall : The wall from which this ray come from (reflection), or nullptr
 * @param target_wall : The wall to which this ray go to (reflection), or nullptr
 * @return            : True if 'ray' intersects a wall in the map
 */
bool SimulationHandler::checkIntersections(
        QLineF ray,
        Wall *origin_wall,
        Wall *target_wall)
{
    // Loop over all walls of the scene and look for intersection with ray
    foreach(Wall *w, m_wall_list) {
        // Don't care about the origin or target wall (where this ray is reflected)
        if (w == origin_wall || w == target_wall) {
            continue;
        }

        // Get the intersection point
        QLineF::IntersectionType i_t = ray.intersects(w->getRealLine(), nullptr);

        // There is obscursion if the intersection with the ray is
        // on the wall (not on its extension)
        if (i_t == QLineF::BoundedIntersection) {
            return true;
        }
    }

    return false;
}

/**
 * @brief SimulationHandler::reflectionCoefficient
 *
 * This function computes the reflection coefficient for the reflection of
 * an incident ray on a wall.
 * The coefficient returned is 3-dimensionnal:
 *  - the two first components are the same and are the coefficient
 *    for a parallel polarization
 *  - the last component is the coefficient for the othogonal polarization
 *
 * @param w      : The reflection wall
 * @param ray_in : The incident ray
 * @return       : The reflection coefficient for this reflection
 */
vector<complex> SimulationHandler::reflectionCoefficient(Wall *w, QLineF in_ray) {
    // Get the properties of the simulation
    const double e_r = simulationData()->getRelPermitivity();

    // Compute the incident angles
    const double theta_i = w->getNormalAngleTo(in_ray);

    // Compute the reflection coefficient for an orthogonal polarization (equation 3.4)
    const complex Gamma_orth = (cos(theta_i) - sqrt(e_r) * sqrt(1 - 1/e_r * pow(sin(theta_i), 2))) /
                               (cos(theta_i) + sqrt(e_r) * sqrt(1 - 1/e_r * pow(sin(theta_i), 2)));

    // Compute the reflection coefficient for a parallel polarization (equation 3.26)
    const complex Gamma_para = (cos(theta_i) - 1/sqrt(e_r) * sqrt(1 - 1/e_r * pow(sin(theta_i), 2))) /
                               (cos(theta_i) + 1/sqrt(e_r) * sqrt(1 - 1/e_r * pow(sin(theta_i), 2)));

    // Return as a 3-D vector
    return {
        Gamma_para,
        Gamma_para,
        Gamma_orth
    };
}


/**
 * @brief SimulationHandler::computeNominalElecField
 *
 * This function computes the "Nominal" electric field (equation 3.52).
 * So this is the electric field as if there were no reflection or transmission.
 * The electric field is returned in a 3-dimentionnal vector.
 *
 * @param em    : The emitter (source of this ray)
 * @param e_ray : The ray coming out from the emitter
 * @param r_ray : The ray coming to the receiver
 * @param dn    : The total length of the ray path
 * @param theta : [Optional] The vertical angle at receiver side
 * @return      : The "Nominal" electric field
 */
vector<complex> SimulationHandler::computeNominalElecField(
        Emitter *em,
        QLineF e_ray,
        QLineF r_ray,
        double dn,
        double theta)
{
    // Incidence angle of the ray from the emitter
    double phi = em->getIncidentRayAngle(e_ray);

    // Get the polarization vector of the emitter
    vector<complex> polarization = em->getPolarization();

    // Get properties from the emitter
    double GTX = em->getGain(theta, phi);
    double PTX = em->getPower();
    double omega = em->getFrequency()*2*M_PI;

    // Compute the direction of the parallel component of the electric field at the receiver.
    // This direction is a unit vector normal to the propagation vector in the incidence plane.
    QLineF E_unit = r_ray.normalVector().unitVector();

    // Propagation constant (air)
    double beta = omega/LIGHT_SPEED;

    // Direct (nominal) electric field (equation 8.77)
    complex E = sqrt(60.0*GTX*PTX) * exp(-1i*beta*dn) / dn;

    // The first component of the polarization vector is the parallel component, the second
    // is the orthogonal one.
    // E_unit is the direction vector of the electric field in the incidence plane.
    return {
        E * polarization[0] * E_unit.dx(),
        E * polarization[0] * E_unit.dy(),
        E * polarization[1]
    };
}

/**
 * @brief SimulationHandler::computeRayPath
 *
 * This function computes the ray path for a combination reflections.
 *
 * @param emitter  : The emitter for this ray path
 * @param receiver : The receiver for this ray path
 * @param images   : The list of reflection images computed for this ray path
 * @param walls    : The list of walls that form a combination of reflections
 * @return         : A pointer to the new RayPath object computed (or nullptr if invalid)
 */
RayPath *SimulationHandler::computeRayPath(
        Emitter *emitter,
        Receiver *receiver,
        QList<QPointF> images,
        QList<Wall*> walls)
{
    // We run backward in this function (from receiver to emitter)

    // The first target point is the receiver
    QPointF target_point = receiver->getRealPos();

    // This list will contain the lines forming the ray path
    QList<QLineF> rays;

    // This coefficient will contain the product of all reflection
    // coefficients for this ray path
    vector<complex> coeff = {1,1,1};

    // Total length of the ray path
    double dn = 0;

    // Wall of the next reflection (towards the receiver)
    Wall *target_wall = nullptr;

    // Loop over the images (backward)
    for (int i = images.size()-1; i >= 0 ; i--) {
        Wall *reflect_wall = walls[i];
        QPointF src_image = images[i];

        // Compute the virtual ray (line from the image to te target point)
        QLineF virtual_ray (src_image, target_point);

        // Get the reflection point (intersection of the virtual ray and the wall)
        QPointF reflection_pt;
        QLineF::IntersectionType i_t =
                virtual_ray.intersects(reflect_wall->getRealLine(), &reflection_pt);

        // The ray path is valid if the reflection is on the wall (not on its extension)
        if (i_t != QLineF::BoundedIntersection) {
            return nullptr; // Return an invalid ray path
        }

        // If the target point is the same as the reflection point
        //  -> not a physics situation -> invalid raypath
        if (reflection_pt == target_point) {
            return nullptr;
        }

        // Create a ray line between these two points
        QLineF ray(reflection_pt, target_point);

        // If this ray intersects a wall -> neglected
        if (checkIntersections(ray, reflect_wall, target_wall)) {
            return nullptr;
        }

        // If this is the virtual ray from the last image to the receiver,
        // it length is the total ray path length dn.
        if (src_image == images.last()) {
            dn = virtual_ray.length();
        }

        // Compute the reflection coefficient for this reflection
        // The multiplication is made component by component (not a cross product).
        coeff *= reflectionCoefficient(reflect_wall, ray);

        // Add this ray line to the list of lines forming the ray path
        rays.append(ray);

        // The next target point is the current reflection point
        target_point = reflection_pt;
        target_wall = reflect_wall;
    }

    // If the target point is the same as the emitter point
    //  -> not a physics situation -> invalid raypath
    if (emitter->getRealPos() == target_point) {
        return nullptr;
    }

    // The last ray line is from the emitter to the target point
    QLineF ray(emitter->getRealPos(), target_point);

    // If this ray intersects a wall -> neglected
    if (checkIntersections(ray, nullptr, target_wall)) {
        return nullptr;
    }

    // Add this last ray to the rays list
    rays.append(ray);

    // If there were no images in the list, we are computing the direct ray, so the length
    // of the ray path (dn) is the length of the ray line from emitter to receiver.
    if (images.size() == 0) {
        dn = ray.length();
    }

    // Compute the electric field for this ray path (equation 8.78)
    // rays.last() is the ray coming out from the emitter
    // rays.first() is the ray coming to the receiver
    // The multiplication is made component by component (not a cross product).
    vector<complex> En = coeff * computeNominalElecField(emitter, rays.last(), rays.first(), dn);

    // Return a new RayPath object
    RayPath *rp = new RayPath(emitter, receiver, rays, En, dn);
    return rp;
}

/**
 * @brief SimulationHandler::recursiveReflection
 *
 * This function gets the image of the source (emitter of previous image) over the 'reflect_wall'
 * and compute the ray path recursively.
 * The computed ray paths are added to the RayPaths list of the receiver
 *
 * @param emitter      : The emitter for this ray path
 * @param receiver     : The receiver for this ray path
 * @param reflect_wall : The wall on which we will compute the reflection
 * @param images       : The list of images for the previous reflections
 * @param walls        : The list of walls for the previous reflections
 * @param level        : The recursion level
 */
void SimulationHandler::recursiveReflection(
        Emitter *emitter,
        Receiver *receiver,
        Wall *reflect_wall,
        QList<QPointF> images,
        QList<Wall *> walls,
        int level)
{
    // Position of the image of the source
    QPointF src_image;

    if (images.size() == 0) {
        // If there is no previous reflection, the source is the emitter
        src_image = mirror(emitter->getRealPos(), reflect_wall);
    }
    else {
        // If there are previous reflections, the source is the last image
        src_image = mirror(images.last(), reflect_wall);
    }

    // Keep the list of walls and images for the next recursions
    images.append(src_image);
    walls.append(reflect_wall);

    // Compute the complete ray path for this set of reflections
    RayPath *rp = computeRayPath(emitter, receiver, images, walls);

    // Add this ray path to his receiver
    receiver->addRayPath(rp);

    // If the level of recursion is under the max number of reflections
    if (level < simulationData()->maxReflectionsCount())
    {
        // Compute the reflection from the 'reflect_wall' to all other walls of the scene
        foreach (Wall *w, m_wall_list)
        {
            // We don't have to compute any reflection from the 'reflect_wall' to itself
            if (w == reflect_wall) {
                continue;
            }

            // Recursive call for each wall of the scene (and increase the recusion level)
            recursiveReflection(emitter, receiver, w, images, walls, level+1);
        }
    }
}


/**
 * @brief SimulationHandler::computeDiffractedRay
 * @param e
 * @param r
 * @param c
 *
 * This function computes the diffracted ray from an emitter to a receiver via the corner c.
 */
void SimulationHandler::computeDiffractedRay(Emitter *e, Receiver *r, Corner *c) {
    // If the target point is the same as the emitter point
    //  -> not a physics situation -> invalid raypath
    if (e->getRealPos() == r->getRealPos()) {
        return;
    }

    // Create the rays from emitter/receiver to the corner
    QLineF ce_ray(c->getRealPos(), e->getRealPos());
    QLineF cr_ray(c->getRealPos(), r->getRealPos());

    // Create measurement lines from adjacent walls ends to the emitter
    QLineF measur_line1(e->getRealPos(), c->getRealEndPoints().at(0));
    QLineF measur_line2(e->getRealPos(), c->getRealEndPoints().at(1));

    QLineF em_adj;   // Wall adjacent to the emitter's ray
    QLineF rv_adj;   // Wall adjacent to the receiver's ray

    // Compare the length of both measurement lines to figure out which wall is
    // adjacent to the ray coming from the emitter.
    if (measur_line1.length() < measur_line2.length()) {
        // Wall 1 is adjacent to the emitter's ray
        em_adj = c->getAdjecentRealLines()[0];
        rv_adj = c->getAdjecentRealLines()[1];
    }
    else {
        // Wall 2 is adjacent to the emitter's ray
        em_adj = c->getAdjecentRealLines()[1];
        rv_adj = c->getAdjecentRealLines()[0];
    }

    // Check if both angles of the rays with their adjacent wall is < 90?? -> valid diffraction
    qreal em_angle = fabs(em_adj.angle() - ce_ray.angle());
    qreal rv_angle = fabs(rv_adj.angle() - cr_ray.angle());

    // Normalize the computed angles to [0,180??]
    if (em_angle > 180) {
        em_angle = 360 - em_angle;
    }
    if (rv_angle > 180) {
        rv_angle = 360 - rv_angle;
    }

    // Check if one of this angle is > 90?? and the sum of both > 90?? -> not a valid diffraction
    if (em_angle > 90 || rv_angle > 90 || em_angle+rv_angle > 90) {
        return;
    }

    // Check if the ray from the emitter/receiver to the corner intersects a wall
    if (checkIntersections(ce_ray, c->getAdjecentWalls().at(0), c->getAdjecentWalls().at(1)) ||
        checkIntersections(cr_ray, c->getAdjecentWalls().at(0), c->getAdjecentWalls().at(1)))
    {
        // If the diffracted ray intersects a wall -> ignore it
        return;
    }

    // Now we have a valid diffraction for this assembly of emitter/receiver/corner.
    // We can compute the diffraction coefficient using the Kniffe-edge model.

    // Get the LOS line
    QLineF los_ray(e->getRealPos(), r->getRealPos());

    // Compute diffraction parameters
    double omega    = e->getFrequency()*2*M_PI;
    double beta     = omega/LIGHT_SPEED;
    double dn       = ce_ray.length() + cr_ray.length();
    double Delta_r  = dn - los_ray.length();

    // Coefficients vector
    vector<complex> coeff = {1, 1, 1};

    // Negative Delta_r = numerical instabilities when (s1+s2) = d_los -> treated as a LOS ray
    if (Delta_r > 0) {
        // Fresnel parameter (equation 3.57)
        double nu = sqrt(2/M_PI * beta * Delta_r);

        // Compute the diffraction coefficient F(??) (equations 3.58, 3.59)
        double F_nu2_mod_dB = -6.9 - 20.0*log10(sqrt(pow((nu - 0.1), 2.0) + 1.0) + nu - 0.1);
        double F_nu_mod = sqrt(pow(10.0, F_nu2_mod_dB/10.0));
        double F_nu_arg = -M_PI_4 - M_PI_2 * pow(nu, 2.0);

        // Complex diffraction coefficient F(??)
        complex F_nu = F_nu_mod * exp(1i*F_nu_arg);
        coeff = {F_nu, F_nu, F_nu};

        vector<complex> En = coeff * computeNominalElecField(e, ce_ray, cr_ray, dn);
    }

    // Compute the electric field in the 3 components
    vector<complex> En = coeff * computeNominalElecField(e, ce_ray, cr_ray, dn);

    // Create a list of the rays
    QList<QLineF> rays = QList<QLineF>() << ce_ray << cr_ray;

    // Add the new RayPath object to the receiver
    RayPath *rp = new RayPath(e, r, rays, En, dn);
    r->addRayPath(rp);
}

/**
 * @brief SimulationHandler::computeGroundReflection
 * @param e
 * @param r
 *
 * This function computes the ray reflected off the ground.
 * This computation assumes the emitter and the receiver are both at the same height.
 * It is also assumed that this ray is computed only when LOS is free (no obstacle).
 */
void SimulationHandler::computeGroundReflection(Emitter *e, Receiver *r) {
    // Get the LOS ray between emitter and receiver
    QLineF los_ray(e->getRealPos(), r->getRealPos());

    // Get the middle of the LOS
    double mid_los = los_ray.length() / 2.0;

    // Get the simulation height
    double sim_h = simulationData()->getSimulationHeight();

    // Compute the reflected ray
    double dn = 2.0 * sqrt(pow(mid_los, 2.0) + pow(sim_h, 2.0));

    // Get incident angle to the emitter/receiver and reflection angle
    double theta_er = M_PI_2 + atan(sim_h / mid_los);
    double theta_i  = M_PI - theta_er;

    // Relative perittivity
    double e_r = simulationData()->getRelPermitivity();

    // Compute the reflection coefficient for an orthogonal polarization (equation 3.4)
    const complex Gamma_orth = (cos(theta_i) - sqrt(e_r) * sqrt(1 - 1/e_r * pow(sin(theta_i), 2))) /
                               (cos(theta_i) + sqrt(e_r) * sqrt(1 - 1/e_r * pow(sin(theta_i), 2)));

    // Compute the reflection coefficient for a parallel polarization (equation 3.26)
    const complex Gamma_para = (cos(theta_i) - 1/sqrt(e_r) * sqrt(1 - 1/e_r * pow(sin(theta_i), 2))) /
                               (cos(theta_i) + 1/sqrt(e_r) * sqrt(1 - 1/e_r * pow(sin(theta_i), 2)));

    // Return as a 3-D vector
    vector<complex> refl_coef {
        Gamma_orth,
        Gamma_orth,
        Gamma_para
    };

    // The multiplication is made component by component (not a cross product).
    vector<complex> En = refl_coef * computeNominalElecField(e, los_ray, los_ray, dn, theta_er);

    // Rays list
    QList<QLineF> rays = QList<QLineF>() << los_ray;

    // Add the new RayPath object to the receiver
    RayPath *rp = new RayPath(e, r, rays, En, dn, theta_er, true);
    r->addRayPath(rp);
}


/**************************************************************************************************/
// ---------------------------- SIMULATION MANAGEMENT FUNCTIONS --------------------------------- //
/**************************************************************************************************/

/**
 * @brief SimulationHandler::computeAllRays
 *
 * This function computes the rays from every emitters to every receivers.
 * This is an asynchronous function that adds computation units to the thread pool.
 */
void SimulationHandler::computeAllRays() {
    // Start the time counter
    m_computation_timer.start();

    // Loop over all receivers
    for (int i = 0 ; i < m_receivers_list.size() ; ) {
        QList<Receiver*> rcv_sublist;

        // Create a sublist of max. AREA_PER_THREAD receivers
        for (int j = 0 ; j < AREA_PER_THREAD && i < m_receivers_list.size() ; j++, i++) {
            rcv_sublist.append(m_receivers_list.at(i));
        }

        // Create a threaded computation unit to compute the rays to this list if receivers
        receiverRaysThreaded(rcv_sublist);
    }
}

/**
 * @brief SimulationHandler::computeReceiverRays
 * @param r
 *
 * This function computes all the rays arriving at the receiver r
 */
void SimulationHandler::computeReceiverRays(Receiver *r) {
    // No need to compute more if this receiver is already out of model
    if (r->outOfModel())
        return;

    // Loop over the emitters
    foreach(Emitter *e, m_emitters_list)
    {        
        // Compute the straight line distance to the base station
        double bs_dist = QLineF(e->getRealPos(), r->getRealPos()).length();

        // Set this receiver as Out of Model
        if (bs_dist < simulationData()->getMinimumValidRadius()) {
            r->setOutOfModel(true, e);
            break;  // No need to compute it for other emitters
        }

        // Ignore this receiver if the distance to the base station is lower than pruning threshold
        if (bs_dist > simulationData()->getPruningRadius()) {
            continue;
        }

        // Compute the direct ray path
        RayPath *LOS = computeRayPath(e, r);

        // Add it to his receiver
        r->addRayPath(LOS);

        // Compute reflection off the ground only if LOS (else it will cross a wall)
        if (LOS != nullptr && simulationData()->maxReflectionsCount() > 0) {
            computeGroundReflection(e, r);
        }

        // Compute reflections only if LOS (or if NLOS reflection forced by settings)
        if (LOS != nullptr || simulationData()->reflectionEnabledNLOS())
        {
            // For each wall in the scene, compute the reflections recursively
            foreach(Wall *w, m_wall_list)
            {
                // Don't compute any reflection if not needed
                if (simulationData()->maxReflectionsCount() > 0) {
                    // Compute the ray paths recursively (in a thread)
                    recursiveReflection(e, r, w);
                }
            }
        }

        // Compute diffraction only if no LOS
        if (LOS == nullptr) {
            // For each corner of the scene
            foreach(Corner *c, m_corners_list) {
                computeDiffractedRay(e, r, c);
            }
        }
    }
}

/**
 * @brief SimulationHandler::receiverRaysThreaded
 * @param r
 *
 * This function creates a computation unit to compute the rays
 * to the receiver r in a thread.
 */
void SimulationHandler::receiverRaysThreaded(QList<Receiver*> r_lst) {
    // Create a computation unit for the recursive computation of the reflections
    ComputationUnit *cu = new ComputationUnit(this, r_lst);

    // Connect the computation unit to the simulation handler
    connect(cu, SIGNAL(computationFinished()), this, SLOT(computationUnitFinished()));

    // Lock this section
    m_mutex.lock();

    // Add this CU to the list
    m_computation_units.append(cu);

    // Add this computation unit to the queue of the thread pool
    m_threadpool.start(cu);

    // Unlock this section
    m_mutex.unlock();

    // Increase the computation units counter
    m_init_cu_count++;
}

/**
 * @brief SimulationHandler::computationUnitFinished
 *
 * This slot is called when a computation unit finished to compute
 */
void SimulationHandler::computationUnitFinished() {
    // Get the computation unit (origin of the signal)
    ComputationUnit *cu = qobject_cast<ComputationUnit*> (sender());

    // Lock this whole slot
    m_mutex.lock();

    // If a computation unit is the origin of the call to this function
    if (cu != nullptr) {
        // One thread can write in this list at a time (mutex)
        m_computation_units.removeAll(cu);

        // Delete this computation unit
        delete cu;
    }

    // Compute the progression and send the progression signal
    double progress = 1.0 - (double) m_computation_units.size() / (double) m_init_cu_count;
    emit simulationProgress(progress);

    // All computations done
    if (m_computation_units.size() == 0) {
        // Mark the simulation as stopped
        m_sim_started = false;

        if (!m_sim_cancelling) {
            qDebug() << "Time (ms):" << m_computation_timer.nsecsElapsed() / 1e6;
            qDebug() << "Count:" << getRayPathsList().size();
            qDebug() << "Receivers:" << m_receivers_list.size();
            qDebug() << "Walls:" << m_wall_list.size();
            qDebug() << "Corners:" << m_corners_list.size();

            // Set simulation done flag
            m_sim_done = true;

            // Emit the simulation finished signal
            emit simulationFinished();
        }
        else {
            // If this is the lastest cancelling thread
            if (m_threadpool.activeThreadCount() <= 1) {
                // Reset the cancelling flag
                m_sim_cancelling = false;

                // Emit the simulation cancelled signal
                emit simulationCancelled();
            }
        }
    }

    // Unlock
    m_mutex.unlock();
}

/**
 * @brief SimulationHandler::startSimulationComputation
 * @param rcv_list
 * @param sim_area
 *
 * This function starts a computation of all rays to a list of receivers.
 * The simulation area is bounded by the sim_area rectangle.
 */
void SimulationHandler::startSimulationComputation(
        QList<Receiver*> rcv_list,
        QRectF sim_area,
        bool reset,
        QList<Emitter*> emit_list)
{
    // Don't start a new computation if already running
    if (isRunning())
        return;

    // The reset flag is set by default
    if (reset) {
        // Reset the previously computed data (if one)
        resetComputedData();
    }

    // Reset simulation done flag
    m_sim_done = false;

    // Setup the receivers list
    m_receivers_list = rcv_list;

    // Setup the emitters list
    if (emit_list.isEmpty()) {
        m_emitters_list = simulationData()->getEmittersList();
    }
    else {
        m_emitters_list = emit_list;
    }

    // Setup the simulation area
    m_sim_area = sim_area;


    qDebug() << m_sim_area;

    // Create the walls list from the buildings list
    m_wall_list = simulationData()->makeBuildingWallsFiltered(m_sim_area);

    // Create the corners list from the walls list
    m_corners_list = simulationData()->makeWallsCorners(m_wall_list);

    // Mark the simulation as running
    m_sim_started = true;

    // Reset the counter of computation units
    m_init_cu_count = 0;

    // Emit the simulation started signal
    emit simulationStarted();
    emit simulationProgress(0);

    // Check if there are receivers to compute
    if (rcv_list.size() > 0) {
        // Compute all rays
        computeAllRays();
    }
    else {
        // If there is no receiver -> send the finished signal
        computationUnitFinished();
    }
}

/**
 * @brief SimulationHandler::stopSimulationComputation
 *
 * This function cancel the current simulation.
 * The cancel operation must wait for all threads to finish.
 */
void SimulationHandler::stopSimulationComputation() {
    // Lock this whole function to ensure thread management integrity
    m_mutex.lock();

    // Clear the queue of the thread pool
    m_threadpool.clear();

    // Mark the simulation as cancelling
    m_sim_cancelling = true;

    // Delete all CU that are not (yet) running
    foreach(ComputationUnit *cu, m_computation_units) {
        if (!cu->isRunning()) {
            // Remove the CU from the list
            m_computation_units.removeAll(cu);

            // Delete this CU
            delete cu;
        }
    }

    // Lock access to the list
    m_mutex.unlock();
}

/**
 * @brief SimulationHandler::resetComputedData
 *
 * This function erases the computation results and computed RayPaths
 */
void SimulationHandler::resetComputedData() {
    // Reset simulation done flag
    m_sim_done = false;

    // Reset each receiver
    foreach(Receiver *r, m_receivers_list) {
        r->reset();
    }

    // Clear the receivers list
    m_receivers_list.clear();

    // Delete all walls (created from buildings list)
    foreach(Wall *w, m_wall_list) {
        delete w;
    }

    // Clear the walls list
    m_wall_list.clear();

    // Delete all corners (created from walls list)
    foreach(Corner *c, m_corners_list) {
        delete c;
    }

    // Clear the corners list
    m_corners_list.clear();
}
