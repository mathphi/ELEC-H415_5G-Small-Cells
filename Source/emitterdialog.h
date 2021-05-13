#ifndef EMITTERDIALOG_H
#define EMITTERDIALOG_H

#include <QDialog>

#include "emitter.h"

namespace Ui {
class EmitterDialog;
}

class EmitterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EmitterDialog(QWidget *parent = nullptr);
    explicit EmitterDialog(Emitter *em, QWidget *parent = nullptr);
    ~EmitterDialog();

    AntennaType::AntennaType getAntennaType();
    double getEIRP();
    double getFrequency();
    double getEfficiency();

protected:
    virtual void keyPressEvent(QKeyEvent *event);

private slots:
    void emitterConfigurationChanged();

private:
    Ui::EmitterDialog *ui;
};

#endif // EMITTERDIALOG_H
