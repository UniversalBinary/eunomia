#ifndef IMAP_GATEWAY_HPP
#define IMAP_GATEWAY_HPP

#include "abstractGateway.hpp"
#include <thread>
#include <chrono>
#include <future>
#include <vmime/vmime.hpp>
#include "utils.hpp"

namespace telemeteryServices
{
    // Certificate verifier (TLS/SSL)
    class customCertificateVerifier : public vmime::security::cert::defaultCertificateVerifier
    {
    public:
        void verify(const vmime::shared_ptr <vmime::security::cert::certificateChain>& chain, const vmime::string& hostname)
        {
            try
            {
                setX509TrustedCerts(m_trustedCerts);

                defaultCertificateVerifier::verify(chain, hostname);
            }
            catch (vmime::security::cert::certificateException&)
            {
                // Obtain subject's certificate
                vmime::shared_ptr <vmime::security::cert::certificate> cert = chain->getAt(0);

                if (cert->getType() == "X.509")
                {
                    m_trustedCerts.push_back(vmime::dynamicCast <vmime::security::cert::X509Certificate>(cert));

                    setX509TrustedCerts(m_trustedCerts);
                    defaultCertificateVerifier::verify(chain, hostname);
                }
            }
        }

    private:
        static std::vector <vmime::shared_ptr <vmime::security::cert::X509Certificate> > m_trustedCerts;
    };

    std::vector <vmime::shared_ptr<vmime::security::cert::X509Certificate>> customCertificateVerifier::m_trustedCerts;

    class myTracer : public vmime::net::tracer
    {

    public:

        myTracer(
            const vmime::shared_ptr <std::ostringstream>& stream,
            const vmime::shared_ptr <vmime::net::service>& serv,
            const int connectionId
        )
            : m_stream(stream),
              m_service(serv),
              m_connectionId(connectionId) {

        }

        void traceSend(const vmime::string& line) {

            *m_stream << "[" << m_service->getProtocolName() << ":" << m_connectionId << "] Rx: " << line << std::endl;
        }

        void traceReceive(const vmime::string& line) {

            *m_stream << "[" << m_service->getProtocolName() << ":" << m_connectionId << "] Tx: " << line << std::endl;
        }

    private:

        vmime::shared_ptr <std::ostringstream> m_stream;
        vmime::shared_ptr <vmime::net::service> m_service;
        const int m_connectionId;
    };


    class myTracerFactory : public vmime::net::tracerFactory {

    public:

        myTracerFactory(const vmime::shared_ptr <std::ostringstream>& stream)
            : m_stream(stream) {

        }

        vmime::shared_ptr <vmime::net::tracer> create(
            const vmime::shared_ptr <vmime::net::service>& serv,
            const int connectionId
        ) {

            return vmime::make_shared <myTracer>(m_stream, serv, connectionId);
        }

    private:

        vmime::shared_ptr <std::ostringstream> m_stream;
    };

    class imapEmailGateway final : public abstractGateway
    {
    private:
        vmime::shared_ptr<vmime::net::session> _session;
        vmime::shared_ptr<vmime::net::store> _store;
        vmime::shared_ptr<vmime::net::transport> SMTPTransport;
        std::thread _pollThread;
        std::string _fetchUsername;
        std::string _fetchPassword;
        std::string _fetchServer;
        unsigned int _fetchPort;
        std::string _sendUsername;
        std::string _sendPassword;
        std::string _sendServer;
        unsigned int _sendPort;
    public:
        imapEmailGateway() : abstractGateway()
        {
            _fetchPort = 993;
            _sendPort = 587;
        }

