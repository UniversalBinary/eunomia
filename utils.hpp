#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <vmime/vmime.hpp>
#include "textCorpus.hpp"

namespace utilities
{
    enum class accessControlAction
    {
        block,
        allow,
    };

    class temporaryFile
    {
    private:
        boost::filesystem::path _path;
    public:
        temporaryFile()
        {
            _path = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
        }

        template<typename StringT>
        explicit temporaryFile(const StringT& file_path)
        {
            _path = boost::filesystem::path(file_path);
        }

        ~temporaryFile()
        {
            boost::system::error_code ec;
            if (!_path.empty() && boost::filesystem::exists(_path, ec))
            {
                boost::filesystem::remove(_path, ec);
            }
        }

        [[nodiscard]] boost::filesystem::path path() const
        {
            return _path;
        }

        [[nodiscard]] std::string string() const
        {
            return _path.string();
        }

        [[nodiscard]] std::wstring wstring() const
        {
            return _path.wstring();
        }

        void copyTo(const boost::filesystem::path& dest)
        {
            boost::filesystem::copy_file(_path, dest);
        }

        bool copyTo(const boost::filesystem::path& dest, boost::system::error_code& ec) noexcept
        {
            return boost::filesystem::copy_file(_path, dest, ec);
        }

        void copyTo(const boost::filesystem::path& dest, boost::filesystem::copy_option option)
        {
            boost::filesystem::copy_file(_path, dest, option);
        }

        bool copyTo(const boost::filesystem::path& dest, boost::filesystem::copy_option option, boost::system::error_code& ec) noexcept
        {
            return boost::filesystem::copy_file(_path, dest, option, ec);
        }

        void moveTo(const boost::filesystem::path& dest)
        {
            boost::filesystem::copy_file(_path, dest);
            boost::filesystem::remove(_path);
            _path.clear();
        }

        bool moveTo(const boost::filesystem::path& dest, boost::system::error_code& ec) noexcept
        {
            if (!boost::filesystem::copy_file(_path, dest, ec)) return false;
            return boost::filesystem::remove(_path, ec);
        }

        void moveTo(const boost::filesystem::path& dest, boost::filesystem::copy_option option)
        {
            boost::filesystem::copy_file(_path, dest, option);
            boost::filesystem::remove(_path);
            _path.clear();
        }

        bool moveTo(const boost::filesystem::path& dest, boost::filesystem::copy_option option, boost::system::error_code& ec)
        {
            if (!boost::filesystem::copy_file(_path, dest, option, ec)) return false;
            return boost::filesystem::remove(_path, ec);
        }
    };

    inline size_t extract_email_addresses(const std::string& line, std::vector<std::string>& addresses, bool clear = true)
    {
        if (clear) addresses.clear();
        boost::regex exp("(?:[a-z0-9_\\-\\.]+@[a-z0-9_\\-\\.]+\\.){1}(?:(?:[a-z]{2,10})|(?:[a-z]{2,10}\\.[a-z]{2,10}))");
        boost::smatch match;
        if (boost::regex_search(line, match, exp))
        {
            for (const auto& sm : match)
            {
                addresses.push_back(sm.str());
            }
        }

        return addresses.size();
    }

    template<typename StringT>
    inline bool compare_email_addresses(const StringT& address1, const StringT& address2)
    {
        return boost::iequals(address1, address2);
    }

    template<typename StringT, typename ListT>
    inline bool search_for_email_address(const StringT& address1, const ListT& addressList)
    {
        for (const auto& s : addressList)
        {
            if (boost::iequals(address1, s)) return true;
        }

        return false;
    }

    template<typename ListT1, typename ListT2>
    bool compare_email_address_lists(const ListT1& list1, const ListT2 list2)
    {
        for (const auto& s : list1)
        {
            if (search_for_email_address(s, list2)) return true;
        }

        return false;
    }

    inline std::string getMessageText(const vmime::shared_ptr<vmime::net::message>& message)
    {
        std::stringstream ss;
        auto pm = message->getParsedMessage();
        if (!pm) return "";
        vmime::messageParser parser(pm);
        vmime::utility::outputStreamAdapter out(ss);
        vmime::mediaType plainText(vmime::mediaTypes::TEXT, vmime::mediaTypes::TEXT_PLAIN);

        for (const auto& textPart : parser.getTextPartList())
        {
            textPart->getText()->extract(out);
        }

        std::cout << "==========================================================================================================================\n";
        fsl::text::textCorpus corpus;
        corpus.parseString(ss.str(), true);
        for (const auto& l : corpus.parts())
        {
            std::cout << l << "\n";
        }
        std::cout << "==========================================================================================================================\n";

        return ss.str();
    }

    inline size_t getAttachments(const vmime::shared_ptr<vmime::net::message>& message, const std::set<std::string>& mimes, accessControlAction accessControl, std::vector<std::unique_ptr<utilities::temporaryFile>>& files)
    {
        size_t count = 0;
        auto pm = message->getParsedMessage();
        if (!pm) return 0;
        std::vector <vmime::shared_ptr<const vmime::attachment> > attchs = vmime::attachmentHelper::findAttachmentsInMessage(pm);

        if (!attchs.empty())
        {
            for (const auto& att : attchs)
            {
                // Check for accepted/blocked mime types
                std::string mime = att->getType().getType() + "/" + att->getType().getSubType();
                if (mimes.contains(mime))
                {
                    if (accessControl == accessControlAction::block) continue;
                }
                else
                {
                    if (accessControl == accessControlAction::allow) continue;
                }

                // Get attachment size
                vmime::size_t size = 0;

                if (att->getData()->isEncoded())
                {
                    size = att->getData()->getEncoding().getEncoder()->getDecodedSize(att->getData()->getLength());
                }
                else
                {
                    size = att->getData()->getLength();
                }

                auto temporary_file = std::make_unique<utilities::temporaryFile>();
                vmime::shared_ptr <vmime::utility::fileSystemFactory> fsf = vmime::platform::getHandler()->getFileSystemFactory();
                vmime::shared_ptr <vmime::utility::file> file = fsf->create(vmime::utility::path::fromString(temporary_file->string(), "/", vmime::charsets::UTF_8));
                file->createFile();
                vmime::shared_ptr <vmime::utility::outputStream> output = file->getFileWriter()->getOutputStream();
                att->getData()->extract(*output);
                files.push_back(std::move(temporary_file));
            }
        }

        return files.size();
    }

