#include "analysisdialog.h"
#include "ui_analysisdialog.h"

#include "simulationhandler.h"
#include "mainwindow.h"

#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLogValueAxis>
#include <QFileDialog>
#include <QMessageBox>

#include <QDebug>

QT_CHARTS_USE_NAMESPACE


AnalysisDialog::AnalysisDialog(QList<Receiver *> rcv_list, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AnalysisDialog)
{
    ui->setupUi(this);

    // Remove the help button
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::WindowMaximizeButtonHint, true);

    // Antialiasing on plots
    ui->chartView->setRenderHint(QPainter::Antialiasing, true);
    ui->chartView->setRenderHint(QPainter::TextAntialiasing, true);

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
    delete m_power_plot;
    delete m_snr_plot;
    delete m_delay_plot;
    delete m_rice_plot;
    delete ui;
}

QLogValueAxis *AnalysisDialog::createDistanceAxis() {
    QLogValueAxis *axis = new QLogValueAxis();
    axis->setTitleText("Distance [m]");
    axis->setLabelFormat("%.1f");
    axis->setBase(10.0);
    axis->setMinorTickCount(10);

    return axis;
}

QValueAxis *AnalysisDialog::createValueAxis(QString axis_name) {
    QValueAxis *axis = new QValueAxis();
    axis->setTitleText(axis_name);
    axis->setLabelFormat("%.1f");
    axis->setTickCount(-1);

    return axis;
}

void AnalysisDialog::preparePlotsData() {
    // Generate the plots
    QLineSeries *power_series   = new QLineSeries();
    QLineSeries *snr_series     = new QLineSeries();
    QLineSeries *delay_series   = new QLineSeries();
    QLineSeries *rice_series    = new QLineSeries();

    // Temporary variables for the loop
    double d;
    double power_val, snr_val, delay_val, rice_val;

    for (int i = 1 ; i < m_receivers_list.size() ; i++) {
        // Pointer to the receiver at distance d
        Receiver *r = m_receivers_list.at(i);

        // Compute the distance (starts at 0m)
        d = i;

        // Get and parse the corresponding values
        power_val   = SimulationData::convertPowerTodBm(r->receivedPower());
        snr_val     = r->userEndSNR();
        delay_val   = r->delaySpread();
        rice_val    = r->riceFactor();

        if (!isnan(power_val) && !isinf(power_val)) {
            // Append this data to the plot
            *power_series << QPointF(d, power_val);
        }
        if (!isnan(snr_val) && !isinf(snr_val)) {
            // Append this data to the plot
            *snr_series << QPointF(d, snr_val);
        }
        if (!isnan(delay_val) && !isinf(delay_val)) {
            // Append this data to the plot
            *delay_series << QPointF(d, delay_val);
        }
        if (!isnan(rice_val) && !isinf(rice_val)) {
            // Append this data to the plot
            *rice_series << QPointF(d, rice_val);
        }
    }

    // Pointer to these series
    m_power_series  = power_series;
    m_snr_series    = snr_series;
    m_delay_series  = delay_series;
    m_rice_series   = rice_series;

    // Create the plots
    m_power_plot    = new QChart();
    m_snr_plot      = new QChart();
    m_delay_plot    = new QChart();
    m_rice_plot     = new QChart();

    // Hide legend
    m_power_plot->legend()->hide();
    m_snr_plot  ->legend()->hide();
    m_delay_plot->legend()->hide();
    m_rice_plot ->legend()->hide();

    // Add prepared data to plots
    m_power_plot->addSeries(power_series);
    m_snr_plot  ->addSeries(snr_series);
    m_delay_plot->addSeries(delay_series);
    m_rice_plot ->addSeries(rice_series);

    QLogValueAxis *power_axisX  = createDistanceAxis();
    QLogValueAxis *snr_axisX    = createDistanceAxis();
    QLogValueAxis *delay_axisX  = createDistanceAxis();
    QLogValueAxis *rice_axisX   = createDistanceAxis();

    m_power_plot->addAxis(power_axisX,  Qt::AlignBottom);
    m_snr_plot  ->addAxis(snr_axisX,    Qt::AlignBottom);
    m_delay_plot->addAxis(delay_axisX,  Qt::AlignBottom);
    m_rice_plot ->addAxis(rice_axisX,   Qt::AlignBottom);

    power_series->attachAxis(power_axisX);
    snr_series  ->attachAxis(snr_axisX);
    delay_series->attachAxis(delay_axisX);
    rice_series ->attachAxis(rice_axisX);

    QValueAxis *power_axisY = createValueAxis("Received power [dBm]");
    QValueAxis *snr_axisY   = createValueAxis("SNE at UE [dB]");
    QValueAxis *delay_axisY = createValueAxis("Delay spread [s]");
    QValueAxis *rice_axisY  = createValueAxis("Rice factor [dB]");

    m_power_plot->addAxis(power_axisY,  Qt::AlignLeft);
    m_snr_plot  ->addAxis(snr_axisY,    Qt::AlignLeft);
    m_delay_plot->addAxis(delay_axisY,  Qt::AlignLeft);
    m_rice_plot ->addAxis(rice_axisY,   Qt::AlignLeft);

    power_series->attachAxis(power_axisY);
    snr_series  ->attachAxis(snr_axisY);
    delay_series->attachAxis(delay_axisY);
    rice_series ->attachAxis(rice_axisY);

    power_axisY->applyNiceNumbers();
    snr_axisY  ->applyNiceNumbers();
    delay_axisY->applyNiceNumbers();
    rice_axisY ->applyNiceNumbers();

    m_power_plot->setTitle("Received power as a function of the distance");
    m_snr_plot  ->setTitle("SNR as a function of the distance");
    m_delay_plot->setTitle("Delay spread as a function of the distance");
    m_rice_plot ->setTitle("Rice factor as a function of the distance");

    // Set title font (large and bold)
    QFont font = m_power_plot->titleFont();
    font.setBold(true);
    font.setPointSizeF(11.0);
    m_power_plot->setTitleFont(font);
    m_snr_plot  ->setTitleFont(font);
    m_delay_plot->setTitleFont(font);
    m_rice_plot ->setTitleFont(font);
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
    // Open file selection dialog
    QString file_path = QFileDialog::getSaveFileName(
                this,
                "Export 1-D analysis plot",
                MainWindow::lastUsedDirectory().path(),
                "JPG (*.jpg);;PNG (*.png);;TIFF (*.tiff);;CSV File (*.csv)");

    // If the user cancelled the dialog
    if (file_path.isEmpty()) {
        return;
    }

    // Set the last used directory
    MainWindow::setLastUsedDirectory(QFileInfo(file_path).dir());

    if (QFileInfo(file_path).suffix() == "csv") {
        exportPlotData(file_path);
    }
    else {
        exportPlotImage(file_path);
    }
}

