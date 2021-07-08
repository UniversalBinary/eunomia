#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "bookingOnPoint.hpp"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _haveQuit = false;
    configFile = nullptr;

    connect(ui->actionStart_Polling, &QAction::triggered, this, &MainWindow::on_startPollingButton_clicked);
    connect(ui->actionStop_Polling, &QAction::triggered, this, &MainWindow::on_stopPollingButton_clicked);
    connect(ui->menuFile, &QMenu::aboutToShow, this, &MainWindow::updateUi);
    connect(ui->depotList, &QListWidget::itemDoubleClicked, this, &MainWindow::on_depotList_itemClicked);
    connect(ui->depotList, &QListWidget::itemActivated, this, &MainWindow::on_depotList_itemClicked);
    connect(ui->depotList, &QListWidget::itemPressed, this, &MainWindow::on_depotList_itemClicked);
    connect(ui->serverStatus, &QListView::doubleClicked, this, &MainWindow::on_serverStatus_clicked);
    connect(ui->serverStatus, &QListView::activated, this, &MainWindow::on_serverStatus_clicked);
    connect(ui->serverStatus, &QListView::pressed, this, &MainWindow::on_serverStatus_clicked);
    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::aboutToQuit);
    ui->depotList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->depotList, SIGNAL(customContextMenuRequested(QPoint)), SLOT(serverListMenuRequested(QPoint)));
    ui->serverStatus->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->serverStatus, SIGNAL(customContextMenuRequested(QPoint)), SLOT(serverStatusMenuRequested(QPoint)));

    int id = QFontDatabase::addApplicationFont("://fonts/SourceCodePro-SemiBold.ttf");
    QString family = QFontDatabase::applicationFontFamilies(id).at(0);
    QFont monospace(family);
    ui->serverStatus->setFont(monospace);
    ui->serverStatus->setStyleSheet("background-color: black; color: green; selection-background-color: green; selection-color: black");
    QStringListModel *model = new QStringListModel();
    _defaultScreen << "Schedule Notification Utility version 1.0.5" << "Copyright (c) 2021 Chris Morrison" << "";
    _defaultScreen << "This program is free software: you can redistribute it and/or modify";
    _defaultScreen << "it under the terms of the GNU General Public License as published by";
    _defaultScreen << "the Free Software Foundation, either version 3 of the License, or";
    _defaultScreen << "(at your option) any later version.";
    _defaultScreen << "";
    _defaultScreen << "This program is distributed in the hope that it will be useful,";
    _defaultScreen << "but WITHOUT ANY WARRANTY; without even the implied warranty of";
    _defaultScreen << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the";
    _defaultScreen << "GNU General Public License for more details.";
    _defaultScreen << "";
    _defaultScreen << "You should have received a copy of the GNU General Public License";
    _defaultScreen << "along with this program.  If not, see <http://www.gnu.org/licenses/>.";
    model->setStringList(_defaultScreen);
    connect(model, &QStringListModel::modelReset, this, &MainWindow::updateUi);
    connect(model, &QStringListModel::dataChanged, this, &MainWindow::serverStatus_dataChanged);
    ui->serverStatus->setModel(model);

    saveLogFileDialog = new QFileDialog(this, Qt::Dialog);
    saveLogFileDialog->setAcceptMode(QFileDialog::AcceptSave);
    saveLogFileDialog->setFileMode(QFileDialog::AnyFile);
    saveLogFileDialog->setDirectory(QDir::homePath());
    saveLogFileDialog->setNameFilter("Log Files (*.log)");
    saveLogFileDialog->setWindowTitle("Save Log File");
    connect(saveLogFileDialog, &QFileDialog::fileSelected, ui->serverStatus, &ServerStatusTerminal::saveToFile);
    saveLogFileDialog->setModal(true);
    saveLogFileDialog->setDefaultSuffix("log");

    QTimer::singleShot(500, this, &MainWindow::loadConfiguration);
}

void MainWindow::aboutToQuit()
{
    auto dc = ui->depotList->count();
    for (int i = 0; i < dc; i++)
    {
        bookingOnPoint *itm = dynamic_cast<bookingOnPoint *>(ui->depotList->item(i));
        itm->stopScanners();
        QCoreApplication::processEvents();
    }
    _haveQuit = true;
}