    inline bool search_for_email_address(const std::string& address1, const vmime::shared_ptr<const vmime::addressList>& addressList1, const vmime::shared_ptr<const vmime::addressList>& addressList2)
    {
        if (addressList1)
        {
            for (const auto& a : addressList1->getAddressList())
            {
                auto mb = vmime::dynamicCast<const vmime::mailbox>(a);
                if (boost::iequals(address1, mb->getEmail().toString())) return true;
            }
        }

        if (addressList2)
        {
            for (const auto& a : addressList2->getAddressList())
            {
                auto mb = vmime::dynamicCast<const vmime::mailbox>(a);
                if (boost::iequals(address1, mb->getEmail().toString())) return true;
            }
        }

        return false;
    }

    inline std::string getFormattedDuration(double seconds)
    {
        std::stringstream ss;
        int millennia = 0;
        int centuries = 0;
        int decades = 0;
        int years = 0;
        int months = 0;
        int weeks = 0;
        int days = 0;
        int hours = 0;
        int minutes = 0;
        int deciseconds = 0;
        int centiseconds = 0;
        int milliseconds = 0;
        int microseconds = 0;
        int nanoseconds = 0;
        int picoseconds = 0;
        int femtoseconds = 0;
        int attoseconds = 0;
        int zeptoseconds = 0;
        int yoctoseconds = 0;

        while (seconds >= 31536000000)
        {
            seconds -= 31536000000;
            millennia++;
        }
        while (seconds >= 3155760000)
        {
            seconds -= 3155760000;
            centuries++;
        }
        while (seconds >= 315576000)
        {
            seconds -= 315576000;
            decades++;
        }
        while (seconds >= 31557600)
        {
            seconds -= 31557600;
            years++;
        }
        while (seconds >= 2629800)
        {
            seconds -= 2629800;
            months++;
        }
        while (seconds >= 604800)
        {
            seconds -= 604800;
            weeks++;
        }
        while (seconds >= 86400)
        {
            seconds -= 86400;
            days++;
        }
        while (seconds >= 3600)
        {
            seconds -= 3600;
            hours++;
        }
        while (seconds >= 60)
        {
            seconds -= 60;
            minutes++;
        }
        if (millennia == 1) ss << " millennium, ";
        if (millennia > 1) ss << millennia << " millennia, ";
        if (centuries == 1) ss << "1 century, ";
        if (centuries > 1) ss << centuries << " centuries, ";
        if (decades == 1) ss << "1 decade, ";
        if (decades > 1) ss << decades << " decades, ";
        if (years == 1) ss << "1 year, ";
        if (years > 1) ss << years << " years, ";
        if (months == 1) ss << "1 month, ";
        if (months > 1) ss << months << " months, ";
        if (weeks == 1) ss << "1 week, ";
        if (weeks > 1) ss << weeks << " weeks, ";
        if (days == 1) ss << "1 day, ";
        if (days > 1) ss << days << " days, ";
        if (hours == 1) ss << "1 hour, ";
        if (hours > 1) ss << hours << " hours, ";
        if (minutes == 1) ss << "1 minute, ";
        if (minutes > 1) ss << minutes << " minutes, ";
        if (seconds == 1) ss << "1 second, ";
        if (seconds > 1) ss << seconds << " seconds, ";

        while (seconds <= 0.000000000000000000000001)
        {
            seconds += 0.000000000000000000000001;
            yoctoseconds++;
        }
        while (seconds <= 0.000000000000000000001)
        {
            seconds += 0.000000000000000000001;
            zeptoseconds++;
        }
        while (seconds <= 0.000000000000000001)
        {
            seconds += 0.000000000000000001;
            attoseconds++;
        }
        while (seconds <= 0.000000000000001)
        {
            seconds += 0.000000000000001;
            femtoseconds++;
        }
        while (seconds <= 0.000000000001)
        {
            seconds += 0.000000000001;
            picoseconds++;
        }
        while (seconds <= 0.000000001)
        {
            seconds += 0.000000001;
            nanoseconds++;
        }
        while (seconds <= 0.000001)
        {
            seconds += 0.000001;
            microseconds += 1;
        }
        while (seconds <= 0.001)
        {
            seconds += 0.001;
            milliseconds += 1;
        }
        while (seconds <= 0.01)
        {
            seconds += 0.01;
            centiseconds += 1;
        }
        while (seconds <= 0.1)
        {
            seconds += 0.1;
            deciseconds += 1;
        }

        std::string out = ss.str();
        auto lc = out.find_last_of(',');
        out.erase(lc);
        lc = out.find_last_of(',');
        if (lc != std::string::npos) out.replace(lc, 2, " and ");

        return out;
    }

    inline std::string getFormattedDuration(const boost::posix_time::time_duration& duration)
    {
        return getFormattedDuration(static_cast<double>(duration.total_nanoseconds()) / 1000000000.00);
    }
}

#endif // UTILS_HPP
