#include "impulsedialog.h"
#include "ui_impulsedialog.h"

#include "receiver.h"
#include "constants.h"
#include "simulationdata.h"
#include "mainwindow.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QScatterSeries>
#include <QLineSeries>
#include <QValueAxis>
#include <QChart>
#include <QFont>

#include <QDebug>

#define MARKER_SIZE     8.0
#define MARKER_BORDER   1.0
#define MARKER_LINE_W   1.0

QT_CHARTS_USE_NAMESPACE


ImpulseDialog::ImpulseDialog(Receiver *r, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImpulseDialog)
{
    ui->setupUi(this);

    // Store receiver pointer
    m_receiver = r;

    // Remove the help button
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    setWindowFlag(Qt::WindowMaximizeButtonHint, true);

    // Antialiasing on plots
    ui->chartView->setRenderHint(QPainter::Antialiasing, true);
    ui->chartView->setRenderHint(QPainter::TextAntialiasing, true);

    // Initial spinbox bandwidth = simulation bandwidth
    ui->bandwidthSpinBox->setValue(SimulationHandler::simulationData()->getSimulationBandwidth() / 1e6);

    // Add impulses types into combobox
    ui->impulseTypeComboBox->addItem("Physical impulse response",   ImpulseType::Physical);
    ui->impulseTypeComboBox->addItem("TDL impulse response",        ImpulseType::TDL);
    ui->impulseTypeComboBox->addItem("Uncorrelated scattering TDL", ImpulseType::UncorrelatedTDL);
    ui->impulseTypeComboBox->setCurrentIndex(0);

    connect(ui->impulseTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateUiComponents()));
    connect(ui->impulseTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(plotSelectedImpulseType()));
    connect(ui->bandwidthSpinBox,    SIGNAL(valueChanged(double)),     this, SLOT(updateUiComponents()));
    connect(ui->bandwidthSpinBox,    SIGNAL(valueChanged(double)),     this, SLOT(plotSelectedImpulseType()));
    connect(ui->checkbox_time0,      SIGNAL(stateChanged(int)),        this, SLOT(plotSelectedImpulseType()));
    connect(ui->button_export,       SIGNAL(clicked()),                this, SLOT(exportCurrentPlot()));
    connect(ui->button_close,        SIGNAL(clicked()),                this, SLOT(close()));

    // Plot the default plot
    plotSelectedImpulseType();
    updateUiComponents();
}

ImpulseDialog::~ImpulseDialog()
{
    delete ui;
}

void ImpulseDialog::plotSelectedImpulseType() {
    // Get the currently selected impulse response
    ImpulseType::ImpulseType imp_type = (ImpulseType::ImpulseType) ui->impulseTypeComboBox->currentData().toInt();

    // Get the corresponding data set and generate the title
    QString plot_title;
    QMap<double,complex> dataset;

    switch (imp_type) {
    case ImpulseType::Physical: {
        plot_title = "Physical impulse response";
        dataset = computePhysicalImpulse();
        break;
    }
    case ImpulseType::TDL: {
        double bw_value = ui->bandwidthSpinBox->value();
        QString bw_string = QString("Bandwidth: %1 MHz").arg(bw_value);
        plot_title = "TDL impulse response - %1";
        plot_title = plot_title.arg(bw_value > 0 ? bw_string : "Narrowband");
        dataset = computeTDLImpulse();
        break;
    }
    case ImpulseType::UncorrelatedTDL: {
        double bw_value = ui->bandwidthSpinBox->value();
        QString bw_string = QString("Bandwidth: %1 MHz").arg(bw_value);
        plot_title = "Uncorrelated scattering TDL - %1";
        plot_title = plot_title.arg(bw_value > 0 ? bw_string : "Narrowband");
        dataset = computeUncorrelatedTDLImpulse();
        break;
    }
    }

    // Plot the new dataset
    plotDataSet(dataset, plot_title);
}

