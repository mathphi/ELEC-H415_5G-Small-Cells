#include "computationunit.h"
#include "simulationhandler.h"

ComputationUnit::ComputationUnit(SimulationHandler *h, Emitter *e, Receiver *r, Wall *w) :
    QObject(h), QRunnable()
{
    // Don't delete the computation unit when finished
    setAutoDelete(false);

    m_handler = h;
    m_emitter = e;
    m_receiver = r;
    m_wall = w;

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
    m_handler->recursiveReflection(m_emitter, m_receiver, m_wall);

    // Mark this CU as stopped
    m_running = false;

    // Emit computation finished signal
    emit computationFinished();
}
