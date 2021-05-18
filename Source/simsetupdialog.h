#ifndef SIMSETUPDIALOG_H
#define SIMSETUPDIALOG_H

#include <QDialog>

#include "simulationdata.h"

namespace Ui {
class SimSetupDialog;
}

class SimSetupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SimSetupDialog(SimulationData *sim_data, QWidget *parent = nullptr);
    ~SimSetupDialog();

public slots:
    virtual int exec();

private slots:
    void updateUiComponents();

private:
    Ui::SimSetupDialog *ui;
    SimulationData *m_simulation_data;
};

#endif // SIMSETUPDIALOG_H
