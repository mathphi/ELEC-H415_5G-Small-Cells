#include "computationunit.h"
#include "simulationhandler.h"

ComputationUnit::ComputationUnit(SimulationHandler *h, QList<Receiver *> r_lst) :
    QObject(h), QRunnable()
{
    // Don't delete the computation unit when finished
    setAutoDelete(false);

    m_handler = h;
    m_receivers_list = r_lst;

    // Mark this CU as stopped
    m_running = false;
}

bool ComputationUnit::isRunning() {
    return m_running;
}

/**
 * @brief ComputationUnit::run
 *
 * This function is called when a thread is ready to run it
 */
void ComputationUnit::run() {
    // Mark this CU as running
    m_running = true;

    // Emit computation started signal
    emit computationStarted();

    // For all receivers of the list
    foreach(Receiver *r, m_receivers_list) {
        // Compute the reflections recursively
        m_handler->computeReceiverRays(r);
    }

    // Mark this CU as stopped
    m_running = false;

    // Emit computation finished signal
    emit computationFinished();
}