        void poll() override
        {
            if (!_running) return;
            if (_polling) return;
            _polling = true;
            std::chrono::time_point now = std::chrono::system_clock::now();
            std::chrono::time_point stop = std::chrono::system_clock::now();
            std::vector<std::string> addressList;
            command currentCommand = command::none;
            std::string subject;
            bool claimed = false;

            if (_notificationReceived) _notificationReceived(*this, _id + " is preparing to start polling for incoming requests", _user_data);

            while (_polling)
            {
                currentCommand = command::none;
                try
                {
                    // Open the default folder in this store
                    vmime::shared_ptr <vmime::net::folder> folder = _store->getDefaultFolder();
                    folder->open(vmime::net::folder::MODE_READ_WRITE);
                    vmime::size_t count = folder->getMessageCount();

                    for (auto idx = count; idx >= 1; idx--)
                    {
                        if (!_polling) break;
                        auto message = folder->getMessage(idx);
                        folder->fetchMessage(message, vmime::net::fetchAttributes::FULL_HEADER);
                        // Check how old the message is.
                        auto date = vmime::dynamicCast<const vmime::datetime>(message->getHeader()->Date()->getValue());
                        std::tm tm1{};
                        tm1.tm_year = date->getYear() - 1900;
                        tm1.tm_mon = date->getMonth() - 1;
                        tm1.tm_mday = date->getDay();
                        tm1.tm_hour = date->getHour();
                        tm1.tm_min = date->getMinute();
                        tm1.tm_sec = date->getSecond();
                        std::time_t t1 = std::mktime(&tm1);
                        std::chrono::time_point<std::chrono::system_clock> dt2 = std::chrono::system_clock::now() - std::chrono::hours(36);
                        std::time_t t2 = std::chrono::system_clock::to_time_t(dt2);
                        if (t1 < t2) break;
                        // Get recipients.
                        auto to = message->getHeader()->To();
                        vmime::shared_ptr<const vmime::addressList> recipients1;
                        vmime::shared_ptr<const vmime::addressList> recipients2;
                        if (to) recipients1 = vmime::dynamicCast<const vmime::addressList>(to->getValue());
                        auto cc = message->getHeader()->Cc();
                        if (cc) recipients2 = vmime::dynamicCast<const vmime::addressList>(cc->getValue());
                        if (!utilities::search_for_email_address(_input_contact, recipients1, recipients2)) continue;
                        // Get sender.
                        auto sh = message->getHeader()->From();
                        if (sh)
                        {
                            auto sender = vmime::dynamicCast<const vmime::mailbox>(sh->getValue());
                            _last_sender = sender->getEmail().toString();
                            if (_last_sender.empty()) continue;
                            _last_sender_name = sender->getName().getWholeBuffer();
                            if (_last_sender_name.empty()) _last_sender_name = "Unknown sender";
                        }
                        else
                        {
                            continue;
                        }
                        // Get the subject.
                        auto sjh = message->getHeader()->Subject();
                        if (sjh)
                        {
                            auto subj = vmime::dynamicCast<const vmime::text>(sjh->getValue());
                            subject = subj->getWholeBuffer();
                            if (subject.empty()) subject = "No subject";
                        }
                        else
                        {
                            subject = "No subject";
                        }

                        if (utilities::search_for_email_address(_last_sender, _senders_acl))
                        {
                            if (_sender_access == utilities::accessControlAction::block)
                            {
                                if (_unauthorisedAccess) _unauthorisedAccess(*this, _last_sender, subject, _user_data);
                                continue;
                            }
                        }
                        else
                        {
                            if (_sender_access == utilities::accessControlAction::allow)
                            {
                                if (_unauthorisedAccess) _unauthorisedAccess(*this, _last_sender, subject, _user_data);
                                continue;
                            }
                        }

                        // Get the attachments.
                        folder->fetchMessage(message, vmime::net::fetchAttributes::STRUCTURE);
                        std::vector<std::unique_ptr<utilities::temporaryFile>> files;
                        utilities::getAttachments(message, _mimes_acl, _mime_access, files);

                        // Get the message text.
                        std::string text = utilities::getMessageText(message);

                        if (boost::iequals(subject, "post"))
                        {
                            currentCommand = command::explicit_post;
                        }
                        else if (boost::iequals(subject, "repost") || boost::iequals(subject, "re-post"))
                        {
                            currentCommand = command::repost;
                        }
                        else if (boost::iequals(subject, "delete") || boost::iequals(subject, "remove"))
                        {
                            currentCommand = command::remove;
                        }
                        else if (boost::iequals(subject, "killswitch"))
                        {
                            currentCommand = command::killswitch;
                        }
                        else
                        {
                            currentCommand = command::implicit_post;
                        }

                        if (_commandReceived)
                        {
                            claimed = _commandReceived(*this, currentCommand, _last_sender, text, files, _user_data);
                            // If the command has been claimed by the callee then delete the email.
                            if (claimed)
                            {
                                vmime::net::messageSet set = vmime::net::messageSet::byNumber(message->getNumber());
                                folder->deleteMessages(set);
                            }
                        }
                    }
                    folder->close(true);
                }
                catch (const std::exception& ex)
                {
                    if (_errorReceived) _errorReceived(*this, _id + " encountered an error checking for new mail: " + std::string(ex.what()), _user_data);
                }

                now = std::chrono::system_clock::now();
                stop = now + std::chrono::minutes(1);
                while (true)
                {
                    now = std::chrono::system_clock::now();
                    if (now >= stop) break;
                    if (!_polling) break;
                    std::this_thread::yield();
                }
            }
        }

