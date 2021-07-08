#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QMessageBox>
#include <QDomDocument>
#include <QTimer>
#include <QStringListModel>
#include <QFontDatabase>
#include <QFile>
#include <QListWidgetItem>
#include <QModelIndexList>
#include <QFileDialog>

class bookingOnPoint;
namespace telemeteryServices { class pop3EmailGateway; }

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    void aboutToQuit();
    ~MainWindow();

private slots:
    void on_startPollingButton_clicked();
    void on_stopPollingButton_clicked();
    void on_actionQuit_triggered();
    void on_depotList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
    //void on_depotList_currentRowChanged(int currentRow);
    void on_actionStart_Server_triggered();
    void on_actionStop_Server_triggered();
    void updateUi();
    void updateLights();
    void on_depotList_itemClicked(QListWidgetItem *item);
    void on_serverStatus_indexesMoved(const QModelIndexList &indexes);
    void on_serverStatus_clicked(const QModelIndex &index);
    void serverStatus_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles);
    void on_clearButton_clicked();
    void on_actionRestart_Server_triggered();
    void on_actionCopy_triggered();
    void on_actionClear_triggered();
    void on_actionSelect_All_triggered();
    void on_actionStart_All_Polling_triggered();
    void on_actionStop_All_Polling_triggered();
    void on_actionHalt_All_Servers_triggered();
    void on_actionSave_Log_triggered();
    void serverListMenuRequested(QPoint pos);
    void serverStatusMenuRequested(QPoint pos);

private:
    Ui::MainWindow *ui;
    QString dataDirectory;
    QFile *configFile;
    QStringList _defaultScreen;
    void loadConfiguration();
    bool _haveQuit;
    QFileDialog *saveLogFileDialog;
};
#endif // MAINWINDOW_H