void MainWindow::loadConfiguration()
{
    int bopCount = 0;
    int successfulLoads = 0;
    dataDirectory = QDir::cleanPath(QDir::homePath() + QDir::separator() + ".eunomia");
    configFile = new QFile(QDir::cleanPath(dataDirectory + QDir::separator() + "config.xml"));
    if (!configFile->open(QFile::Text | QFile::ReadWrite))
    {
        QMessageBox::critical(this, this->windowTitle(), "Failed to open config.xml file: " + configFile->errorString());
        QCoreApplication::quit();
        return;
    }

    QDomDocument domDocument("config");
    if (!domDocument.setContent(configFile))
    {
        QMessageBox::critical(this, this->windowTitle(), "An error occurred while processing the configuration file. It may be corrupt, damaged or incomplete.");
        QCoreApplication::quit();
        return;
    }
    auto docElement = domDocument.documentElement();
    auto bopElements = docElement.elementsByTagName("booking-on-point");
    if (bopElements.isEmpty())
    {
        QMessageBox::critical(this, this->windowTitle(), "An error occurred while processing the configuration file. It may be malformed, damaged or incomplete.");
        QCoreApplication::quit();
        return;
    }

    bopCount = bopElements.count();

    for (int idx1 = 0; idx1 < bopCount; idx1++)
    {
        auto cNode = bopElements.at(idx1).namedItem("company");
        auto nNode = bopElements.at(idx1).namedItem("name");
        auto rNode = bopElements.at(idx1).namedItem("line");
        auto ouNode = bopElements.at(idx1).namedItem("organisational-unit");

        if (cNode.isNull() || nNode.isNull() || rNode.isNull() || ouNode.isNull()) continue;

        auto company = cNode.toElement().text();
        auto name = nNode.toElement().text();
        auto line = rNode.toElement().text();
        auto orgu = ouNode.toElement().text();

        if (company.isEmpty() || name.isEmpty() || line.isEmpty() || orgu.isEmpty()) continue;

        auto depotPtr = std::make_unique<bookingOnPoint>(company, name, orgu, line);

        // Look for an email configuration.
        auto telemNode = bopElements.at(idx1).namedItem("telemetry");
        if (telemNode.isNull() || telemNode.childNodes().isEmpty())
        {
            QMessageBox::critical(this, this->windowTitle(), "An error occurred while processing the configuration file. It may be malformed, damaged or incomplete.");
            QCoreApplication::quit();
            return;
        }

        for (int idx2 = 0; idx2 < telemNode.childNodes().count(); idx2++)
        {
            auto sNode = telemNode.childNodes().at(idx2);
            if (sNode.nodeName() == "mail-scanner")
            {
                auto pNode = sNode.namedItem("protocol");
                if (pNode.isNull())
                {
                    QMessageBox::warning(this, this->windowTitle(), "The configuration file has missing information. It may be malformed, damaged or incomplete.");
                    continue;
                }
                auto proto = pNode.toElement().text();
                if ((proto.compare("pop3", Qt::CaseInsensitive) == 0) || (proto.compare("pop3s", Qt::CaseInsensitive) == 0))
                {
                    //auto ptr = std::make_unique<telemeteryServices::pop3EmailGateway>(); // TODO: Implement POP3.
                    //depotPtr->parseAndAddMailGateway(sNode, ptr);
                }
                else if ((proto.compare("IMAP", Qt::CaseInsensitive) == 0) || (proto.compare("IMAPS", Qt::CaseInsensitive) == 0))
                {
                    auto ptr = std::make_unique<telemeteryServices::imapEmailGateway>();
                    depotPtr->parseAndAddMailGateway(sNode, ptr);
                }
                else
                {
                    QMessageBox::warning(this, this->windowTitle(), "Unknown or unsupported email protocol specified in configuration file.");
                    continue;
                }
            }
        }

        if (depotPtr->scanners().size() == 0)
        {
            depotPtr->setState(DepotServerState::InvalidConfiguration);
        }
        else
        {
            depotPtr->setState(DepotServerState::Stopped);
            successfulLoads++;
        }

        // Add the depot object to the list
        ui->depotList->addItem(depotPtr.release());
    }

    configFile->close();
    updateUi();

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateLights);
    timer->start(500);
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_startPollingButton_clicked()
{
    ui->actionStart_Polling->setEnabled(false);
    ui->startPollingButton->setEnabled(false);
    auto currentDepot = dynamic_cast<bookingOnPoint *>(ui->depotList->currentItem());
    if (currentDepot) currentDepot->startPollingAsync();
    updateUi();
}

