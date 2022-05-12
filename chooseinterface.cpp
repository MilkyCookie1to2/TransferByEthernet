#include "chooseinterface.h"
#include "ui_chooseinterface.h"
#include "ListInterfaces.h"

ChooseInterface::ChooseInterface(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChooseInterface)
{
    ui->setupUi(this);
    ListInterfaces table_interface;
    std::vector<const char*> list = table_interface.getListInterfaces();
    if(!list.empty()) {
        for(const char* elem : list)
        {
            if(strstr(elem, "e"))
                ui->list_interface->addItem(QString::fromStdString(elem));
        }
    }
}

ChooseInterface::~ChooseInterface()
{
    delete ui;
}


void ChooseInterface::on_exit_button_clicked()
{
    exit(0);
}


void ChooseInterface::on_ok_button_clicked()
{
    QString name_interface = ui->list_interface->currentText();
    emit send_name_interface(name_interface);
    close();
}

