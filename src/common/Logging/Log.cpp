/*
 * Copyright (C) 2008-2019 TrinityCore <https://www.trinitycore.org/>
 * Copyright (C) 2005-2008 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Log.h"
#include "Config.h"
#include "Util.h"
#include "Poco/ConsoleChannel.h"
#include "Poco/FormattingChannel.h"
#include "Poco/PatternFormatter.h"
#include "Poco/SplitterChannel.h"
#include "Poco/FileChannel.h"
#include "Poco/Logger.h"
#include "Poco/AutoPtr.h"
#include "Poco/Path.h"
#include <sstream>

using namespace Poco;

Log::Log()
{

}

Log::~Log()
{

}

Log* Log::instance()
{
    static Log instance;
    return &instance;
}

void Log::Initialize()
{
    LoadFromConfig();
}

void Log::InitColorsForConsoleLogger(std::string ListColors)
{
    if (ListColors.empty())
    {
        _Colored = false;
        return;
    }

    int color[LOG_LEVEL_MAX];

    std::istringstream ss(ListColors);

    for (uint8 i = 1; i < LOG_LEVEL_MAX; ++i)
    {
        ss >> color[i];

        if (!ss)
            return;

        if (color[i] < 0 || color[i] >= COLOR_TYPE_END)
            return;
    }

    for (uint8 i = 0; i < LOG_LEVEL_MAX; ++i)
        _Colors[i] = ColorTypes(color[i]);

    _Colored = true;
}

ColorTypes Log::GetColorForLevel(LogLevel Level)
{
    if (_Colored)
        return _Colors[Level];

    return ColorTypes::WHITE;
}

void Log::_Write(std::string const& filter, LogLevel const level, std::string const& message)
{
    if (!ShouldLog(filter, level))
        return;

    bool stdout_stream = (level != LOG_LEVEL_ERROR || level != LOG_LEVEL_FATAL || level != LOG_LEVEL_CRITICAL);

    Logger& logger = Logger::get(filter);

    try
    {
        if (IsColored())
            SetColor(stdout_stream, GetColorForLevel(level));

        switch (level)
        {
        case LOG_LEVEL_FATAL:
            logger.fatal(message);
            break;
        case LOG_LEVEL_CRITICAL:
            logger.critical(message);
            break;
        case LOG_LEVEL_ERROR:
            logger.error(message);
            break;
        case LOG_LEVEL_WARNING:
            logger.warning(message);
            break;
        case LOG_LEVEL_NOTICE:
            logger.notice(message);
            break;
        case LOG_LEVEL_INFO:
            logger.information(message);
            break;
        case LOG_LEVEL_DEBUG:
            logger.debug(message);
            break;
        case LOG_LEVEL_TRACE:
            logger.trace(message);
            break;
        default:
            break;
        }

        if (IsColored())
            ResetColor(stdout_stream);
    }
    catch (const std::exception& e)
    {
        outError("%s", e.what());
    }
}

std::string Log::GetDynamicFileName(std::string ChannelName, std::string Arg)
{
    Tokenizer tokens(sConfigMgr->GetStringDefault("LogChannel." + ChannelName, ""), ',');

    // If bad file name or bad config option
    if (tokens.size() < 3)
        return "";

    char namebuf[WARHEAD_PATH_MAX];
    snprintf(namebuf, WARHEAD_PATH_MAX, tokens[3], Arg.c_str());

    return namebuf;
}

FormattingChannel* Log::GetFileChannel(std::string ChannelName)
{
    if (_ChannelMapFiles.count(ChannelName))
        return _ChannelMapFiles[ChannelName];

    return nullptr;
}

FormattingChannel* Log::GetConsoleChannel()
{
    if (_ChannelMapConsole.count(_consoleChannel))
        return _ChannelMapConsole[_consoleChannel];

    return nullptr;
}

void Log::AddConsoleChannel(std::string ChannelName, FormattingChannel* channel)
{
    if (_ChannelMapConsole.count(ChannelName))
        return;

    _ChannelMapConsole[ChannelName] = channel;
}

void Log::AddFileChannel(std::string ChannelName, FormattingChannel* channel)
{
    if (_ChannelMapFiles.count(ChannelName))
        return;

    _ChannelMapFiles[ChannelName] = channel;
}

void Log::ClearnAllChannels()
{
    _ChannelMapFiles.clear();
    _ChannelMapConsole.clear();
}

std::string Log::GetChannelFromLogger(std::string LoggerName)
{
    std::string Temp = LoggerName;
    if (Temp.empty())
        Temp = "root";

    std::string LoggerOption = sConfigMgr->GetStringDefault("Logger." + Temp, "6,Server");

    Tokenizer tokens(LoggerOption, ',');

    if (tokens.size() >= 1)
        return tokens[1];

    return "";
}

void Log::_writeCommand(std::string const message, std::string const accountid)
{
    std::string LoggerGMCommands = "commands.gm";
    std::string GMChannelName = GetChannelFromLogger(LoggerGMCommands);

    // If dynamic file for GM commands
    if (!GetFileChannel(GMChannelName))
    {
        std::string LoggerOption = sConfigMgr->GetStringDefault("LogChannel." + GMChannelName, "");
        if (LoggerOption.empty())
            return;

        Tokenizer tokens(LoggerOption, ',');
        if (tokens.size() < 2)
            return;

        // Get options
        std::string Times = GetPositionOptions(LoggerOption, 1);
        std::string Pattern = GetPositionOptions(LoggerOption, 2);
        std::string RotateOnOpen = GetPositionOptions(LoggerOption, 4);
        std::string Rotation = GetPositionOptions(LoggerOption, 5);
        std::string Archive = GetPositionOptions(LoggerOption, 6);

        // Get filename
        std::string DynamicFileName = GetDynamicFileName(GMChannelName, accountid);

        // Configuration pattern channel        
        AutoPtr<PatternFormatter> _pattern(new PatternFormatter);
        
        try
        {
            _pattern->setProperty("pattern", Pattern);
            _pattern->setProperty("times", Times);
        }
        catch (const std::exception& e)
        {
            outError("%s\n", e.what());
        }

        // Configuration file channel
        AutoPtr<FileChannel> _channel(new FileChannel);

        try
        {
            _channel->setProperty("path", m_logsDir + DynamicFileName);
            _channel->setProperty("times", Times);

            if (!RotateOnOpen.empty())
                _channel->setProperty("rotateOnOpen", RotateOnOpen);

            if (!Rotation.empty())
                _channel->setProperty("rotation", Rotation);

            if (!Archive.empty())
                _channel->setProperty("archive", Archive);
        }
        catch (const std::exception& e)
        {
            outError("%s\n", e.what());
        }

        AutoPtr<SplitterChannel> SplitShannel(new SplitterChannel);

        try
        {
            SplitShannel->addChannel(new FormattingChannel(_pattern, _channel));
            SplitShannel->addChannel(GetConsoleChannel()->getChannel());
        }
        catch (const std::exception& e)
        {
            outError("%s\n", e.what());
        }

        try
        {
            Logger::create("commands.gm.dynamic", SplitShannel);
            _Write("commands.gm.dynamic", LOG_LEVEL_INFO, message);
            Logger::destroy("commands.gm.dynamic");
        }
        catch (const std::exception& e)
        {
            outError("%s", e.what());
        }
    }
    else
    {
        try
        {
            Logger::get("commands.gm").information(message);
        }
        catch (const std::exception& e)
        {
            outError("%s", e.what());
        }
    }
}

void Log::SetColor(bool stdout_stream, ColorTypes color)
{
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
    static WORD WinColorFG[COLOR_TYPE_END] =
    {
        0,                                                                                                  // BLACK
        FOREGROUND_RED,                                                                                     // RED
        FOREGROUND_GREEN,                                                                                   // GREEN
        FOREGROUND_RED      | FOREGROUND_GREEN,                                                             // BROWN
        FOREGROUND_BLUE,                                                                                    // BLUE
        FOREGROUND_RED      | FOREGROUND_BLUE,                                                              // MAGENTA
        FOREGROUND_GREEN    | FOREGROUND_BLUE,                                                              // CYAN
        FOREGROUND_RED      | FOREGROUND_GREEN      | FOREGROUND_BLUE,                                      // WHITE
        FOREGROUND_RED      | FOREGROUND_GREEN      | FOREGROUND_INTENSITY,                                 // YELLOW
        FOREGROUND_RED      | FOREGROUND_INTENSITY,                                                         // RED_BOLD
        FOREGROUND_GREEN    | FOREGROUND_INTENSITY,                                                         // GREEN_BOLD
        FOREGROUND_BLUE     | FOREGROUND_INTENSITY,                                                         // BLUE_BOLD
        FOREGROUND_RED      | FOREGROUND_BLUE       | FOREGROUND_INTENSITY,                                 // MAGENTA_BOLD
        FOREGROUND_GREEN    | FOREGROUND_BLUE       | FOREGROUND_INTENSITY,                                 // CYAN_BOLD
        FOREGROUND_RED      | FOREGROUND_GREEN      | FOREGROUND_BLUE           | FOREGROUND_INTENSITY      // WHITE_BOLD
    };

    HANDLE hConsole = GetStdHandle(stdout_stream ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
    SetConsoleTextAttribute(hConsole, WinColorFG[color]);
#else
    enum ANSITextAttr
    {
        TA_NORMAL = 0,
        TA_BOLD = 1,
        TA_BLINK = 5,
        TA_REVERSE = 7
    };

    enum ANSIFgTextAttr
    {
        FG_BLACK = 30, FG_RED, FG_GREEN, FG_BROWN, FG_BLUE,
        FG_MAGENTA, FG_CYAN, FG_WHITE, FG_YELLOW
    };

    enum ANSIBgTextAttr
    {
        BG_BLACK = 40, BG_RED, BG_GREEN, BG_BROWN, BG_BLUE,
        BG_MAGENTA, BG_CYAN, BG_WHITE
    };

    static uint8 UnixColorFG[COLOR_TYPE_END] =
    {
        FG_BLACK,                                           // BLACK
        FG_RED,                                             // RED
        FG_GREEN,                                           // GREEN
        FG_BROWN,                                           // BROWN
        FG_BLUE,                                            // BLUE
        FG_MAGENTA,                                         // MAGENTA
        FG_CYAN,                                            // CYAN
        FG_WHITE,                                           // WHITE
        FG_YELLOW,                                          // YELLOW
        FG_RED,                                             // LRED
        FG_GREEN,                                           // LGREEN
        FG_BLUE,                                            // LBLUE
        FG_MAGENTA,                                         // LMAGENTA
        FG_CYAN,                                            // LCYAN
        FG_WHITE                                            // LWHITE
    };

    fprintf((stdout_stream ? stdout : stderr), "\x1b[%d%sm", UnixColorFG[color], (color >= YELLOW && color < COLOR_TYPE_END ? ";1" : ""));
#endif
}

void Log::ResetColor(bool stdout_stream)
{
#if WARHEAD_PLATFORM == WARHEAD_PLATFORM_WINDOWS
    HANDLE hConsole = GetStdHandle(stdout_stream ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
#else
    fprintf(( stdout_stream ? stdout : stderr ), "\x1b[0m");
#endif
}

void Log::outMessage(std::string const& filter, LogLevel const level, std::string&& message)
{
    _Write(filter, level, message);
}

void Log::outCommand(std::string&& message, std::string&& AccountID)
{
    _writeCommand(message, AccountID);
}

void Log::CreateLogger(std::string Name, LogLevel level, std::string FileChannelName)
{
    AutoPtr<SplitterChannel> _SC(new SplitterChannel);

    //Add console channel (default)
    _SC->addChannel(GetConsoleChannel());

    // Add file channel
    if (GetFileChannel(FileChannelName))
        _SC->addChannel(GetFileChannel(FileChannelName));

    try
    {
        Logger::create(Name, _SC, (uint8)level);
    }
    catch (const std::exception& e)
    {
        outError("%s", e.what());
    }
}

void Log::CreateLoggerFromConfig(std::string const& ConfigLoggerName)
{
    if (ConfigLoggerName.empty())
        return;

    enum LoggerOptions
    {
        LOGGER_OPTIONS_LOG_LEVEL,
        LOGGER_OPTIONS_CHANNEL_NAME
    };

    std::string options = sConfigMgr->GetStringDefault(ConfigLoggerName, "");
    std::string LoggerName = ConfigLoggerName.substr(7);
    if (LoggerName == "root")
        LoggerName = "";

    if (Logger::has(LoggerName))
    {
        outError("Is exist - %s\n", LoggerName.c_str());
        return;
    }

    if (options.empty())
    {
        outError("Log::CreateLoggerFromConfig: Missing config option Logger.%s\n", LoggerName.c_str());
        return;
    }

    Tokenizer tokens(options, ',');

    if (!tokens.size() || tokens.size() > LOGGER_OPTIONS_CHANNEL_NAME + 1)
    {
        //fprintf(stderr, "Log::CreateLoggerFromConfig: Wrong config option Logger.%s=%s\n", LoggerName.c_str(), options.c_str());
        outError("Bad config for Logger (%s)", LoggerName.c_str());
        return;
    }

    LogLevel level = LogLevel(atoi(GetPositionOptions(options, LOGGER_OPTIONS_LOG_LEVEL).c_str()));
    if (level >= LOG_LEVEL_MAX)
    {
        //fprintf(stderr, "Log::CreateLoggerFromConfig: Wrong Log Level for logger %s\n", LoggerName.c_str());
        outError("Log::CreateLoggerFromConfig: Wrong Log Level for logger %s", LoggerName.c_str());
        return;
    }

    std::string FileChannelName = GetPositionOptions(options, LOGGER_OPTIONS_CHANNEL_NAME);

    CreateLogger(LoggerName, level, FileChannelName);
}

std::string Log::GetPositionOptions(std::string Options, uint8 Position)
{
    Tokenizer tokens(Options, ',');
    if (tokens.size() < Position + 1)
        return "";

    return tokens[Position];
}

void Log::CreateChannelsFromConfig(std::string const& LogChannelName)
{
    if (LogChannelName.empty())
        return;

    enum ChannelOptions
    {
        CHANNEL_OPTIONS_TYPE,
        CHANNEL_OPTIONS_TIMES,
        CHANNEL_OPTIONS_PATTERN,
        CHANNEL_OPTIONS_OPTION_1,
        CHANNEL_OPTIONS_OPTION_2,
        CHANNEL_OPTIONS_OPTION_3,
        CHANNEL_OPTIONS_OPTION_4
    };

    enum ChannelOptionsType
    {
        CHANNEL_OPTIONS_TYPE_CONSOLE = 1,
        CHANNEL_OPTIONS_TYPE_FILE
    };

    std::string options = sConfigMgr->GetStringDefault(LogChannelName, "");
    std::string ChannelName = LogChannelName.substr(11);
    
    if (options.empty())
    {
        outError("Log::CreateLoggerFromConfig: Missing config option LogChannel.%s", ChannelName.c_str());
        return;
    }

    Tokenizer tokens(options, ',');

    if (tokens.size() < CHANNEL_OPTIONS_PATTERN + 1)
    {
        //fprintf(stderr, "Log::CreateLoggerFromConfig: Wrong config option Logger.%s=%s\n", LoggerName.c_str(), options.c_str());
        outError("Log::CreateLoggerFromConfig: Wrong config option (< CHANNEL_OPTIONS_PATTERN) LogChannel.%s=%s\n", ChannelName.c_str(), options.c_str());
        return;
    }

    if (tokens.size() > CHANNEL_OPTIONS_OPTION_4 + 1)
    {
        //fprintf(stderr, "Log::CreateLoggerFromConfig: Wrong config option Logger.%s=%s\n", LoggerName.c_str(), options.c_str());
        outError("Log::CreateLoggerFromConfig: Wrong config option (> CHANNEL_OPTIONS_OPTION_4) LogChannel.%s=%s\n", ChannelName.c_str(), options.c_str());
        return;
    }

    uint8 ChannelType = atoi(GetPositionOptions(options, CHANNEL_OPTIONS_TYPE).c_str());
    if (ChannelType > CHANNEL_OPTIONS_TYPE_FILE)
    {
        outError("Log::CreateLoggerFromConfig: Wrong channel type for LogChannel.%s\n", ChannelName.c_str());
        return;
    }

    std::string Times = GetPositionOptions(options, CHANNEL_OPTIONS_TIMES);
    if (Times.empty())
    {
        outError("Log::CreateLoggerFromConfig: Empty times for LogChannel.%s", ChannelName.c_str());
        return;
    }
    
    std::string Pattern = GetPositionOptions(options, CHANNEL_OPTIONS_PATTERN);
    if (Pattern.empty())
    {
        outError("Log::CreateLoggerFromConfig: Empty pattern for LogChannel.%s", ChannelName.c_str());
        return;
    }

    // Start configuration pattern channel
    AutoPtr<PatternFormatter> _pattern(new PatternFormatter);

    try
    {
        _pattern->setProperty("pattern", Pattern);
        _pattern->setProperty("times", Times);
    }
    catch (const std::exception& e)
    {
        outError("%s\n", e.what());
    }

    if (ChannelType == CHANNEL_OPTIONS_TYPE_CONSOLE)
    {
        // Configuration console channel
        AutoPtr<ConsoleChannel> _channel(new ConsoleChannel);

        // Init Colors
        InitColorsForConsoleLogger(GetPositionOptions(options, CHANNEL_OPTIONS_OPTION_1));

        if (_consoleChannel.empty())
        {
            _consoleChannel = ChannelName;

            AddConsoleChannel(ChannelName, new FormattingChannel(_pattern, _channel));
        }
    }
    else if (ChannelType == CHANNEL_OPTIONS_TYPE_FILE)
    {
        if (tokens.size() < 4)
        {
            outError("Bad file name for LogChannel.%s", ChannelName.c_str());
            return;
        }

        std::string FileName = GetPositionOptions(options, CHANNEL_OPTIONS_OPTION_1);
        std::string RotateOnOpen = GetPositionOptions(options, CHANNEL_OPTIONS_OPTION_2);
        std::string Rotation = GetPositionOptions(options, CHANNEL_OPTIONS_OPTION_3);
        std::string Archive = GetPositionOptions(options, CHANNEL_OPTIONS_OPTION_4);

        if (FileName.find("%s") == std::string::npos)
        {
            // Configuration file channel
            AutoPtr<FileChannel> _channel(new FileChannel);

            try
            {
                _channel->setProperty("path", m_logsDir + FileName);
                _channel->setProperty("times", Times);

                if (!RotateOnOpen.empty())
                    _channel->setProperty("rotateOnOpen", RotateOnOpen);

                if (!Rotation.empty())
                    _channel->setProperty("rotation", Rotation);

                if (!Archive.empty())
                    _channel->setProperty("archive", Archive);
            }
            catch (const std::exception& e)
            {
                outError("%s\n", e.what());
            }

            AddFileChannel(ChannelName, new FormattingChannel(_pattern, _channel));
        }
    }
}

void Log::ReadLoggersFromConfig()
{
    std::vector<std::string> keys = sConfigMgr->GetKeysByString("Logger.");
    for (std::string const& loggerName : keys)
        CreateLoggerFromConfig(loggerName);
}

void Log::ReadChannelsFromConfig()
{
    std::vector<std::string> keys = sConfigMgr->GetKeysByString("LogChannel.");
    for (std::string const& channelName : keys)
        CreateChannelsFromConfig(channelName);
}

void Log::LoadFromConfig()
{
    Logger::shutdown();

    ClearnAllChannels();
    InitLogsDir();
    ReadChannelsFromConfig();
    ReadLoggersFromConfig();    
}

void Log::InitLogsDir()
{
    m_logsDir = sConfigMgr->GetStringDefault("LogsDir", "");

    if (!m_logsDir.empty())
        if ((m_logsDir.at(m_logsDir.length() - 1) != '/') && (m_logsDir.at(m_logsDir.length() - 1) != '\\'))
            m_logsDir.push_back('/');

    Path path(m_logsDir);

    if (!path.isDirectory())
        m_logsDir = "";
}

bool Log::ShouldLog(std::string const& type, LogLevel level) const
{
    LogLevel logLevel = (LogLevel)Logger::get(type).getLevel();
    return logLevel != LOG_LEVEL_DISABLED && logLevel >= level;
}

void Log::outError(std::string&& message)
{
    sLog->SetColor(false, ColorTypes::RED);
    printf("%s\n", message.c_str());
    sLog->ResetColor(false);
}

void Log::outCharDump(char const* str, uint32 accountId, uint64 guid, char const* name)
{
    if (!str)
        return;

    _Write("entities.player.dump", LOG_LEVEL_INFO, Warhead::StringFormat("== START DUMP ==\n(Account: %u. Guid: %u. Name: %s)\n%s\n== END DUMP ==\n", accountId, guid, name, str));
}
