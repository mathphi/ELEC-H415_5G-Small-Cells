#ifndef OPTIMIZERDIALOG_H
#define OPTIMIZERDIALOG_H

#include <QDialog>

#include "antennas.h"

namespace Ui {
class OptimizerDialog;
}

class OptimizerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptimizerDialog(QWidget *parent = nullptr);
    ~OptimizerDialog();

    AntennaType::AntennaType getAntennaType();
    double getEIRP();
    double getFrequency();
    double getEfficiency();

    double getCoverThreshold();
    double getCoverMargin();

public slots:
    virtual int exec();

private slots:
    void emitterConfigurationChanged();

private:
    Ui::OptimizerDialog *ui;
};

#endif // OPTIMIZERDIALOG_H
