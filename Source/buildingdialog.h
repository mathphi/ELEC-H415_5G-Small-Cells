#ifndef BUILDINGDIALOG_H
#define BUILDINGDIALOG_H

#include <QDialog>

namespace Ui {
class BuildingDialog;
}

class BuildingDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BuildingDialog(QWidget *parent = nullptr);
    ~BuildingDialog();

    QSize getBuildingSize();

protected:
    virtual void keyPressEvent(QKeyEvent *event);

private:
    Ui::BuildingDialog *ui;
};

#endif // BUILDINGDIALOG_H
