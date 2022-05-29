#ifndef CHOOSEINTERFACE_H
#define CHOOSEINTERFACE_H

#include <QDialog>

namespace Ui {
class ChooseInterface;
}

class ChooseInterface : public QDialog
{
    Q_OBJECT

public:
    explicit ChooseInterface(QWidget *parent = nullptr);
    ~ChooseInterface();

signals:
    void send_name_interface(QString );

private slots:
    void on_exit_button_clicked();

    void on_ok_button_clicked();

    void reject();

private:
    Ui::ChooseInterface *ui;
    bool no_exit;
};

#endif // CHOOSEINTERFACE_H
