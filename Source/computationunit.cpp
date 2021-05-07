#include "computationunit.h"
#include "simulationhandler.h"

ComputationUnit::ComputationUnit(SimulationHandler *h, Receiver *r) :
    QObject(h), QRunnable()
{
    // Don't delete the computation unit when finished
    setAutoDelete(false);

    m_handler = h;
    m_receiver = r;

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

    // Compute the reflections recursively
    m_handler->computeReceiverRays(m_receiver);

    // Mark this CU as stopped
    m_running = false;

    // Emit computation finished signal
    emit computationFinished();
}
