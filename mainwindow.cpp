#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QThread>
#include <thread>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("TransferByEthernet");
    progress_win = new SendProgress(this);
    recieve_progress_win = new RecieveProgress(this);
    ui->table_files->setColumnCount(2);
    ui->table_files->setHorizontalHeaderItem(0, new QTableWidgetItem("Name"));
    ui->table_files->setHorizontalHeaderItem(1, new QTableWidgetItem("Size"));
    ui->table_files->setColumnWidth(1,200);
    ui->table_files->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Stretch);
    ui->table_files->setEditTriggers(QTableWidget::NoEditTriggers);
    ui->table_files->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->table_files->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->table_files->setDragEnabled(true);
    ui->table_files->setDragDropMode(QAbstractItemView::DragDrop);
    setAcceptDrops(true);

    connect(ui->table_files, SIGNAL(customContextMenuRequested(const QPoint &)),
            this, SLOT(ProvideContextMenu(const QPoint &)));
    connect(progress_win, SIGNAL(cancel_send_signal()), this, SLOT(cancel_send()));
    connect(this, SIGNAL(send_finished()),this, SLOT(close_send_win()));
    connect(this, SIGNAL(recieve_begin_signal(char*)), this, SLOT(recieve_begin(char*)));
    connect(this, SIGNAL(recieve_end_signal()), this, SLOT(recieve_end()));
    connect(this, SIGNAL(answer_request_signal(unsigned char*)), this, SLOT(answer_request(unsigned char*)));
    connect(recieve_progress_win, SIGNAL(cancel_recieve_signal()), this, SLOT(cancel_recieve()));
    connect(this, SIGNAL(set_begin_settings_signal(QString)), progress_win, SLOT(set_begin_settings(QString)));
    connect(this, SIGNAL(print_message_signal(int, QString)), this, SLOT(print_message(int, QString)));

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

