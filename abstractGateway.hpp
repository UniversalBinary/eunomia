/*
 * Copyright (c) 2021 Chris Morrison
 *
 * Filename: abtract_scanner.hpp
 * Created on Tuesday 2nd March 2021 at 17:20
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef _ABSTRACT_GATEWAY_HPP_
#define _ABSTRACT_GATEWAY_HPP_

#include <functional>
#include <set>
#include <boost/regex.hpp>

#include "utils.hpp"

namespace telemeteryServices
{
    /**
     * <p>The exception that is thrown when the scanning must come to an end due to a valid killswitch command being issued.</p>
     * <p>This exception must not be thrown for any other reason.</p>
     * <p>Once this exception has been thrown, the scanning thread must gracefully and cleanly exit.</p>
     */
    class killswitch_triggered : public std::exception
    {
    public:
        killswitch_triggered() : std::exception()
        {

        }
    };

    /**
     * The exception that is thrown when a call is made to an object that is not in a valid state for that call.
     */
    class illegal_object_state : public std::exception
    {
    private:
        std::string _mess;
    public:
        illegal_object_state() : std::exception()
        {
            _mess = "Illegal object state.";
        }

        explicit illegal_object_state(const char *message) : std::exception()
        {
            _mess = message;
        }

        [[nodiscard]] const char *what() const noexcept override
        {
            return _mess.c_str();
        }
    };

    /**
     * Provides values for the commands that can be issued to the telemetry scanner.
     */
    enum class command
    {
        /**
         * No command has been issued yet, or the last command has been processed.
         */
        none,
        /**
         * The enclosed payload file has been posted.
         */
        implicit_post,
        /**
         * The enclosed payload file has been posted with a 'post' command.
         */
        explicit_post,
        /**
         * The last file to be posted must be removed and replaced with the attached payload file, a 'repost' command was provided.
         */
        repost,
        /**
         * The results of the last post command should be undone.
         */
        remove,
        /**
         * The killswitch command has been requested, the killswitch command must be issued once any cleaning up has been done and notifications have been sent.
         */
        killswitch_pending,
        /**
         * The process or thread leveraging this telemetry scanner must stop scanning and come to an end.
         */
        killswitch,

        subcribe,

        unsubscribe,
    };

    /**
     * A convenience function that gets a string representation of the given command.
     * @param value The command that the string representation is required.
     * @return A string representation of the given command, or an empty string if command is none or an unrecognised value.
     */
    std::string commandToString(command value)
    {
        switch (value)
        {
        case command::none:
            return "";
        case command::subcribe:
            return "subscribe";
        case command::unsubscribe:
            return "unsubscribe";
        case command::implicit_post:
            return "implicit post";
        case command::explicit_post:
            return "explicit post";
        case command::repost:
            return "re-post";
        case command::remove:
            return "remove";
        case command::killswitch:
        case command::killswitch_pending:
            return "killswitch";
        }

        return "";
    }

    class abstractGateway;

    typedef std::function<void(const abstractGateway& sender, const std::string& message, void* userData)> notificationCallback;
    typedef std::function<void(const abstractGateway& sender, const std::string& message, void* userData)> warningCallback;
    typedef std::function<void(const abstractGateway& sender, const std::string& message, void* userData)> errorCallback;
    typedef std::function<void(const abstractGateway& sender, const std::string& originator, const std::string& subject, void* userData)> unauthorisedAccessCallback;
    typedef std::function<bool(const abstractGateway& sender, command command, const std::string& originator, std::string& message, std::vector<std::unique_ptr<utilities::temporaryFile>>& payload, void* userData)> commandReceivedCallback;

    class abstractGateway
    {        
    public:
        virtual ~abstractGateway()
        {

        }

        abstractGateway()
        {
            _polling = false;
            _running = false;
            _mime_access = utilities::accessControlAction::block;
            _sender_access = utilities::accessControlAction::block;
        }
        abstractGateway(const abstractGateway& other) = default;
        abstractGateway(abstractGateway&& other) = default;

        void setSendersAccessControlAction(utilities::accessControlAction action)
        {
            _sender_access = action;
        }

        utilities::accessControlAction getSenderAccessControlAction() const
        {
            return _sender_access;
        }

        void setMimeTypeAccessControlAction(utilities::accessControlAction action)
        {
            _mime_access = action;
        }

        utilities::accessControlAction getMimeTypeAccessControlAction() const
        {
            return _mime_access;
        }

        void addControlledMimeType(const std::string& mime)
        {
            if (mime.empty()) return;
            _mimes_acl.emplace(mime);
        }

        void addControlledSender(const std::string& sender)
        {
            if (sender.empty()) return;
            _senders_acl.emplace(sender);
        }

        void setKillswitchPassword(const std::string& password)
        {
            _killswitch_password = password;
        }

        void setNotificationCallback(const notificationCallback& callback)
        {
            _notificationReceived = callback;
        }

        void setWarningCallback(const warningCallback& callback)
        {
            _warningReceived = callback;
        }

        void setErrorCallback(const errorCallback& callback)
        {
            _errorReceived = callback;
        }

        void setCommandReceivedCallback(const commandReceivedCallback& callback)
        {
            _commandReceived = callback;
        }

        void setUnauthorisedAccessCallback(const unauthorisedAccessCallback& callback)
        {
            _unauthorisedAccess = callback;
        }

        void setAdminContact(const std::string& contact)
        {
            _admin_contact = contact;
        }

        [[nodiscard]] std::string getAdminAddress() const
        {
            return _admin_contact;
        }

        void setInputContact(const std::string& contact)
        {
            _input_contact = contact;
        }

        [[nodiscard]] std::string getInputContact() const
        {
            return _input_contact;
        }

        void *userData()
        {
            return _user_data;
        }

        void setUserData(void *data)
        {
            _user_data = data;
        }

        bool polling()
        {
            return _polling;
        }

        std::string getID() const
        {
            return _id;
        }

        virtual void poll() = 0;
        virtual void pollAsync() = 0;
        virtual void pause() = 0;
        virtual void stop() = 0;
        virtual bool start() = 0;
        virtual void messageUser(const std::string& user_id, const std::string& subject, const std::string& message) const = 0;
        virtual void messageAdmin(const std::string& subject, const std::string& message) const = 0;
        virtual void messageLastPoster(const std::string& subject, const std::string& message) const = 0;

    protected:
        std::set<std::string> _mimes_acl;
        std::set<std::string> _senders_acl;
        utilities::accessControlAction _mime_access;
        utilities::accessControlAction _sender_access;
        std::string _id;
        void *_user_data;
        bool _polling;
        bool _running;
        std::string _admin_contact;
        std::string _input_contact;
        std::string _killswitch_password;
        commandReceivedCallback _commandReceived;
        notificationCallback _notificationReceived;
        warningCallback _warningReceived;
        errorCallback _errorReceived;
        unauthorisedAccessCallback _unauthorisedAccess;
        std::string _last_sender_name;
        std::string _last_sender;
    };
}

#endif //_ABSTRACT_SCANNER_HPP_