void MainWindow::on_stopPollingButton_clicked()
{
    ui->actionStop_Polling->setEnabled(false);
    ui->stopPollingButton->setEnabled(false);
    auto currentDepot = dynamic_cast<bookingOnPoint *>(ui->depotList->currentItem());
    if (currentDepot) currentDepot->stopPolling();
    updateUi();
}

void MainWindow::on_actionQuit_triggered()
{
    qApp->quit();
}

void MainWindow::on_depotList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    auto prev = dynamic_cast<bookingOnPoint *>(previous);
    if (prev) prev->disclaimModel();
    auto cur = dynamic_cast<bookingOnPoint *>(current);
    QStringListModel *model = dynamic_cast<QStringListModel *>(ui->serverStatus->model());
    if (cur)
    {
        cur->claimListModel(model);
    }
    else
    {
        model->setStringList(_defaultScreen);
    }
    updateUi();
}

void MainWindow::updateUi()
{
    if (_haveQuit) return;

    QStringListModel *model = dynamic_cast<QStringListModel *>(ui->serverStatus->model());
    auto currentDepot = dynamic_cast<bookingOnPoint *>(ui->depotList->currentItem());

    bool allInvalid = ui->depotList->allInvalid();
    bool anyInvalid = ui->depotList->anyInvalid();
    bool allStopped = ui->depotList->allStopped();
    bool anyStopped = ui->depotList->anyStopped();
    bool allStarted = ui->depotList->allStarted();
    bool anyStarted = ui->depotList->anyStarted();
    bool allPolling = ui->depotList->allPolling();
    bool anyPolling = ui->depotList->anyPolling();
    ui->actionRestart_Server->setEnabled(anyStarted || anyPolling);

    ui->actionStart_All_Polling->setEnabled(!allPolling && !allInvalid);
    ui->actionStop_All_Polling->setEnabled(anyPolling && !allInvalid);
    ui->actionHalt_All_Servers->setEnabled((anyStarted || anyPolling) && !allInvalid);

    ui->clearButton->setEnabled(ui->serverStatus->canClear());
    ui->actionCut->setEnabled(ui->serverStatus->canCut());
    ui->actionPaste->setEnabled(ui->serverStatus->canPaste());
    ui->actionCopy->setEnabled(ui->serverStatus->canCopy());
    ui->actionClear->setEnabled(ui->serverStatus->canClear());
    ui->actionSelect_All->setEnabled(ui->serverStatus->canSelectAll());
    ui->actionSave_Log->setEnabled(ui->serverStatus->canSave());

    if (currentDepot)
    {
        switch (currentDepot->state())
        {
        case DepotServerState::InvalidConfiguration:
            currentDepot->setIcon(QIcon("://images/Off_Light.svg"));
            currentDepot->setToolTip("Invalid server configuration");
            ui->startPollingButton->setEnabled(false);
            ui->actionStart_Polling->setEnabled(false);
            ui->stopPollingButton->setEnabled(false);
            ui->actionStop_Polling->setEnabled(false);
            ui->actionStart_Server->setEnabled(false);
            ui->actionStop_Server->setEnabled(false);
            break;
        case DepotServerState::Stopped:
            currentDepot->setIcon(QIcon("://images/Red_Light.svg"));
            currentDepot->setToolTip("Stopped");
            ui->startPollingButton->setEnabled(false);
            ui->actionStart_Polling->setEnabled(false);
            ui->stopPollingButton->setEnabled(false);
            ui->actionStop_Polling->setEnabled(false);
            ui->actionStart_Server->setEnabled(true);
            ui->actionStop_Server->setEnabled(false);
            break;
        case DepotServerState::Started:
            currentDepot->setIcon(QIcon("://images/Yellow_Light.svg"));
            currentDepot->setToolTip("Running");
            ui->startPollingButton->setEnabled(true);
            ui->actionStart_Polling->setEnabled(true);
            ui->stopPollingButton->setEnabled(false);
            ui->actionStop_Polling->setEnabled(false);
            ui->actionStart_Server->setEnabled(false);
            ui->actionStop_Server->setEnabled(true);
            break;
        case DepotServerState::Polling:
            currentDepot->setIcon(QIcon("://images/Green_Light.svg"));
            currentDepot->setToolTip("Polling");
            ui->startPollingButton->setEnabled(false);
            ui->actionStart_Polling->setEnabled(false);
            ui->stopPollingButton->setEnabled(true);
            ui->actionStop_Polling->setEnabled(true);
            ui->actionStart_Server->setEnabled(false);
            ui->actionStop_Server->setEnabled(true);
            break;
        case DepotServerState::NetworkIssue:
            currentDepot->setIcon(QIcon("://images/Red_Light.svg"));
            currentDepot->setToolTip("Network issues");
            //ui->startPollingButton->setEnabled(false);
            //ui->actionStart_Polling->setEnabled(false);
            //ui->stopPollingButton->setEnabled(false);
            //ui->actionStop_Polling->setEnabled(false);
            //ui->actionStart_Server->setEnabled(false);
            //ui->actionStop_Server->setEnabled(true);
            break;
        }
    }
    else
    {
        ui->startPollingButton->setEnabled(false);
        ui->actionStart_Polling->setEnabled(false);
        ui->stopPollingButton->setEnabled(false);
        ui->actionStop_Polling->setEnabled(false);
        ui->actionStart_Server->setEnabled(false);
        ui->actionStop_Server->setEnabled(false);
    }
}