void AnalysisDialog::exportPlotImage(QString file_path) {
    // Prepare an image with the double resolution of the scene
    QImage image(ui->chartView->sceneRect().size().toSize()*4, QImage::Format_ARGB32);

    // Fill background transparent only if PNG is selected as the destination format
    if (QFileInfo(file_path).suffix().toLower() == "png") {
        image.fill(Qt::transparent);
    }
    else {
        image.fill(Qt::white);
    }

    // Paint the scene into the image
    QPainter painter(&image);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    ui->chartView->scene()->render(&painter, QRectF(), ui->chartView->sceneRect());

    // Write the exported image into the file
    if (!image.save(file_path)) {
        QMessageBox::critical(this, "Error", "Unable to write into the selected file");
        return;
    }
}

void AnalysisDialog::exportPlotData(QString file_path) {
    // File handler instance
    QFile csv_file(file_path);

    // Open in write mode
    if (!csv_file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Unable to write into the selected file");
        return;
    }

    // Get the current series
    QLineSeries *current_series;
    ResultType::ResultType r_type = (ResultType::ResultType) ui->resultTypeComboBox->currentData().toInt();

    switch (r_type) {
    case ResultType::Power: {
        current_series = m_power_series;
        break;
    }
    case ResultType::SNR: {
        current_series = m_snr_series;
        break;
    }
    case ResultType::DelaySpread: {
        current_series = m_delay_series;
        break;
    }
    case ResultType::RiceFactor: {
        current_series = m_rice_series;
        break;
    }
    }

    // Get the data points
    QList<QPointF> points = current_series->points();

    // Put data points into the file
    QString csv_content;

    // Each line is a tab-separated values line
    foreach(QPointF pt, points) {
        csv_content.append(QString("%1,%2\n").arg(pt.x(), 0, 'f', 10).arg(pt.y(), 0, 'f', 10));
    }

    // Write content to file
    csv_file.write(csv_content.toUtf8());

    // Close the file
    csv_file.close();
}
