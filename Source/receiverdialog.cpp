#include "ui_receiverdialog.h"
#include "receiverdialog.h"
#include "simulationdata.h"

#include <QKeyEvent>

ReceiverDialog::ReceiverDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ReceiverDialog)
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
}

ReceiverDialog::ReceiverDialog(Receiver *em, QWidget *parent)
    : ReceiverDialog(parent)
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
    ui->spinbox_efficiency->setValue(em->getEfficiency() * 100.0);
}

ReceiverDialog::~ReceiverDialog()
{
    delete ui;
}

void ReceiverDialog::keyPressEvent(QKeyEvent *event) {
    // Prevent dialog closing on enter key press
    if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        return;

    QDialog::keyPressEvent(event);
}

AntennaType::AntennaType ReceiverDialog::getAntennaType() {
    return (AntennaType::AntennaType) ui->combobox_antenna_type->currentData().toInt();
}

double ReceiverDialog::getEfficiency() {
    // Efficiency is in %
    return ui->spinbox_efficiency->value() / 100.0;
}