void MainWindow::updateLights()
{
    if (_haveQuit) return;
    for (int i = 0; i < ui->depotList->count(); i++)
    {
        auto itm = ui->depotList->item(i);
        bookingOnPoint *dpt = dynamic_cast<bookingOnPoint* >(itm);
        switch (dpt->state())
        {
        case DepotServerState::InvalidConfiguration:
            dpt->setIcon(QIcon("://images/Off_Light.svg"));
            dpt->setToolTip("Invalid server configuration");
            break;
        case DepotServerState::Stopped:
            dpt->setIcon(QIcon("://images/Red_Light.svg"));
            dpt->setToolTip("Stopped");
            break;
        case DepotServerState::Started:
            dpt->setIcon(QIcon("://images/Yellow_Light.svg"));
            dpt->setToolTip("Running");
            break;
        case DepotServerState::Polling:
            dpt->setIcon(QIcon("://images/Green_Light.svg"));
            dpt->setToolTip("Polling");
            break;
        case DepotServerState::NetworkIssue:
            dpt->setIcon(QIcon("://images/Red_Light.svg"));
            dpt->setToolTip("Network issues");
            break;
        }
    }
}

void MainWindow::on_actionStart_Server_triggered()
{
    ui->actionStart_Server->setEnabled(false);
    auto currentDepot = dynamic_cast<bookingOnPoint *>(ui->depotList->currentItem());
    if (currentDepot) currentDepot->startScannersAsync();
    updateUi();
}


void MainWindow::on_actionStop_Server_triggered()
{
    ui->actionStop_Server->setEnabled(false);
    auto currentDepot = dynamic_cast<bookingOnPoint *>(ui->depotList->currentItem());
    if (currentDepot) currentDepot->stopScannersAsync();
    updateUi();
}


void MainWindow::on_depotList_itemClicked(QListWidgetItem *item)
{
    updateUi();
}


void MainWindow::on_serverStatus_indexesMoved(const QModelIndexList &indexes)
{
    updateUi();
}


void MainWindow::on_serverStatus_clicked(const QModelIndex &index)
{
    updateUi();
}

void MainWindow::serverStatus_dataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &roles)
{
    updateUi();
}