void MainWindow::set_name_interface(QString name_intreface)
{
    this->name_interface = name_intreface.toStdString().c_str();
    ui->interface_message->setText(ui->interface_message->text()+" "+name_intreface);
    set_src_mac(this->name_interface);
    set_arp_list(this->name_interface);
    this->listen_thread = QThread::create(&MainWindow::listen_fun, this);
    this->listen_thread->start();
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
    connect(socket, SIGNAL(set_range_signal(int)), progress_win, SLOT(set_range(int)));
    connect(socket, SIGNAL(set_progress_send_signal(int)), progress_win, SLOT(set_progress(int)));
    connect(socket, SIGNAL(set_progress_recieve_signal(int)), recieve_progress_win, SLOT(set_recieve_byte(int)));

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
            float size = 0;
            QString size_text = "bytes";
            if((int)(file_info.size()/1024)==0){
                size = file_info.size();
            } else {
                if((int)(file_info.size()/1048576)==0){
                    size = file_info.size()/1024;
                    size_text = "Kbytes";
                } else {
                    if((int)(file_info.size()/1073741824)==0){
                        size = file_info.size()/1048576;
                        size_text = "Mbytes";
                    } else {
                        size =file_info.size()/1073741824;
                        size_text = "Gbytes";
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
            bool repeater = false;
            for(QTableWidgetItem *item : ui->table_files->selectedItems()){
                if(!repeater){
                    files_to_send.remove(item->row());
                    ui->table_files->removeRow(item->row());
                    repeater = true;
                } else
                    repeater = false;
            }
        }
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    qDebug()<<e->position().ry();
    qDebug()<<e->mimeData()->formats();
    if (e->mimeData()->hasUrls() && e->position().rx()>ui->table_files->x() && e->position().rx()<(ui->table_files->size().width()+ui->table_files->x())
            && e->position().ry()>ui->table_files->y() && e->position().ry()<(ui->table_files->y()+ui->table_files->size().height())) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    foreach (const QUrl &url, e->mimeData()->urls()) {
        QString fileName = url.toLocalFile();
        qDebug() << "Dropped file:" << fileName;
        QFileInfo file_info;
        file_info.setFile(url.toLocalFile());
        qDebug()<<file_info.fileName();
        if(file_info.exists() && file_info.isFile())
        {
            files_to_send+=fileName;
            ui->table_files->insertRow(ui->table_files->rowCount());
            ui->table_files->setItem(ui->table_files->rowCount()-1, 0, new QTableWidgetItem(file_info.fileName()));
            float size = 0;
            QString size_text = "bytes";
            if((int)(file_info.size()/1024)==0){
                size = file_info.size();
            } else {
                if((int)(file_info.size()/1048576)==0){
                    size = file_info.size()/1024;
                    size_text = "Kbytes";
                } else {
                    if((int)(file_info.size()/1073741824)==0){
                        size = file_info.size()/1048576;
                        size_text = "Mbytes";
                    } else {
                        size =file_info.size()/1073741824;
                        size_text = "Gbytes";
                    }
                }
            }
            ui->table_files->setItem(ui->table_files->rowCount()-1, 1, new QTableWidgetItem(QString::number(size)+" "+size_text));
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
    listen_thread->terminate();
    while(!listen_thread->isFinished()){}
    this->setEnabled(false);
    progress_win->show();
    progress_win->setEnabled(true);
    send_thread = QThread::create(&MainWindow::send_fun, this);
    send_thread->start();
}

void MainWindow::send_fun()
{
    abort_flag = false;
    for(QString file: files_to_send){
        QFileInfo file_info(file);
        if(!file_info.exists()){
            QMessageBox message;
            message.critical(0, "Error", "File "+file_info.fileName() + " doesn't exist");
            emit send_finished();
            return;
        }
        emit set_begin_settings_signal(file_info.fileName());
        int status = socket->send_file(this->src_mac, this->dst_mac, (char*)(file.toStdString().c_str()));
        if(status == UNKNOWN_ERROR){
            emit print_message_signal(UNKNOWN_ERROR, "send");
            emit send_finished();
            return;
        }
        if(status == REQUEST_NOT_ACCEPT){
            emit print_message_signal(REQUEST_NOT_ACCEPT, "send");
            abort_flag = true;
            emit send_finished();
            return;
        }
        if(status == RESPONSE_WAITING_TIME_OUT){
            emit print_message_signal(RESPONSE_WAITING_TIME_OUT, "send");
            emit send_finished();
            return;
        }
        if(status == ABORT){
            emit print_message_signal(ABORT, "send");
            abort_flag = true;
            emit send_finished();
            return;
        }
    }
    socket->send_message(src_mac, dst_mac, (char*)"END_FILES");
    emit send_finished();
}

void MainWindow::cancel_send()
{
    if(send_thread->isRunning()){
        send_thread->terminate();
        while(!send_thread->isFinished()){}
        if(!abort_flag)
            socket->send_message(this->src_mac, this->dst_mac, (char*)"ABORT");
    }
    if(!this->isEnabled())
        this->setEnabled(true);
    if(!listen_thread->isRunning()){
        socket->clear_socket();
        this->listen_thread = QThread::create(&MainWindow::listen_fun, this);
        this->listen_thread->start();
    }
}

void MainWindow::cancel_recieve()
{
    if(recieve_thread->isRunning()){
        recieve_thread->terminate();
        while(!recieve_thread->isFinished()){}
        if(!abort_flag)
            socket->send_message(this->src_mac, this->dst_mac, (char*)"ABORT");
    }
    if(!this->isEnabled())
        this->setEnabled(true);
    if(!listen_thread->isRunning()){
        socket->clear_socket();
        this->listen_thread = QThread::create(&MainWindow::listen_fun, this);
        this->listen_thread->start();
    }
}

void MainWindow::close_send_win()
{
    progress_win->close();
}

void MainWindow::recieve_fun()
{
    while(1){
        abort_flag = false;
        int status = socket->receive_file(src_mac, dst_mac, name_file);
        if(status == UNKNOWN_ERROR){
            emit print_message_signal(UNKNOWN_ERROR, "recieve");
            break;
        }
        if(status == RESPONSE_WAITING_TIME_OUT){
            emit print_message_signal(RESPONSE_WAITING_TIME_OUT, "recieve");
            break;
        }
        if(status == ABORT){
            emit print_message_signal(ABORT, "recieve");
            break;
        }
        if(status == ERROR_CREATE_FILE){
            emit print_message_signal(ERROR_CREATE_FILE, "recieve");
            break;
        }

        unsigned char* answer = socket->listen(src_mac);//remake
        char* answer_text = (char *) calloc(strlen((char*)(answer+14))+1, sizeof(char));
        memcpy(answer_text, (char*)(answer+24), strlen((char*)(answer+24))+1);
        if(QString::fromStdString(answer_text)=="ABORT"){
            emit print_message_signal(ABORT, "recieve");
            break;
        }
        if(QString::fromStdString(answer_text) == ""){
            emit print_message_signal(SUCCESS, "recieve");
            break;
        }
        name_file = (char *) calloc(strlen((char*)(answer+24))+1, sizeof(char));
        memcpy(name_file, (char*)(answer+24), strlen((char*)(answer+24))+1);
        emit recieve_begin_signal(name_file);
        socket->send_message(src_mac, dst_mac, (char*)"YES");
    }
    emit recieve_end_signal();
}

void MainWindow::listen_fun()
{
    unsigned char* get_request;
    while(1){
        get_request = socket->listen(this->src_mac);
        unsigned char* data = get_request + 14;
        if(memcmp(data,"YES",3)!=0 && memcmp(data,"NO",3)!=0)
            break;
    }
    emit answer_request_signal(get_request);
}

void MainWindow::recieve_begin(char *name_file)
{
    recieve_progress_win->begin_settings(name_file);
}

void MainWindow::recieve_end()
{
    recieve_progress_win->close();
}

void MainWindow::answer_request(unsigned char* get_request)
{
    this->setEnabled(false);
    memcpy( dst_mac, get_request+ETH_ALEN, ETH_ALEN);
    name_file = (char *) calloc(strlen((char*)(get_request+24))+1, sizeof(char));
    memcpy(name_file, (char*)(get_request+24), strlen((char*)(get_request+24))+1);
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(0, "Request", "Accept request from "+QString::number(get_request[6],16).toUpper()+":"+QString::number(get_request[7],16).toUpper()+":"+QString::number(get_request[8],16).toUpper()
            +":"+QString::number(get_request[9],16).toUpper()+":"+QString::number(get_request[10],16).toUpper()+":"+QString::number(get_request[11],16).toUpper()+" which send "+ QString::fromStdString(name_file),QMessageBox::Yes|QMessageBox::No);
    if(reply == QMessageBox::Yes){
        recieve_progress_win->setEnabled(true);
        recieve_progress_win->begin_settings(name_file);
        recieve_progress_win->show();
        socket->send_message(src_mac, dst_mac, (char*)"YES");
        this->recieve_thread = QThread::create(&MainWindow::recieve_fun, this);
        this->recieve_thread->start();
    }else{
        socket->send_message(src_mac, dst_mac, (char*)"NO");
        recieve_progress_win->close();
        if(!this->isEnabled())
            this->setEnabled(true);
        if(!listen_thread->isRunning()){
            this->listen_thread = QThread::create(&MainWindow::listen_fun, this);
            this->listen_thread->start();
        }
    }
}

void MainWindow::print_message(int status, QString operation)
{
    if(status == UNKNOWN_ERROR){
        QMessageBox message;
        message.critical(0, "Error", "Unknow error");
    }
    if(status == RESPONSE_WAITING_TIME_OUT){
        QMessageBox message;
        message.critical(0, "Error", "Response waiting time out");
    }
    if(status == ABORT){
        QMessageBox message;
        message.warning(0, "Warning", "Sending process was abort");
        abort_flag = true;
    }
    if(status == ERROR_CREATE_FILE){
        QMessageBox message;
        message.critical(0, "Error", "Error while creating file");
    }
    if(status == REQUEST_NOT_ACCEPT){
        QMessageBox message;
        message.warning(0, "Warning", "Request wasn't accept");
        abort_flag = true;
        emit send_finished();
        return;
    }
    if(status == SUCCESS){
        QMessageBox message;
        message.information(0, "Success", "Files was successfully recieved");
    }
    if(operation == "send"){
        emit send_finished();
    }
    if(operation == "recieve"){
        emit recieve_end_signal();
    }
}