        void pollAsync() override
        {
            if (!_running) return;
            if (_polling) return;
            _pollThread = std::thread([this]{ poll(); });
        }

        void pause() override
        {
            if (!_running) return;
            if (!_polling) return;
            _polling = false;
            if (_pollThread.joinable()) _pollThread.join();
            if (_notificationReceived) _notificationReceived(*this, _id + " is no longer polling for incoming requests", _user_data);
        }

        bool start() override
        {
            if (_running) return true;

            if (_fetchUsername.empty()) return false;
            if (_fetchPassword.empty()) return false;
            if (_fetchServer.empty()) return false;
            if (_fetchPort == 0) return false;
            if (_sendUsername.empty()) return false;
            if (_sendPassword.empty()) return false;
            if (_sendServer.empty()) return false;
            if (_sendPort == 0) return false;
            if (_senders_acl.empty() && (_sender_access == utilities::accessControlAction::allow)) return false;
            if (_mimes_acl.empty() && (_mime_access == utilities::accessControlAction::allow)) return false;
            if (_admin_contact.empty()) return false;
            if (_input_contact.empty()) return false;
            if (_killswitch_password.empty()) return false;

            _id = "IMAP session (" + _fetchServer + ":" + _input_contact + ")";

            try
            {
                _session = vmime::net::session::create();
                _session->getProperties().setProperty("options.sasl", true);
                _session->getProperties().setProperty("auth.username", _fetchUsername);
                _session->getProperties().setProperty("auth.password", _fetchPassword);
                vmime::utility::url url("imaps", _fetchServer, _fetchPort);
                _store = _session->getStore(url);
                _store->setProperty("connection.tls", true);
                _store->setProperty("connection.tls.required", true);
                _store->setProperty("options.need-authentication", true);
                _store->setProperty("auth.username", _fetchUsername);
                _store->setProperty("auth.password", _fetchPassword);
                _store->setProperty("options.chunking", false);
                _store->setCertificateVerifier(vmime::make_shared<customCertificateVerifier>());

                vmime::shared_ptr <std::ostringstream> traceStream = vmime::make_shared <std::ostringstream>();
                _store->setTracerFactory(vmime::make_shared <myTracerFactory>(traceStream));

                _store->connect();

            }
            catch (const std::exception& ex)
            {
                if (_errorReceived) _errorReceived(*this, _id + " failed to start: " + std::string(ex.what()), _user_data);
                return false;
            }

            if (_notificationReceived) _notificationReceived(*this, _id + " is running", _user_data);

            try
            {
                vmime::utility::url url("smtp", _sendServer, _sendPort);
                SMTPTransport = _session->getTransport(url);
                SMTPTransport->setProperty("connection.tls", true);
                SMTPTransport->setProperty("connection.tls.required", true);
                SMTPTransport->setProperty("options.need-authentication", true);
                SMTPTransport->setProperty("auth.username", _sendUsername);
                SMTPTransport->setProperty("auth.password", _sendPassword);
                SMTPTransport->setProperty("options.chunking", false);
                SMTPTransport->setCertificateVerifier(vmime::make_shared<customCertificateVerifier>());
                SMTPTransport->connect();

            }
            catch (const std::exception& ex)
            {
                if (_errorReceived) _errorReceived(*this, "SMTP session (" + _sendServer + ":" + _input_contact + ") failed to start: " + std::string(ex.what()), _user_data);
                return false;
            }

            if (_notificationReceived) _notificationReceived(*this, "SMTP session (" + _sendServer + ":" + _input_contact + ") is running", _user_data);


            _running = true;

            return true;
        }

