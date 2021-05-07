#ifndef COMPUTATIONUNIT_H
#define COMPUTATIONUNIT_H

#include <QObject>
#include <QRunnable>

#include "simulationdata.h"


class SimulationHandler;

class ComputationUnit : public QObject, public QRunnable
{
    Q_OBJECT

public:
    explicit ComputationUnit(SimulationHandler *h, Emitter *e, Receiver *r, Wall *w);

    bool isRunning();
    void run() override;


signals:
    void computationStarted();
    void computationFinished();

private:
    Emitter *m_emitter;
    Receiver *m_receiver;
    Wall *m_wall;

    SimulationHandler *m_handler;
    bool m_running;
};

#endif // COMPUTATIONUNIT_H
