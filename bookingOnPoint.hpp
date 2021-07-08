#ifndef DEPOT_H
#define DEPOT_H

#include <QString>
#include <QXmlStreamReader>
#include <QListWidgetItem>
#include <QDateTime>
#include <QDomNode>
#include <QStringListModel>
#include "imapEmailGateway.hpp"

inline QString buildQString(const char * string)
{
    return QString::fromUtf8(string, std::strlen(string));
}

inline QString buildQString(const wchar_t * string)
{
    return QString::fromWCharArray(string);
}

inline QString buildQString(const std::string& string)
{
    return QString::fromStdString(string);
}

inline QString buildQString(const std::wstring& string)
{
    return QString::fromStdWString(string);
}

inline QString buildQString(const std::u16string& string)
{
    return QString::fromStdU16String(string);
}

inline QString buildQString(const std::u32string& string)
{
    return QString::fromStdU32String(string);
}

inline QString buildQString(const QString& string)
{
    return QString(string);
}

enum DepotServerState
{
    InvalidConfiguration,
    Stopped,
    Started,
    Polling,
    NetworkIssue,
};

class bookingOnPoint : public QListWidgetItem
{
private:
    QString _company;
    QString _name;
    QString _organisationalUnit;
    QString _line;
    std::vector<std::unique_ptr<telemeteryServices::abstractGateway>> _scanners;
    DepotServerState _state;
    std::thread _startThread;
    std::thread _stopThread;
    QStringList _messageLog;
    QStringListModel *_model;
    std::mutex logMutex;
    std::mutex stateMutex;
public:
    bookingOnPoint(const QString& company, const QString& name, const QString& orgUnit, const QString& line);
    ~bookingOnPoint() override;
    template<typename ScannerT>
    void parseAndAddMailGateway(const QDomNode& sNode, ScannerT& scanner);
    [[nodiscard]] QString name() const;
    [[nodiscard]] QString organisationalUnit() const;
    [[nodiscard]] QString line() const;
    void startScanners();
    void startScannersAsync();
    void stopScanners();
    void stopScannersAsync();
    void startPollingAsync();
    void stopPolling();
    [[nodiscard]] bool blocked() const;
    [[nodiscard]] const std::vector<std::unique_ptr<telemeteryServices::abstractGateway>>& scanners() const;
    DepotServerState state();
    void setState(DepotServerState newState);
    void claimListModel(QStringListModel *model);
    void disclaimModel();
    void clearMessageLog();
    void appendLogMessage(const QString& message);
};

inline void notification(const telemeteryServices::abstractGateway& sender, const std::string& message, void* userData)
{
    bookingOnPoint *depot = static_cast<bookingOnPoint *>(userData);
    depot->appendLogMessage(QString::fromStdString(message));
}

inline void warning(const telemeteryServices::abstractGateway& sender, const std::string& message, void* userData)
{
    bookingOnPoint *depot = static_cast<bookingOnPoint *>(userData);
    depot->appendLogMessage(QString::fromStdString(message));
}

inline void error(const telemeteryServices::abstractGateway& sender, const std::string& message, void* userData)
{
    bookingOnPoint *depot = static_cast<bookingOnPoint *>(userData);
    depot->appendLogMessage(QString::fromStdString(message));
}

inline void unauthorised(const telemeteryServices::abstractGateway& sender, const std::string& originator, const std::string& subject, void* userData)
{
    bookingOnPoint *depot = static_cast<bookingOnPoint *>(userData);
    std::string s = "Unauthorised interaction detected and blocked from '" + originator + "' in " + sender.getID();
    depot->appendLogMessage(QString::fromStdString(s));
}

inline bool command(const telemeteryServices::abstractGateway& sender, telemeteryServices::command command, const std::string& originator, std::string& message, std::vector<std::unique_ptr<utilities::temporaryFile>>& payload, void* userData)
{
    bookingOnPoint *depot = static_cast<bookingOnPoint *>(userData);
    std::string s = "Command '" + commandToString(command) + "' received from '" + originator + "' via " + sender.getID();
    depot->appendLogMessage(QString::fromStdString(s));

    std::cout << "Text: \n" << message << "\n";
    for (const auto& a : payload)
    {
        std::cout << "Attachment: " << a->string() << "\n";
    }

    switch (command)
    {
    case telemeteryServices::command::none:
        break;
    case telemeteryServices::command::implicit_post:

        break;
    case telemeteryServices::command::explicit_post:
        if (payload.empty())
        {
            sender.messageLastPoster("The last 'post' command to the " + depot->name().toStdString() + " schedule sheet dispatcher failed!", "There was no payload file attached to this email.\n\nDid you forget to attach the PDF file?");
            std::string s = "The last '" + commandToString(command) + "' command received from '" + originator + "' via " + sender.getID() + " failed because there was no viable payload attached to the email.";
            depot->appendLogMessage(QString::fromStdString(s));
            return true;
        }
        break;
    case telemeteryServices::command::repost:
        if (payload.empty())
        {
            sender.messageLastPoster("The last 're-post' command to the " + depot->name().toStdString() + " schedule sheet dispatcher failed!", "There was no payload file attached to this email.\n\nDid you forget to attach the PDF file?");
            std::string s = "The last '" + commandToString(command) + "' command received from '" + originator + "' via " + sender.getID() + " failed because there was no viable payload attached to the email.";
            depot->appendLogMessage(QString::fromStdString(s));
            return true;
        }
        break;
    case telemeteryServices::command::remove:
        break;
    case telemeteryServices::command::killswitch_pending:
        break;
    case telemeteryServices::command::killswitch:
        break;
    case telemeteryServices::command::subcribe:
        break;
    case telemeteryServices::command::unsubscribe:
        break;

    }

    // Process and action command.
    return false;
}

