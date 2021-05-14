#include "simsetupdialog.h"
#include "ui_simsetupdialog.h"

#include <QKeyEvent>

SimSetupDialog::SimSetupDialog(SimulationData *sim_data, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SimSetupDialog)
{
    ui->setupUi(this);
    m_simulation_data = sim_data;
}

SimSetupDialog::~SimSetupDialog()
{
    delete ui;
}


/**
 * @brief SimSetupDialog::exec
 * @return
 *
 * This slot overrides QDialog::exec() to update the UI before showing
 */
int SimSetupDialog::exec() {
    ui->maxReflectionsCountSpinBox->setValue(m_simulation_data->maxReflectionsCount());
    ui->reflectionNLOSCheckBox->setChecked(m_simulation_data->reflectionEnabledNLOS());
    ui->simulationHeightSpinBox->setValue(m_simulation_data->getSimulationHeight());
    ui->permittivitySpinBox->setValue(m_simulation_data->getRelPermitivity());
    ui->bandwidthSpinBox->setValue(m_simulation_data->getSimulationBandwidth() / 1e6);
    ui->temperatureSpinBox->setValue(SimulationData::convertKelvinToCelsius(m_simulation_data->getSimulationTemperature()));
    ui->noiseFigureSpinBox->setValue(m_simulation_data->getSimulationNoiseFigure());
    ui->targetSNRSpinBox->setValue(m_simulation_data->getSimulationTargetSNR());

    // Execute the dialog
    int ans = QDialog::exec();

    // Apply the new values if changes were accepted
    if (ans == QDialog::Accepted) {
        m_simulation_data->setReflectionsCount(ui->maxReflectionsCountSpinBox->value());
        m_simulation_data->setReflectionEnabledNLOS(ui->reflectionNLOSCheckBox->isChecked());
        m_simulation_data->setSimulationHeight(ui->simulationHeightSpinBox->value());
        m_simulation_data->setRelPermitivity(ui->permittivitySpinBox->value());
        m_simulation_data->setSimulationBandwidth(ui->bandwidthSpinBox->value() * 1e6);
        m_simulation_data->setSimulationTemperature(SimulationData::convertCelsiusToKelvin(ui->temperatureSpinBox->value()));
        m_simulation_data->setSimulationNoiseFigure(ui->noiseFigureSpinBox->value());
        m_simulation_data->setSimulationTargetSNR(ui->targetSNRSpinBox->value());
    }

    return ans;
}