        void stop() override
        {
            if (!_running) return;
            if (_polling) pause();

            if (_notificationReceived) _notificationReceived(*this, _id + " has stopped running", _user_data);
            if (_notificationReceived) _notificationReceived(*this, "SMTP session (" + _sendServer + ":" + _input_contact + ") has stopped running", _user_data);
        }

        void messageUser(const std::string& recipient, const std::string& subject, const std::string& message) const override
        {
            try
            {
                vmime::messageBuilder msgbld;
                msgbld.setExpeditor(vmime::mailbox(_input_contact));
                vmime::addressList toli;
                toli.appendAddress(vmime::make_shared<vmime::mailbox>(recipient));
                msgbld.setRecipients(toli);
                msgbld.setSubject(vmime::text(subject));
                msgbld.getTextPart()->setText(vmime::make_shared<vmime::stringContentHandler>(message));
                vmime::shared_ptr<vmime::message> msg = msgbld.construct();
                SMTPTransport->send(msg);
            }
            catch (const std::exception& ex)
            {
                if (_errorReceived) _errorReceived(*this, "SMTP session (" + _sendServer + ":" + _input_contact + ") sending message to '" + recipient + "' failed: " + std::string(ex.what()), _user_data);
            }
        }

        void messageAdmin(const std::string& subject, const std::string& message) const override
        {
            messageUser(_admin_contact, subject, message);
        }

        void messageLastPoster(const std::string& subject, const std::string& message) const override
        {
            messageUser(_last_sender, subject, message);
        }

        [[nodiscard]] std::string fetchUsername() const
        {
            return _fetchUsername;
        }

        [[nodiscard]] std::string fetchPassword() const
        {
            return _fetchPassword;
        }

        [[nodiscard]] std::string fetchServer() const
        {
            return _fetchServer;
        }

        [[nodiscard]] unsigned int fetchPort() const
        {
            return _fetchPort;
        }

        [[nodiscard]] std::string sendUsername() const
        {
            return _sendUsername;
        }

        [[nodiscard]] std::string sendPassword() const
        {
            return _sendPassword;
        }

        [[nodiscard]] std::string sendServer() const
        {
            return _sendServer;
        }

        [[nodiscard]] unsigned int sendPort() const
        {
            return _sendPort;
        }

        void setFetchUsername (const std::string& value)
        {
            _fetchUsername = value;
        }

        void setFetchPassword (const std::string& value)
        {
            _fetchPassword = value;
        }

        void setFetchServer (const std::string& value)
        {
            _fetchServer = value;
        }

        void setFetchPort(unsigned int port)
        {
            _fetchPort = port;
        }

        void setSendUsername (const std::string& value)
        {
            _sendUsername = value;
        }

        void setSendPassword (const std::string& value)
        {
            _sendPassword = value;
        }

        void setSendServer (const std::string& value)
        {
            _sendServer = value;
        }

        void setSendPort(unsigned int port)
        {
            _sendPort = port;
        }
    };
}

#endif // IMAP_GATEWAY_HPP
