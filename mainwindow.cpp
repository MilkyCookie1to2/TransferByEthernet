#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QThread>
#include <thread>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    progress_win = new SendProgress(this);
    recieve_progress_win = new RecieveProgress(this);
    ui->table_files->setColumnCount(2);
    ui->table_files->setHorizontalHeaderItem(0, new QTableWidgetItem("Name"));
    ui->table_files->setHorizontalHeaderItem(1, new QTableWidgetItem("Size"));
    ui->table_files->setColumnWidth(1,200);
    ui->table_files->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    ui->table_files->setEditTriggers(QTableWidget::NoEditTriggers);
    ui->table_files->setSelectionMode(QAbstractItemView::NoSelection);
    ui->table_files->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->table_files, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(ProvideContextMenu(const QPoint &)));
    connect(progress_win, SIGNAL(cancel_send_signal()), this, SLOT(cancel_send()));
    connect(this, SIGNAL(send_finished()),this, SLOT(close_send_win()));
    connect(this, SIGNAL(recieve_begin_signal(char*)), this, SLOT(recieve_begin(char*)));
    connect(this, SIGNAL(recieve_end_signal()), this, SLOT(recieve_end()));

    ChooseInterface *win_ch_intr;
    win_ch_intr = new ChooseInterface();
    connect(win_ch_intr, SIGNAL(send_name_interface(QString)), this, SLOT(set_name_interface(QString)));
    win_ch_intr->setModal(true);
    win_ch_intr->exec();
    win_ch_intr->close();
    edit_line_mac = new QLineEdit();
    edit_line_mac->setInputMask("HH:HH:HH:HH:HH:HH;_");
    ui->list_of_dst_macs->setLineEdit(edit_line_mac);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_update_arp_button_clicked()
{
    ui->list_of_dst_macs->setEnabled(false);
    this->arp_table.clear();
    ui->list_of_dst_macs->clear();
    set_arp_list(this->name_interface);
    ui->list_of_dst_macs->setEnabled(true);
}

void MainWindow::set_name_interface(QString name_intreface)
{
    this->name_interface = name_intreface.toStdString().c_str();
    ui->interface_message->setText(ui->interface_message->text()+" "+name_intreface);
    set_src_mac(this->name_interface);
    set_arp_list(this->name_interface);
    this->recieve_thread = QThread::create(&MainWindow::recieve_fun, this);
    this->recieve_thread->start();
}

void MainWindow::set_src_mac(const char *interface)
{
    Interface dev(interface);
    this->src_mac = dev.get_mac();
    if(this->src_mac == NULL){
        QMessageBox message;
        message.critical(this, "Error", "Error get source MAC address.\nMake sure that application will launch with root");
        exit(0);
    }
    this->index_interface = dev.get_index();
    this->socket = new SocketEther(this->index_interface);
    QString src_mac_message = QString::number(this->src_mac[0], 16)+":"+QString::number(this->src_mac[1], 16)+":"+QString::number(this->src_mac[2], 16)
            +":"+QString::number(this->src_mac[3], 16)+":"+QString::number(this->src_mac[4], 16)+":"+QString::number(this->src_mac[5], 16);
    ui->src_mac_address->setText(ui->src_mac_address->text()+" "+src_mac_message.toUpper());
}

void MainWindow::set_arp_list(const char *interface)
{
    ARPTable arps(interface);
    if(arps.get_ARP_table(this->arp_table) == -1){
        return;
    }
    for(unsigned char* elem : this->arp_table){
        QString dst_mac_message = QString::number(elem[0], 16)+":"+QString::number(elem[1], 16)+":"+QString::number(elem[2], 16)
                +":"+QString::number(elem[3], 16)+":"+QString::number(elem[4], 16)+":"+QString::number(elem[5], 16);
        ui->list_of_dst_macs->addItem(dst_mac_message.toUpper());
    }
}




void MainWindow::on_add_file_button_clicked()
{
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog-> setWindowTitle (tr ("Select files to send"));
    fileDialog->setDirectory("~");
    //fileDialog->setNameFilter(tr("Images(*.png *.jpg *.jpeg *.bmp)"));
    // Настройка позволяет выбрать несколько файлов, по умолчанию используется только один файл QFileDialog :: ExistingFiles
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    fileDialog->setViewMode(QFileDialog::Detail);
    QStringList fileNames;
    if(fileDialog->exec())
    {
        fileNames = fileDialog->selectedFiles();
    }
    files_to_send += fileNames;
    for(QString file : fileNames) {
        QFileInfo file_info(file);
        if(file_info.exists())
        {
            ui->table_files->insertRow(ui->table_files->rowCount());
            ui->table_files->setItem(ui->table_files->rowCount()-1, 0, new QTableWidgetItem(file_info.fileName()));
            int size = 0;
            QString size_text = "байт";
            if((int)(file_info.size()/1024)==0){
                size = file_info.size();
            } else {
                if((int)(file_info.size()/1048576)==0){
                    size = file_info.size()/1024;
                    size_text = "Кбайт";
                } else {
                    if((int)(file_info.size()/1073741824)==0){
                        size = file_info.size()/1048576;
                        size_text = "Мбайт";
                    } else {
                        size =file_info.size()/1073741824;
                        size_text = "Гбайт";
                    }
                }
            }
            ui->table_files->setItem(ui->table_files->rowCount()-1, 1, new QTableWidgetItem(QString::number(size)+" "+size_text));
        }
    }
}

