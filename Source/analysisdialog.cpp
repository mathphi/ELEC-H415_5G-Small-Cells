#include "analysisdialog.h"
#include "ui_analysisdialog.h"

#include "simulationhandler.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <QDebug>

using namespace QtCharts;


AnalysisDialog::AnalysisDialog(QList<Receiver *> rcv_list, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnalysisDialog)
{
    ui->setupUi(this);

    // Remove the help button
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);

    // Antialiasing on plots
    ui->chartView->setRenderHint(QPainter::Antialiasing);

    // Store the receivers list
    // This list is ordered from 0 meter
    m_receivers_list = rcv_list;

    // Prepare plots data
    preparePlotsData();

    // Add result types into combobox
    ui->resultTypeComboBox->addItem("Received power",   ResultType::Power);
    ui->resultTypeComboBox->addItem("SNR at UE",        ResultType::SNR);
    ui->resultTypeComboBox->addItem("Delay spread",     ResultType::DelaySpread);
    ui->resultTypeComboBox->addItem("Rice factor",      ResultType::RiceFactor);
    ui->resultTypeComboBox->setCurrentIndex(0);

    connect(ui->resultTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedTypeChanged()));
    connect(ui->button_export,      SIGNAL(clicked()),                this, SLOT(exportCurrentPlot()));
    connect(ui->button_close,       SIGNAL(clicked()),                this, SLOT(close()));

    // Show the default type selection
    selectedTypeChanged();
}

AnalysisDialog::~AnalysisDialog()
{
    delete ui;
    delete m_power_plot;
    delete m_snr_plot;
    delete m_delay_plot;
    delete m_rice_plot;
}


void AnalysisDialog::preparePlotsData() {
    // Generate the plots
    QLineSeries *power_series   = new QLineSeries();
    QLineSeries *snr_series     = new QLineSeries();
    QLineSeries *delay_series   = new QLineSeries();
    QLineSeries *rice_series    = new QLineSeries();

    // Temporary variables for the loop
    double d, logd;
    double power_val, snr_val, delay_val, rice_val;

    for (int i = 0 ; i < m_receivers_list.size() ; i++) {
        // Pointer to the receiver at distance d
        Receiver *r = m_receivers_list.at(i);

        // Compute the distance (starts at 1m)
        d = (i+1);
        logd = log10(d);

        // Get and parse the corresponding values
        power_val   = SimulationData::convertPowerTodBm(r->receivedPower());
        snr_val     = r->userEndSNR();
        delay_val   = r->delaySpread();
        rice_val    = r->riceFactor();

        if (!isnan(power_val) && !isinf(power_val)) {
            // Append this data to the plot
            *power_series << QPointF(logd, power_val);
        }
        if (!isnan(snr_val) && !isinf(snr_val)) {
            // Append this data to the plot
            *snr_series << QPointF(logd, snr_val);
        }
        if (!isnan(delay_val) && !isinf(delay_val)) {
            // Append this data to the plot
            *delay_series << QPointF(logd, delay_val);
        }
        if (!isnan(rice_val) && !isinf(rice_val)) {
            // Append this data to the plot
            *rice_series << QPointF(logd, rice_val);
        }
    }

    // Create the plots
    m_power_plot    = new QChart();
    m_snr_plot      = new QChart();
    m_delay_plot    = new QChart();
    m_rice_plot     = new QChart();

    // Hide legend
    m_power_plot->legend()->hide();
    m_snr_plot->legend()->hide();
    m_delay_plot->legend()->hide();
    m_rice_plot->legend()->hide();

    // Add prepared data to plots
    m_power_plot->addSeries(power_series);
    m_snr_plot->addSeries(snr_series);
    m_delay_plot->addSeries(delay_series);
    m_rice_plot->addSeries(rice_series);

    m_power_plot->createDefaultAxes();
    m_snr_plot->createDefaultAxes();
    m_delay_plot->createDefaultAxes();
    m_rice_plot->createDefaultAxes();

    m_power_plot->axes(Qt::Horizontal).first()->setTitleText("Distance log(d)");


    m_power_plot->setTitle("Received power as a function of the distance");
    m_snr_plot  ->setTitle("SNR as a function of the distance");
    m_delay_plot->setTitle("Delay spread as a function of the distance");
    m_rice_plot ->setTitle("Rice factor as a function of the distance");
}

void AnalysisDialog::selectedTypeChanged() {
    ResultType::ResultType r_type = (ResultType::ResultType) ui->resultTypeComboBox->currentData().toInt();

    switch (r_type) {
    case ResultType::Power: {
        ui->chartView->setChart(m_power_plot);
        break;
    }
    case ResultType::SNR: {
        ui->chartView->setChart(m_snr_plot);
        break;
    }
    case ResultType::DelaySpread: {
        ui->chartView->setChart(m_delay_plot);
        break;
    }
    case ResultType::RiceFactor: {
        ui->chartView->setChart(m_rice_plot);
        break;
    }
    }
}


void AnalysisDialog::exportCurrentPlot() {

}