void ImpulseDialog::plotDataSet(QMap<double, complex> dataset, QString plot_title) {
    // Keep a pointer to the currently displayed chart
    QChart *prev_chart = ui->chartView->chart();

    // Create the chart series
    QScatterSeries *series = new QScatterSeries;
    series->setMarkerShape(QScatterSeries::MarkerShapeCircle);
    series->setMarkerSize(MARKER_SIZE);
    series->setBrush(Qt::white);
    series->setPen(QPen(Qt::blue, MARKER_BORDER));

    QList<double> keys = dataset.keys();
    QList<complex> values = dataset.values();

    // Get human readable time
    QString time_units;
    double factor;

    // Draw an empty chart if no data
    if (dataset.size() > 0) {
        SimulationData::delayToHumanReadable(keys.last(), &time_units, &factor);
    }
    else {
        SimulationData::delayToHumanReadable(1, &time_units, &factor);
    }

    // Add all data into the series
    for (int i = 0 ; i < dataset.size() ; i++) {
        series->append(QPointF(keys.at(i) / factor, 10*log10(abs(values.at(i)))));
    }

    // Create the plot
    QChart *chart = new QChart();

    // Add the data points to the plot
    chart->addSeries(series);
    chart->setTitle(plot_title);

    // Set title font (large and bold)
    QFont font = chart->titleFont();
    font.setBold(true);
    font.setPointSizeF(11.0);
    chart->setTitleFont(font);

    // Add axis
    QValueAxis *axisX = new QValueAxis();
    axisX->setLabelFormat("%.1f");
    axisX->setTitleText(QString("Delay [%1]").arg(time_units));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat("%.1f");
    axisY->setTitleText("Magnitude [dB]");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Set axis fonts
    QFont ax_font = axisX->titleFont();
    ax_font.setBold(true);
    axisX->setTitleFont(ax_font);
    axisY->setTitleFont(ax_font);

    // These functions doesn't work well with only one element
    if (dataset.size() > 1) {
        // Show nice range and values
        axisX->applyNiceNumbers();
        axisY->applyNiceNumbers();
    }

    // Compute the range mean
    double x_range_mean = qMax(10.0, (axisX->max() - axisX->min()) / 2.0);
    double y_range_mean = qMax(10.0, qAbs((axisY->max() - axisY->min()) / 2.0));

    // If the checkbox is checked -> start time to 0
    double x_min = qMax(0.0, axisX->min() - x_range_mean * 0.05);   // Add 5% of avg to the max
    if (ui->checkbox_time0->isChecked()) {
        x_min = 0.0;
        x_range_mean = qMax(10.0, axisX->max() / 2.0);
    }

    // Force axis X and Y to start at 0
    axisX->setRange(x_min, axisX->max() + x_range_mean * 0.05);     // Add 5% to the max
    axisY->setRange(axisY->min() - y_range_mean * 0.05,
                    axisY->max() + y_range_mean * 0.05);            // Add 5% of avg to the max

    // Hide the legend
    chart->legend()->hide();

    // Add vertical lines to each value
    foreach (QPointF p, series->points()) {
        // Create a line series with a vertical line to y=0
        QLineSeries *ls = new QLineSeries();
        ls->append(QPointF(p.x(), p.y()));
        ls->append(QPointF(p.x(), axisY->min()));

        // Add each vertical line to the plot
        chart->addSeries(ls);
        ls->attachAxis(axisX);
        ls->attachAxis(axisY);

        // Change the pen width
        QPen pen = ls->pen();
        pen.setWidthF(MARKER_LINE_W);
        ls->setPen(pen);
    }

    // Remove then re-add main series (put it on the top order)
    chart->removeSeries(series);
    chart->addSeries(series);
    series->attachAxis(axisX);
    series->attachAxis(axisY);

    // Change the displayed chart
    ui->chartView->setChart(chart);

    // If there was a chart displayed previously, delete it
    if (prev_chart) {
        delete prev_chart;
    }
}