bookingOnPoint::bookingOnPoint(const QString& company, const QString& name, const QString& orgUnit, const QString& line) : QListWidgetItem("Invalid", nullptr, QListWidgetItem::ItemType::UserType)
{
    _company = company;
    _name = name;
    _organisationalUnit = orgUnit;
    _line = line;
    _state = DepotServerState::InvalidConfiguration;
    _model = nullptr;
    setIcon(QIcon("://images/Off_Light.svg"));
    setText(_name + " (" + _line + ")");
}

template<typename ScannerT>
void bookingOnPoint::parseAndAddMailGateway(const QDomNode& sNode, ScannerT& scanner)
{
    auto aaNode = sNode.namedItem("admin-address");
    if (aaNode.isNull() && aaNode.toElement().text().isEmpty()) return;
    scanner->setAdminContact(aaNode.toElement().text().toStdString());
    auto iaNode = sNode.namedItem("input-address");
    if (iaNode.isNull() && iaNode.toElement().text().isEmpty()) return;
    scanner->setInputContact(iaNode.toElement().text().toStdString());
    auto kkNode = sNode.namedItem("killswitch-key");
    if (kkNode.isNull() || kkNode.toElement().text().isEmpty()) return;
    scanner->setKillswitchPassword(kkNode.toElement().text().toStdString());
    auto isNode = sNode.namedItem("incoming-server");
    if (isNode.isNull() || isNode.childNodes().isEmpty()) return;
    auto unNode = isNode.namedItem("username");
    if (unNode.isNull() || unNode.toElement().text().isEmpty()) return;
    scanner->setFetchUsername(unNode.toElement().text().toStdString());
    auto pwNode = isNode.namedItem("password");
    if (pwNode.isNull() || pwNode.toElement().text().isEmpty()) return;
    scanner->setFetchPassword(pwNode.toElement().text().toStdString());
    auto hNode = isNode.namedItem("host");
    if (hNode.isNull() || hNode.toElement().text().isEmpty()) return;
    scanner->setFetchServer(hNode.toElement().text().toStdString());
    auto pNode = isNode.namedItem("port");
    if (pNode.isNull() || pNode.toElement().text().isEmpty()) return;
    scanner->setFetchPort(pNode.toElement().text().toUInt());
    auto ogNode = sNode.namedItem("outgoing-server");
    if (ogNode.isNull() || ogNode.childNodes().isEmpty()) return;
    unNode = ogNode.namedItem("username");
    if (unNode.isNull() || unNode.toElement().text().isEmpty()) return;
    scanner->setSendUsername(unNode.toElement().text().toStdString());
    pwNode = ogNode.namedItem("password");
    if (pwNode.isNull() || pwNode.toElement().text().isEmpty()) return;
    scanner->setSendPassword(pwNode.toElement().text().toStdString());
    hNode = ogNode.namedItem("host");
    if (hNode.isNull() || hNode.toElement().text().isEmpty()) return;
    scanner->setSendServer(hNode.toElement().text().toStdString());
    pNode = ogNode.namedItem("port");
    if (pNode.isNull() || pNode.toElement().text().isEmpty()) return;
    scanner->setSendPort(pNode.toElement().text().toUInt());
    scanner->setSendersAccessControlAction(utilities::accessControlAction::allow);
    auto aclNode = sNode.namedItem("access-control-list");
    if (aclNode.isNull() || !aclNode.isElement()) return;
    auto inputers = aclNode.toElement().elementsByTagName("email");
    if (inputers.isEmpty()) return;
    for (int idx3 = 0; idx3 < inputers.count(); idx3++)
    {
        scanner->addControlledSender(inputers.at(idx3).toElement().text().toStdString());
    }
    scanner->setMimeTypeAccessControlAction(utilities::accessControlAction::allow);
    scanner->addControlledMimeType("application/pdf");
    scanner->setUserData(this);
    // Attach the callbacks.
    scanner->setNotificationCallback(notification);
    scanner->setErrorCallback(error);
    scanner->setWarningCallback(warning);
    scanner->setUnauthorisedAccessCallback(unauthorised);
    scanner->setCommandReceivedCallback(command);
    _scanners.push_back(std::move(scanner));
    _state = DepotServerState::Stopped;
}

