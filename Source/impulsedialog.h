#ifndef IMPULSEDIALOG_H
#define IMPULSEDIALOG_H

#include <QDialog>
#include <QChart>

#include "constants.h"

namespace ImpulseType {
enum ImpulseType {
    Physical,
    TDL,
    UncorrelatedTDL
};
}

namespace Ui {
class ImpulseDialog;
}

class Receiver;

class ImpulseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ImpulseDialog(Receiver *r, QWidget *parent = nullptr);
    ~ImpulseDialog();

private slots:
    void plotSelectedImpulseType();
    void exportCurrentPlot();
    void updateUiComponents();

private:
    void plotDataSet(QMap<double, complex> dataset, QString plot_title);
    QMap<double,complex> computePhysicalImpulse();
    QMap<double,complex> computeTDLImpulse();
    QMap<double,complex> computeUncorrelatedTDLImpulse();

    void exportPlotImage(QString file_path);
    void exportPlotData(QString file_path);

    Ui::ImpulseDialog *ui;
    Receiver *m_receiver;
};

#endif // IMPULSEDIALOG_H