void ImpulseDialog::exportCurrentPlot() {
    // Open file selection dialog
    QString file_path = QFileDialog::getSaveFileName(
                this,
                "Export impulse response plot",
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


void ImpulseDialog::exportPlotImage(QString file_path) {
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

void ImpulseDialog::exportPlotData(QString file_path) {
    // File handler instance
    QFile csv_file(file_path);

    // Open in write mode
    if (!csv_file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error", "Unable to write into the selected file");
        return;
    }

    // Get the currently selected impulse response
    ImpulseType::ImpulseType imp_type = (ImpulseType::ImpulseType) ui->impulseTypeComboBox->currentData().toInt();

    // Get the corresponding data set
    QMap<double,complex> dataset;

    switch (imp_type) {
    case ImpulseType::Physical: {
        dataset = computePhysicalImpulse();
        break;
    }
    case ImpulseType::TDL: {
        dataset = computeTDLImpulse();
        break;
    }
    case ImpulseType::UncorrelatedTDL: {
        dataset = computeUncorrelatedTDLImpulse();
        break;
    }
    }

    // Put data points into the file
    QString csv_content;

    // Dataset lists
    QList<double> keys = dataset.keys();
    QList<complex> values_c = dataset.values();

    // Each line is a tab-separated values line
    for (int i = 0 ; i < dataset.size() ; i++) {
        csv_content.append(QString("%1,%2\n").arg(keys.at(i), 0, 'f', 10).arg(abs(values_c.at(i)), 0, 'f', 10));
    }

    // Write content to file
    csv_file.write(csv_content.toUtf8());

    // Close the file
    csv_file.close();
}

void ImpulseDialog::updateUiComponents() {
    // Get the impulse type
    ImpulseType::ImpulseType imp_type = (ImpulseType::ImpulseType) ui->impulseTypeComboBox->currentData().toInt();

    // The bandidth spinbox is visible only for TDL impulse response
    ui->group_bandwidth->setVisible(imp_type == ImpulseType::TDL || imp_type == ImpulseType::UncorrelatedTDL);

    // Change the Time resultion label content
    if (ui->bandwidthSpinBox->value() > 0) {
        // If not narrowband -> show time resolution
        QString tau_units;
        double delta_tau = 1 / (2.0 * ui->bandwidthSpinBox->value() * 1e6);
        delta_tau = SimulationData::delayToHumanReadable(delta_tau, &tau_units);
        ui->label_time_resolution->setText(QString("Time resolution: %1 %2").arg(delta_tau, 0, 'f', 2).arg(tau_units));
    }
    else {
        // If narrowband -> show narrowband text
        ui->label_time_resolution->setText(QString("Time resolution: Narrowband"));
    }
}


QMap<double, complex> ImpulseDialog::computePhysicalImpulse() {
    // Create a triple polarization vector
    vector<complex> polariz = {
        m_receiver->getAntenna()->getPolarization()[0],
        m_receiver->getAntenna()->getPolarization()[0],
        m_receiver->getAntenna()->getPolarization()[1]
    };

    // Compute the complex taps first
    QHash<double,complex> imp_taps;

    // Generate the taps
    foreach(RayPath *rp, m_receiver->getRayPaths()) {
        double ampl  = rp->getAmplitude();
        double phase = arg(dotProduct(rp->getElectricField(), polariz));
        complex tap  = ampl*exp(1i*phase);
        double tau   = rp->getDelay();

        imp_taps.insert(tau, imp_taps.value(tau, 0.0) + tap);
    }

    // Create a sorted list of keys
    QList<double> keys_list = imp_taps.keys();
    std::sort(keys_list.begin() , keys_list.end());

    // Sort the taps by tau increasing (and keep only the modulus)
    QMap<double,complex> impulse_resp;
    foreach(double tau, keys_list) {
        impulse_resp.insert(tau, imp_taps.value(tau));
    }

    return impulse_resp;
}

QMap<double,complex> ImpulseDialog::computeTDLImpulse() {
    // Get the physical impulse response
    QMap<double,complex> phys_imp = computePhysicalImpulse();

    // Get the configured bandwidth
    double bw = ui->bandwidthSpinBox->value() * 1e6;

    double delta_tau;
    if (ui->bandwidthSpinBox->value() > 0) {
        // Compute the time resolution
        delta_tau = 1 / (2.0 * bw);
    }
    else {
        // If narrowband -> delta tau is the whole time range
        delta_tau = phys_imp.lastKey();
    }

    // TDL impulse response
    double tau_key;
    QMap<double,complex> tdl_imp;

    // Place the taps in their respective discrete taps
    foreach (double phys_tau, phys_imp.keys()) {
        // Get the physical impulse value at this tau
        complex phys_val = phys_imp.value(phys_tau);

        // Get the discrete tau
        tau_key = ceil(phys_tau / delta_tau) * delta_tau;

        // TDL sinc factor
        double sinc_factor = sinc(2*bw * (phys_tau - tau_key));

        // Accumulate physical taps in this tap
        tdl_imp.insert(tau_key, phys_val*sinc_factor + tdl_imp.value(tau_key, 0.0));
    }

    return tdl_imp;
}

QMap<double,complex> ImpulseDialog::computeUncorrelatedTDLImpulse() {
    // Get the physical impulse response
    QMap<double,complex> phys_imp = computePhysicalImpulse();

    double delta_tau;
    if (ui->bandwidthSpinBox->value() > 0) {
        // Compute the time resolution
        delta_tau = 1 / (2.0 * ui->bandwidthSpinBox->value() * 1e6);
    }
    else {
        // If narrowband -> delta tau is the whole time range
        delta_tau = phys_imp.lastKey();
    }

    // TDL impulse response under US assumption
    double tau_key;
    QMap<double,complex> tdl_us_imp;

    // Place the taps in their respective discrete taps
    foreach (double phys_tau, phys_imp.keys()) {
        // Get the physical impulse value at this tau
        complex phys_val = phys_imp.value(phys_tau);

        // Get the discrete tau
        tau_key = ceil(phys_tau / delta_tau) * delta_tau;

        // Accumulate physical taps in this tap
        tdl_us_imp.insert(tau_key, phys_val + tdl_us_imp.value(tau_key, 0.0));
    }

    return tdl_us_imp;
}