bookingOnPoint::~bookingOnPoint()
{
    for (auto& scn : _scanners)
    {
        scn->stop();
    }
}

void bookingOnPoint::claimListModel(QStringListModel *model)
{
    if (model == nullptr) throw std::invalid_argument("model == nullptr");

    _model = model;
    _model->setStringList(_messageLog);
}

void bookingOnPoint::disclaimModel()
{
    _model = nullptr;
}

void bookingOnPoint::clearMessageLog()
{
    logMutex.lock();
    _messageLog.clear();
    if (_model) _model->setStringList(_messageLog);
    logMutex.unlock();
}

void bookingOnPoint::appendLogMessage(const QString& message)
{
    QString now = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    QString line = now + " " + message;

    logMutex.lock();
    _messageLog << line;
    if(_model && _model->insertRow(_model->rowCount()))
    {
        QModelIndex index = _model->index(_model->rowCount() - 1, 0);
        _model->setData(index, line);
    }
    logMutex.unlock();
}

const std::vector<std::unique_ptr<telemeteryServices::abstractGateway>>& bookingOnPoint::scanners() const
{
    return _scanners;
}

QString bookingOnPoint::name() const
{
    return _name;
}

QString bookingOnPoint::organisationalUnit() const
{
    return _organisationalUnit;
}

QString bookingOnPoint::line() const
{
    return _line;
}

DepotServerState bookingOnPoint::state()
{
    DepotServerState retval;
    stateMutex.lock();
    retval = _state;
    stateMutex.unlock();

    return retval;
}

void bookingOnPoint::setState(DepotServerState newState)
{
    stateMutex.lock();
    _state = newState;
    stateMutex.unlock();
}

void bookingOnPoint::startScannersAsync()
{
    if (_state == DepotServerState::Started) return;
    if (_stopThread.joinable()) _stopThread.join();
    if (_startThread.joinable()) return;

    _startThread = std::thread([this]
    {
        bool _failures = false;
        for (auto& scn : _scanners)
        {
            if (!scn->start()) _failures = true;
        }

        if (_failures)
        {
            for (auto& scn : _scanners)
            {
                scn->pause();
                scn->stop();
            }
            _state = DepotServerState::Stopped;
            appendLogMessage("Server failed to start");
        }
        else
        {
            _state = DepotServerState::Started;
            appendLogMessage("Server started");
        }
    });
}

void bookingOnPoint::startScanners()
{
    if (_state == DepotServerState::Started) return;
    if (_stopThread.joinable()) _stopThread.join();
    if (_startThread.joinable()) return;

    bool _failures = false;
    for (auto& scn : _scanners)
    {
        if (!scn->start()) _failures = true;
    }

    if (_failures)
    {
        for (auto& scn : _scanners)
        {
            scn->pause();
            scn->stop();
        }
        _state = DepotServerState::Stopped;
        appendLogMessage("Server failed to start");
    }
    else
    {
        _state = DepotServerState::Started;
        appendLogMessage("Server started");
    }
}

void bookingOnPoint::stopScanners()
{
    if (_state == DepotServerState::Stopped) return;
    if (_startThread.joinable()) _startThread.join();
    if (_stopThread.joinable()) return;
    for (auto& scn : _scanners)
    {
        scn->pause();
        scn->stop();
    }

    _state = DepotServerState::Stopped;
    appendLogMessage("Server stopped");
}

void bookingOnPoint::stopScannersAsync()
{
    if (_state == DepotServerState::Stopped) return;
    if (_startThread.joinable()) _startThread.join();
    if (_stopThread.joinable()) return;

    _stopThread = std::thread([this]
    {
        for (auto& scn : _scanners)
        {
            scn->pause();
            scn->stop();
        }

        _state = DepotServerState::Stopped;
        appendLogMessage("Server stopped");
    });
}

void bookingOnPoint::startPollingAsync()
{
    if (_state != DepotServerState::Started) return;
    if (_startThread.joinable()) _startThread.join();
    if (_stopThread.joinable()) return;

    for (auto& scn : _scanners)
    {
        scn->pollAsync();
    }
    _state = DepotServerState::Polling;

    appendLogMessage("Polling for telemetery");
}

void bookingOnPoint::stopPolling()
{
    if (_state != DepotServerState::Polling) return;
    for (auto& scn : _scanners)
    {
        scn->pause();
    }
    _state = DepotServerState::Started;

    appendLogMessage("Polling concluded");
}

#endif // DEPOT_H