void MainWindow::on_clearButton_clicked()
{
    ui->clearButton->setEnabled(false);
    auto currentDepot = dynamic_cast<bookingOnPoint *>(ui->depotList->currentItem());
    if (currentDepot) currentDepot->clearMessageLog();

    updateUi();
}


void MainWindow::on_actionRestart_Server_triggered()
{
    ui->actionRestart_Server->setEnabled(false);
    ui->actionStart_All_Polling->setEnabled(false);
    ui->actionStop_All_Polling->setEnabled(false);
    ui->actionHalt_All_Servers->setEnabled(false);
    ui->startPollingButton->setEnabled(false);
    ui->actionStart_Polling->setEnabled(false);
    ui->stopPollingButton->setEnabled(false);
    ui->actionStop_Polling->setEnabled(false);
    ui->actionStart_Server->setEnabled(false);
    ui->actionStop_Server->setEnabled(false);

    auto dc = ui->depotList->count();
    for (int i = 0; i < dc; i++)
    {
        QCoreApplication::processEvents();
        bookingOnPoint *itm = dynamic_cast<bookingOnPoint *>(ui->depotList->item(i));
        itm->stopScanners();
    }
    while (ui->depotList->stillStopping())
    {
        QCoreApplication::processEvents();
    }
    for (int i = 0; i < dc; i++)
    {
        QCoreApplication::processEvents();
        bookingOnPoint *itm = dynamic_cast<bookingOnPoint *>(ui->depotList->item(i));
        itm->startScannersAsync();
    }
    while (ui->depotList->stillStarting())
    {
        QCoreApplication::processEvents();
    }

    updateUi();
}

void MainWindow::serverListMenuRequested(QPoint pos)
{
    QModelIndex index = ui->depotList->indexAt(pos);

    QMenu *menu = new QMenu(this);
    menu->addAction(ui->actionStart_Server);
    menu->addAction(ui->actionStop_Server);
    menu->addSeparator();
    menu->addAction(ui->actionStart_Polling);
    menu->addAction(ui->actionStop_Polling);
    menu->popup(ui->depotList->viewport()->mapToGlobal(pos));
}

void MainWindow::on_actionCopy_triggered()
{
    ui->serverStatus->copy();
}

void MainWindow::on_actionClear_triggered()
{
    ui->clearButton->setEnabled(false);
    auto currentDepot = dynamic_cast<bookingOnPoint *>(ui->depotList->currentItem());
    if (currentDepot) currentDepot->clearMessageLog();

    updateUi();
}

void MainWindow::serverStatusMenuRequested(QPoint pos)
{
    QModelIndex index = ui->serverStatus->indexAt(pos);

    QMenu *menu = new QMenu(this);
    menu->addAction(ui->actionClear);
    menu->addSeparator();
    menu->addAction(ui->actionSelect_All);
    menu->addSeparator();
    menu->addAction(ui->actionCut);
    menu->addAction(ui->actionCopy);
    menu->addAction(ui->actionPaste);
    menu->addSeparator();
    menu->addAction(ui->actionSave_Log);
    menu->popup(ui->serverStatus->viewport()->mapToGlobal(pos));
}

void MainWindow::on_actionSelect_All_triggered()
{
    ui->serverStatus->selectAll();
    updateUi();
}

void MainWindow::on_actionStop_All_Polling_triggered()
{
    ui->actionStop_All_Polling->setEnabled(false);
    ui->depotList->stopAllPollingAsync();
    updateUi();
}

void MainWindow::on_actionStart_All_Polling_triggered()
{
    ui->actionStart_All_Polling->setEnabled(false);
    ui->depotList->startAllPollingAsync();
    ui->actionStop_All_Polling->setEnabled(true);
    updateUi();
}

void MainWindow::on_actionHalt_All_Servers_triggered()
{
    ui->actionHalt_All_Servers->setEnabled(true);
    ui->depotList->haltAllAsync();
    ui->actionStop_All_Polling->setEnabled(false);
    updateUi();
}

void MainWindow::on_actionSave_Log_triggered()
{
    saveLogFileDialog->show();
}


