#ifndef RECIEVEPROGRESS_H
#define RECIEVEPROGRESS_H

#include <QDialog>

namespace Ui {
class RecieveProgress;
}

class RecieveProgress : public QDialog
{
    Q_OBJECT

public:
    explicit RecieveProgress(QWidget *parent = nullptr);
    ~RecieveProgress();

signals:
    void cancel_recieve_signal();

public slots:
    void begin_settings(char* );
    void set_recieve_byte(int );
    void reject();

private:
    Ui::RecieveProgress *ui;
    int recieved_bytes;
};

#endif // RECIEVEPROGRESS_H
