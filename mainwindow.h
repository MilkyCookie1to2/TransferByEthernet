#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ARPTable.h"
#include "SocketEther.h"
#include "Interface.h"
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include "sendprogress.h"
#include "chooseinterface.h"
#include <QProgressBar>
#include "recieveprogress.h"
#include <QMutex>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void send_finished();
    void recieve_begin_signal(char*);
    void recieve_end_signal();


private slots:
    void on_update_arp_button_clicked();

    void set_name_interface(QString);
    void set_src_mac(const char*);
    void set_arp_list(const char*);

    void on_add_file_button_clicked();

    void ProvideContextMenu(const QPoint &);

    void on_send_button_clicked();

    void send_fun();

    void cancel_send();

    void close_send_win();

    void recieve_fun();

    void recieve_begin(char*);
    void recieve_end();

private:
    Ui::MainWindow *ui;
    const char* name_interface;
    unsigned char* src_mac;
    unsigned char dst_mac[6];
    int index_interface;
    std::vector<unsigned char *> arp_table;
    QLineEdit *edit_line_mac;
    QStringList files_to_send;
    SocketEther *socket;
    SendProgress *progress_win;
    RecieveProgress *recieve_progress_win;
    QThread *send_thread;
    QThread *recieve_thread;
};
#endif // MAINWINDOW_H
