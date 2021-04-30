#include "buildingdialog.h"
#include "ui_buildingdialog.h"

#include <QKeyEvent>

BuildingDialog::BuildingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BuildingDialog)
{
    ui->setupUi(this);

    // Disable the help button on title bar
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
}

BuildingDialog::~BuildingDialog()
{
    delete ui;
}

void BuildingDialog::keyPressEvent(QKeyEvent *event) {
    // Prevent dialog closing on enter key press
    if(event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return)
        return;

    QDialog::keyPressEvent(event);
}

QSize BuildingDialog::getBuildingSize() {
    return QSize(
        ui->buildingWidthSpinBox->value(),
        ui->buildingHeightSpinBox->value()
    );
}