void MainWindow::ProvideContextMenu(const QPoint &pos)
{
    if(ui->table_files->indexAt(pos).data().isValid())
    {
        QMenu submenu;
        submenu.addAction("Delete");
        QPoint item = ui->table_files->mapToGlobal(pos);
        QAction* rightClickItem = submenu.exec(item);
        if (rightClickItem && rightClickItem->text().contains("Delete") )
        {
            files_to_send.remove(ui->table_files->indexAt(pos).row());
            ui->table_files->removeRow(ui->table_files->indexAt(pos).row());
        }
    }
}


void MainWindow::on_send_button_clicked()
{
    if(files_to_send.empty()){
        QMessageBox message;
        message.critical(0, "Error", "Nothing to send");
        return;
    }
    for(QString file : files_to_send){
        QFileInfo file_info(file);
        if(!file_info.exists()){
            QMessageBox message;
            message.critical(0, "Error", "File "+file_info.fileName() + " doesn't exist");
            return;
        }
    }
    //std::cout << ui->list_of_dst_macs->currentText().toStdString() << std::endl;
    QStringList parts_dst_mac = ui->list_of_dst_macs->currentText().split(":");
    int i=0;
    for(QString part: parts_dst_mac){
        if(part.isEmpty()){
            QMessageBox message;
            message.critical(0, "Error", "Invalid destination MAC address");
            return;
        } else {
            bool ok;
            int number = part.toInt(&ok,16);
            if(ok){
                dst_mac[i] = number;
                i++;
            } else {
                QMessageBox message;
                message.critical(0, "Error", "Invalid destination MAC address");
                return;
            }

        }
    }
    //printf("\ndada %X:%X:%X:%X:%X:%X", dst_mac[0],dst_mac[1],dst_mac[2],dst_mac[3],dst_mac[4],dst_mac[5]);
    recieve_thread->terminate();
    this->setEnabled(false);
    progress_win->show();
    progress_win->setEnabled(true);
    send_thread = QThread::create(&MainWindow::send_fun, this);
    send_thread->start();
}

void MainWindow::send_fun()
{

    for(QString file: files_to_send){
        QFileInfo file_info(file);
        if(!file_info.exists()){
            QMessageBox message;
            message.critical(0, "Error", "File "+file_info.fileName() + " doesn't exist");
            break;
        }
        progress_win->set_begin_settings(file_info.fileName());
        if(socket->send_file(this->src_mac, this->dst_mac, (char*)(file.toStdString().c_str()), progress_win)==-1){
            QMessageBox message;
            message.critical(0, "Error", "Unknow error");
            break;
        }
    }
    this->setEnabled(true);
    emit send_finished();
}

void MainWindow::cancel_send()
{
    send_thread->terminate();
    this->setEnabled(true);
    this->recieve_thread = QThread::create(&MainWindow::recieve_fun, this);
    this->recieve_thread->start();
}

void MainWindow::close_send_win()
{
    progress_win->close();
    this->recieve_thread = QThread::create(&MainWindow::recieve_fun, this);
    this->recieve_thread->start();
}

void MainWindow::recieve_fun()
{
    while(1){
        unsigned char* get_request = socket->listen(this->src_mac);
        this->setEnabled(false);
        char *name_file = (char *) calloc(1, sizeof(char));
        strcat(name_file, (char *) get_request + 24);
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Request", "Accept request from "+QString::number(get_request[6],16).toUpper()+":"+QString::number(get_request[7],16).toUpper()+":"+QString::number(get_request[8],16).toUpper()
                +":"+QString::number(get_request[9],16).toUpper()+":"+QString::number(get_request[10],16).toUpper()+":"+QString::number(get_request[11],16).toUpper()+" which send "+name_file,QMessageBox::Yes|QMessageBox::No);
        if(reply == QMessageBox::Yes){
            emit recieve_begin_signal(name_file);
            if(socket->receive_file(get_request, src_mac, (unsigned char *) "YES", name_file, recieve_progress_win)==0) {
                puts("File was successfully received");
                QMessageBox message;
                message.information(this,"Success", "File was successfully received");
            } else {
                QMessageBox message;
                message.critical(this, "Error", "Unknow error");
            }
            emit recieve_end_signal();
            this->setEnabled(true);
        } else {
            socket->receive_file(get_request, src_mac, (unsigned char *) "NO", name_file, recieve_progress_win);
            this->setEnabled(true);
        }
    }
}

void MainWindow::recieve_begin(char *name_file)
{
    recieve_progress_win->setEnabled(true);
    recieve_progress_win->begin_settings(name_file);
    recieve_progress_win->show();
}

void MainWindow::recieve_end()
{
    recieve_progress_win->close();
}


