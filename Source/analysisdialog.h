#ifndef ANALYSISDIALOG_H
#define ANALYSISDIALOG_H

#include <QDialog>
#include <QChart>
#include <QValueAxis>
#include <QLogValueAxis>
#include <QLineSeries>

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
    QtCharts::QLogValueAxis *createDistanceAxis();
    QtCharts::QValueAxis *createValueAxis(QString axis_name, QString fmt = "%.1f");
    void preparePlotsData();
    void exportPlotImage(QString file_path);
    void exportPlotData(QString file_path);

    Ui::AnalysisDialog *ui;

    QList<Receiver*> m_receivers_list;

    QtCharts::QChart *m_power_plot;
    QtCharts::QChart *m_snr_plot;
    QtCharts::QChart *m_delay_plot;
    QtCharts::QChart *m_rice_plot;

    QtCharts::QLineSeries *m_power_series;
    QtCharts::QLineSeries *m_snr_series;
    QtCharts::QLineSeries *m_delay_series;
    QtCharts::QLineSeries *m_rice_series;
};

#endif // ANALYSISDIALOG_H
