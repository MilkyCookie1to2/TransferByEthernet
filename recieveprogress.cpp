#include "recieveprogress.h"
#include "ui_recieveprogress.h"

RecieveProgress::RecieveProgress(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RecieveProgress)
{
    ui->setupUi(this);
}

RecieveProgress::~RecieveProgress()
{
    delete ui;
}

void RecieveProgress::begin_settings(char *name_file)
{
    this->recieved_bytes = 0;
    ui->recieve_file_name->setText("Recieve "+QString::fromStdString(name_file));
    ui->recieve_progress->setText("Recieved 0 bytes");
}

void RecieveProgress::set_recieve_byte(int recieved)
{
    recieved_bytes += recieved;
    int size = 0;
    QString size_text = "bytes";
    if((int)(recieved_bytes/1024)==0){
        size = recieved_bytes;
    } else {
        if((int)(recieved_bytes/1048576)==0){
            size = recieved_bytes/1024;
            size_text = "Kbytes";
        } else {
            if((int)(recieved_bytes/1073741824)==0){
                size = recieved_bytes/1048576;
                size_text = "Mbytes";
            } else {
                size =recieved_bytes/1073741824;
                size_text = "Gbytes";
            }
        }
    }
    ui->recieve_progress->setText("Recieved "+QString::number(size)+" "+size_text);
}

void RecieveProgress::reject()
{
    QDialog::reject();
}
