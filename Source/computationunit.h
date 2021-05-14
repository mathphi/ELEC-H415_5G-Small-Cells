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
    explicit ComputationUnit(SimulationHandler *h, QList<Receiver*> r_lst);

    bool isRunning();
    void run() override;


signals:
    void computationStarted();
    void computationFinished();

private:
    QList<Receiver*> m_receivers_list;
    SimulationHandler *m_handler;
    bool m_running;
};

#endif // COMPUTATIONUNIT_H
