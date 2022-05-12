#include "sendprogress.h"
#include "ui_sendprogress.h"

SendProgress::SendProgress(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SendProgress)
{
    ui->setupUi(this);
    ui->progressBar->setHidden(true);
    ui->progressBar->setValue(0);
    ui->request_label->setText("Waiting accept request...");
}

SendProgress::~SendProgress()
{
    delete ui;
}

void SendProgress::set_begin_settings(QString sending_file)
{
    ui->progressBar->setHidden(true);
    ui->request_label->setHidden(false);
    ui->file_name_label->setText("Sending "+sending_file);
    ui->request_label->setText("Waiting accept request...");
}



void SendProgress::set_progress(int progress)
{
    ui->progressBar->setValue(ui->progressBar->value()+progress);
}

void SendProgress::set_range(int size)
{
    ui->request_label->setHidden(true);
    ui->progressBar->setHidden(false);
    ui->progressBar->setRange(0, size);
}

void SendProgress::reject()
{
    emit cancel_send_signal();
    QDialog::reject();
}
