#include "optimizerdialog.h"
#include "ui_optimizerdialog.h"

#include "antennas.h"
#include "simulationdata.h"
#include "simulationhandler.h"

OptimizerDialog::OptimizerDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptimizerDialog)
{
    ui->setupUi(this);

    // Disable the help button on title bar
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    // Add items to the antenna type combobox
    for (AntennaType::AntennaType type : AntennaType::AntennaTypeList) {
        // Get an antenna's instance of this type
        Antenna *ant = Antenna::createAntenna(type, 1.0);

        // Add item for each type
        ui->combobox_antenna_type->addItem(ant->getAntennaName(), type);

        // We don't need the antenna's instance anymore
        delete ant;
    }

    connect(ui->spinbox_eirp, SIGNAL(valueChanged(double)), this, SLOT(emitterConfigurationChanged()));
    connect(ui->combobox_antenna_type, SIGNAL(currentIndexChanged(int)), this, SLOT(emitterConfigurationChanged()));
    connect(ui->spinbox_efficiency, SIGNAL(valueChanged(double)), this, SLOT(emitterConfigurationChanged()));

    emitterConfigurationChanged();
}

OptimizerDialog::~OptimizerDialog()
{
    delete ui;
}

AntennaType::AntennaType OptimizerDialog::getAntennaType() {
    return (AntennaType::AntennaType) ui->combobox_antenna_type->currentData().toInt();
}

double OptimizerDialog::getEIRP() {
    return ui->spinbox_eirp->value();
}

double OptimizerDialog::getFrequency() {
    // Frequency is in GHz
    return ui->spinbox_frequency->value() * 1e9;
}

double OptimizerDialog::getEfficiency() {
    // Efficiency is in %
    return ui->spinbox_efficiency->value() / 100.0;
}

double OptimizerDialog::getCoverThreshold() {
    return ui->thresholdSpinBox->value() / 100.0;
}

double OptimizerDialog::getCoverMargin() {
    return ui->coverageMarginSpinBox->value();
}


/**
 * @brief OptimizerDialog::emitterConfigurationChanged
 *
 * This slots update the label to show the converted power in watts
 * from the given eirp value
 */
void OptimizerDialog::emitterConfigurationChanged() {
    QString suffix = "W";

    Antenna *ant = Antenna::createAntenna(getAntennaType(), getEfficiency());
    double power_watts = getEIRP() / ant->getGainMax();
    double power_dbm = SimulationData::convertPowerTodBm(power_watts);

    // Convert to readable units and values
    if (power_watts < 1e-3) {
        suffix = "µW";
        power_watts *= 1e6;
    }
    else if (power_watts < 1) {
        suffix = "mW";
        power_watts *= 1e3;
    }

    delete ant;

    ui->label_power_watts->setText(QString("= %1 %2 = %3 dBm").arg(power_watts, 0, 'f', 2).arg(suffix).arg(power_dbm, 0, 'f', 1));
}


/**
 * @brief OptimizerDialog::exec
 * @return
 *
 * This slot overrides QDialog::exec() to update the UI before showing
 */
int OptimizerDialog::exec() {
    // Update UI with current values
    ui->targetSNRSpinBox->setValue(SimulationHandler::simulationData()->getSimulationTargetSNR());
    ui->pruningRadiusSpinBox->setMinimum(SimulationHandler::simulationData()->getMinimumValidRadius());

    // Special case for pruning
    double prune_radius = SimulationHandler::simulationData()->getPruningRadius();
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
        // Update simulation data according to UI
        SimulationHandler::simulationData()->setSimulationTargetSNR(ui->targetSNRSpinBox->value());

        // Special case for pruning
        prune_radius = ui->pruningRadiusSpinBox->value();
        if (prune_radius == ui->pruningRadiusSpinBox->minimum()) {
            // No pruning means an infinite pruning radius
            SimulationHandler::simulationData()->setPruningRadius(INFINITY);
        }
        else {
            SimulationHandler::simulationData()->setPruningRadius(prune_radius);
        }
    }

    return ans;
}
