#ifndef SENDPROGRESS_H
#define SENDPROGRESS_H

#include <QDialog>

namespace Ui {
class SendProgress;
}

class SendProgress : public QDialog
{
    Q_OBJECT

    friend class MainWindow;

public:
    explicit SendProgress(QWidget *parent = nullptr);
    ~SendProgress();

    void set_begin_settings(QString);

signals:
    void cancel_send_signal();

public slots:
    void set_progress(int );
    void set_range(int);
    void reject();

private:
    Ui::SendProgress *ui;
};

#endif // SENDPROGRESS_H
