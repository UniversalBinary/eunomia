//
// Created by chris on 23/05/2021.
//

#include <QListWidget>
#include <future>
#include "bookingOnPoint.hpp"

#ifndef _BOOKING_ON_POINT_LIST_HPP
#define _BOOKING_ON_POINT_LIST_HPP


class bookingOnPointList : public QListWidget
{
private:
    std::future<void> _startFuture;
    std::future<void> _stopFuture;
    std::future<void> _restartFuture;
    std::future<void> _haltFuture;
public:
    explicit bookingOnPointList(QWidget *parent) : QListWidget(parent)
    {

    }
public slots:
    void startAllPollingAsync()
    {
        if (_startFuture.valid()) return;
        if (_restartFuture.valid()) return;
        if (_stopFuture.valid()) _startFuture.wait();
        if (_haltFuture.valid()) return;

        _startFuture = std::async(std::launch::async, [this]
        {
            for (auto i = 0; i < count(); i++)
            {
                auto bop = dynamic_cast<bookingOnPoint *>(item(i));
                bop->startScanners();
                bop->startPollingAsync();
            }
        });
    }

    void stopAllPollingAsync()
    {
        if (_startFuture.valid()) _startFuture.wait();
        if (_restartFuture.valid()) _restartFuture.wait();
        if (_stopFuture.valid()) return;
        if (_haltFuture.valid()) return;

        _stopFuture = std::async(std::launch::async, [this]
        {
            for (auto i = 0; i < count(); i++)
            {
                auto bop = dynamic_cast<bookingOnPoint *>(item(i));
                bop->stopPolling();
            }
        });
    }

    void haltAllAsync()
    {
        if (_startFuture.valid()) _startFuture.wait();
        if (_restartFuture.valid()) _restartFuture.wait();
        if (_stopFuture.valid()) _stopFuture.wait();
        if (_haltFuture.valid()) return;

        _haltFuture = std::async(std::launch::async, [this]
        {
            for (auto i = 0; i < count(); i++)
            {
                auto bop = dynamic_cast<bookingOnPoint *>(item(i));
                bop->stopPolling();
                bop->stopScanners();
            }
        });
    }

public:
    [[nodiscard]] bool blocking() const
    {
        return _startFuture.valid() || _stopFuture.valid() || _restartFuture.valid();
    }

    bool anyInvalid() const
    {
        for (auto i = 0; i < count(); i++)
        {
            auto bop = dynamic_cast<bookingOnPoint *>(item(i));
            if (bop->state() == DepotServerState::InvalidConfiguration) return true;
        }

        return false;
    }

    bool allInvalid() const
    {
        for (auto i = 0; i < count(); i++)
        {
            auto bop = dynamic_cast<bookingOnPoint *>(item(i));
            if (bop->state() != DepotServerState::InvalidConfiguration) return false;
        }

        return true;
    }

    bool anyStopped() const
    {
        for (auto i = 0; i < count(); i++)
        {
            auto bop = dynamic_cast<bookingOnPoint *>(item(i));
            if (bop->state() == DepotServerState::Stopped) return true;
        }

        return false;
    }

    bool allStopped() const
    {
        for (auto i = 0; i < count(); i++)
        {
            auto bop = dynamic_cast<bookingOnPoint *>(item(i));
            if (bop->state() != DepotServerState::Stopped) return false;
        }

        return true;
    }

    bool anyStarted() const
    {
        for (auto i = 0; i < count(); i++)
        {
            auto bop = dynamic_cast<bookingOnPoint *>(item(i));
            if (bop->state() == DepotServerState::Started) return true;
        }

        return false;
    }

    bool allStarted() const
    {
        for (auto i = 0; i < count(); i++)
        {
            auto bop = dynamic_cast<bookingOnPoint *>(item(i));
            if (bop->state() != DepotServerState::Started) return false;
        }

        return true;
    }

    bool anyPolling() const
    {
        for (auto i = 0; i < count(); i++)
        {
            auto bop = dynamic_cast<bookingOnPoint *>(item(i));
            if (bop->state() == DepotServerState::Polling) return true;
        }

        return false;
    }

    bool allPolling() const
    {
        for (auto i = 0; i < count(); i++)
        {
            auto bop = dynamic_cast<bookingOnPoint *>(item(i));
            if (bop->state() != DepotServerState::Polling) return false;
        }

        return true;
    }

    bool stillStopping() const
    {
        for (auto i = 0; i < count(); i++)
        {
            auto bop = dynamic_cast<bookingOnPoint *>(item(i));
            if (bop->state() != DepotServerState::Stopped) return true;
        }

        return false;
    }

    bool stillStarting() const
    {
        for (auto i = 0; i < count(); i++)
        {
            auto bop = dynamic_cast<bookingOnPoint *>(item(i));
            if (bop->state() != DepotServerState::Started) return true;
        }

        return false;
    }
};


#endif // _BOOKING_ON_POINT_LIST_HPP
