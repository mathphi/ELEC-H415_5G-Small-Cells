#include "ui_emitterdialog.h"
#include "emitterdialog.h"
#include "simulationdata.h"

#include <QKeyEvent>

EmitterDialog::EmitterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EmitterDialog)
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

    connect(ui->spinbox_power, SIGNAL(valueChanged(double)), this, SLOT(powerSpinboxChanged(double)));
}

EmitterDialog::EmitterDialog(Emitter *em, QWidget *parent)
    : EmitterDialog(parent)
{
    int type_index = 0;

    // Get the index of the antenna type in the combobox
    for (int i = 0 ; i < ui->combobox_antenna_type->count() ; i++) {
        if (ui->combobox_antenna_type->itemData(i) == em->getAntenna()->getAntennaType()) {
            type_index = i;
        }
    }

    // Set the initial data in the fields
    ui->combobox_antenna_type->setCurrentIndex(type_index);
    ui->spinbox_frequency->setValue(em->getFrequency() / 1.0e9);
    ui->spinbox_efficiency->setValue(em->getEfficiency() * 100.0);
    ui->spinbox_power->setValue(SimulationData::convertPowerTodBm(em->getPower()));
}

EmitterDialog::~EmitterDialog()
{
    delete ui;
}

void EmitterDialog::keyPressEvent(QKeyEvent *event) {
    // Prevent dialog closing on enter key press
    if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        return;

    QDialog::keyPressEvent(event);
}

AntennaType::AntennaType EmitterDialog::getAntennaType() {
    return (AntennaType::AntennaType) ui->combobox_antenna_type->currentData().toInt();
}

double EmitterDialog::getPower() {
    // Power is in dBm
    return SimulationData::convertPowerToWatts(ui->spinbox_power->value());
}

double EmitterDialog::getFrequency() {
    // Frequency is in GHz
    return ui->spinbox_frequency->value() * 1e9;
}

double EmitterDialog::getEfficiency() {
    // Efficiency is in %
    return ui->spinbox_efficiency->value() / 100.0;
}

/**
 * @brief EmitterDialog::powerSpinboxChanged
 * @param value
 *
 * This slots update the label to show the converted power in watts
 * from the given value in dBm
 */
void EmitterDialog::powerSpinboxChanged(double value) {
    QString suffix = "W";

    double power_watts = SimulationData::convertPowerToWatts(value);

    // Convert to readable units and values
    if (power_watts < 1e-3) {
        suffix = "nW";
        power_watts *= 1e6;
    }
    else if (power_watts < 1) {
        suffix = "mW";
        power_watts *= 1e3;
    }

    ui->label_power_watts->setText(QString("= %1 %2").arg(power_watts, 0, 'f', 2).arg(suffix));
}
