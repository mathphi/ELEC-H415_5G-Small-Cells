#include "simsetupdialog.h"
#include "ui_simsetupdialog.h"

#include <QKeyEvent>

SimSetupDialog::SimSetupDialog(SimulationData *sim_data, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SimSetupDialog)
{
    ui->setupUi(this);
    m_simulation_data = sim_data;

    connect(ui->validEmitterRadiusSpinBox, SIGNAL(valueChanged(double)), this, SLOT(updateUiComponents()));
}

SimSetupDialog::~SimSetupDialog()
{
    delete ui;
}

/**
 * @brief SimSetupDialog::updateUiComponents
 *
 * This function updates the UI components
 */
void SimSetupDialog::updateUiComponents() {
    // Save state if it was to the minimum (no pruning) value
    bool is_min = ui->pruningRadiusSpinBox->value() == ui->pruningRadiusSpinBox->minimum();

    // Don't set a pruning smaller than minimum valid radius
    ui->pruningRadiusSpinBox->setMinimum(ui->validEmitterRadiusSpinBox->value());

    // If puning was previously set to 'no pruning', restore it
    if (is_min) {
        ui->pruningRadiusSpinBox->setValue(ui->pruningRadiusSpinBox->minimum());
    }
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
    ui->validEmitterRadiusSpinBox->setValue(m_simulation_data->getMinimumValidRadius());

    // Special case for pruning
    double prune_radius = m_simulation_data->getPruningRadius();
    if (isinf(prune_radius)) {
        ui->pruningRadiusSpinBox->setValue(ui->pruningRadiusSpinBox->minimum());
    }
    else {
        ui->pruningRadiusSpinBox->setValue(prune_radius);
    }

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
        m_simulation_data->setMinimumValidRadius(ui->validEmitterRadiusSpinBox->value());

        // Special case for pruning
        prune_radius = ui->pruningRadiusSpinBox->value();
        if (prune_radius == ui->pruningRadiusSpinBox->minimum()) {
            // No pruning means an infinite pruning radius
            m_simulation_data->setPruningRadius(INFINITY);
        }
        else {
            m_simulation_data->setPruningRadius(prune_radius);
        }
    }

    return ans;
}
