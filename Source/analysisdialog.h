#ifndef ANALYSISDIALOG_H
#define ANALYSISDIALOG_H

#include <QDialog>
#include <QChart>

#include "receiver.h"

namespace Ui {
class AnalysisDialog;
}

class AnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AnalysisDialog(QList<Receiver*> rcv_list, QWidget *parent = nullptr);
    ~AnalysisDialog();

private slots:
    void selectedTypeChanged();

    void exportCurrentPlot();

private:
    void preparePlotsData();

    Ui::AnalysisDialog *ui;

    QList<Receiver*> m_receivers_list;

    QtCharts::QChart *m_power_plot;
    QtCharts::QChart *m_snr_plot;
    QtCharts::QChart *m_delay_plot;
    QtCharts::QChart *m_rice_plot;
};

#endif // ANALYSISDIALOG_H
