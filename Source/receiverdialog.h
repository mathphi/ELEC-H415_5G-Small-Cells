#ifndef RECEIVERDIALOG_H
#define RECEIVERDIALOG_H

#include <QDialog>

#include "receiver.h"

namespace Ui {
class ReceiverDialog;
}

class ReceiverDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ReceiverDialog(QWidget *parent = nullptr);
    explicit ReceiverDialog(Receiver *em, QWidget *parent = nullptr);
    ~ReceiverDialog();

    AntennaType::AntennaType getAntennaType();
    double getEfficiency();

protected:
    virtual void keyPressEvent(QKeyEvent *event);

private:
    Ui::ReceiverDialog *ui;
};

#endif // RECEIVERDIALOG_H
