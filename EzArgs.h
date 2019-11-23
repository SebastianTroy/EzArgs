#ifndef EZARGS_H
#define EZARGS_H

#include <string>
#include <functional>
#include <map>
#include <sstream>
#include <iostream>

namespace EzArgs {

enum class Error {
    None,
    ExpectedShortAlias,
    ExpectedAliasIndicator,
    OptionHasNoAliases,
    AliasClash,
    EmptyAlias,
    UnrecognisedAlias,
    ExpectedArgumentValue,
    UnExpectedArgumentValue,
    UnrecognisedArgumentValue,
    NullOptionAction,
    CustomError_1 = 100,
};

/**
 * Used when setting Options and parsing args.
 *
 * @param where Points to the arg or Option where the error was detected.
 */
using ErrorHandler = std::function<void(Error, const std::string& where)>;

/**
 * Should return false if any error is encountered while parsing. Sould not
 * modify valueOut if false is returned. Should set valueOut to a valid value
 * if true is returned.
 */
template <typename T>
using ArgValueParser = const std::function<Error(const std::string& argString, T& valueOut)>;

/**
 * Should return a vector of {index, aliases, value} objects, and a vector of
 * positional args. Just make sure the format is correct, don't check for
 * duplicates or unrecognised arguments or values here.
 */
using ParsedArg = std::tuple<int, std::string, std::string>;
using CmdLineParser = std::function<std::tuple<std::vector<ParsedArg>, std::vector<std::string>>(int argc, char** argv, const ErrorHandler& errorFunc_)>;

/**
 * When an option is tri
 */
using OptionAction = std::function<Error(std::string toParse)>;

/**
 * @brief The Option struct represents a thing you'd like to do in response to a
 *        argument specified when your program is run
 *
 * @param aliases_ A string representing the option's name(s), for example using
 *              Posix style arg parsing "h,H,help" would result in this options
 *              action being triggered by the presence of "-h", "-H" or
 *              "--help". The names are comma seperated and any number of names
 *              can be specified.
 *
 * @param onParse_ Use this to do something when this option is specified. A
 *                number of helper functions have been provided for common use
 *                cases such as setting values, e.g DetectPresence(bEnableThing)
 *
 * @param helpText_ Use this text to describe usage of this Option.
 */
struct Option {
    const std::string aliases_;
    const OptionAction onParse_;
    const std::string helpText_;
};

// Private namespace for hidden internal helpers
namespace {

template <typename T>
inline ArgValueParser<T> GetDefaultParser()
{
    return [](const std::string& argString, T& valueOut) -> Error
    {
        auto stream = std::stringstream(argString);
        T temp;
        stream >> temp;
        if (!stream.fail() && stream.eof()) {
            valueOut = temp;
            return Error::None;
        } else {
            return Error::UnExpectedArgumentValue;
        }
    };
}

template <>
inline ArgValueParser<bool> GetDefaultParser()
{
    return [](const std::string& argString, bool& valueOut) -> Error
    {
        if (argString == "true") {
            valueOut = true;
            return Error::None;
        } else if (argString == "false") {
            valueOut = false;
            return Error::None;
        } else if (argString.empty()) {
            return Error::ExpectedArgumentValue;
        }
        return Error::UnrecognisedArgumentValue;
    };
}

template <>
inline ArgValueParser<std::string> GetDefaultParser()
{
    return [](const std::string& argString, std::string& valueOut) -> Error
    {
        valueOut = argString;
        return Error::None;
    };
}

static std::vector<std::string> ParseAliases(const std::string& aliases)
{
    std::vector<std::string> segments;
    std::string::size_type index = 0;
    while (index < aliases.size()) {
        auto first = index;
        auto last = aliases.find_first_of(',', index);
        std::string segment = aliases.substr(first, last - first);
        index += segment.size() + 1;
        segments.push_back(segment);
    }
    return segments;
}

} // end private namespace

template <typename T>
inline OptionAction ParseRequiredValue(T& valueOut, ArgValueParser<T>& parser = GetDefaultParser<T>())
{
    return [&](std::string argValue) -> Error
    {
        if (argValue.empty()) {
            return Error::ExpectedArgumentValue;
        } else {
            return parser(argValue, valueOut);
        }
    };
}

template <typename T>
inline OptionAction ParseOptionalValue(std::optional<T>& valueOut, ArgValueParser<T>& parser = GetDefaultParser<T>())
{
    return [&](std::string argValue) -> Error
    {
        if (argValue.empty()) {
            valueOut = {};
            return Error::None;
        } else {
            T temp;
            Error error = parser(argValue, temp);
            if (error == Error::None) {
                valueOut = temp;
            }
            return error;
        }
    };
}

inline OptionAction DetectPresence(bool& valueOut)
{
    valueOut = false;
    return [&](std::string argValue) -> Error
    {
        valueOut = true;
        return argValue.empty() ? Error::None : Error::UnExpectedArgumentValue;
    };
}

std::string PointToArg(int argc, char** argv, int argcToPointTo)
{
    std::stringstream stream;
    unsigned pointerIndex = 0;
    for (int i = 0; i < argc; i++) {
        std::string arg(argv[i]);
        if (i < argcToPointTo) {
            pointerIndex += arg.size() + 1;
        }
        stream << arg << " ";
    }
    stream << std::endl << std::string(pointerIndex, ' ') << "^";
    return stream.str();
}

std::string PointToOptions(const std::vector<Option>& options, std::vector<unsigned> indexesToPointTo)
{
    std::stringstream stream;
    for (unsigned currentIndex = 0; currentIndex < options.size(); currentIndex++) {
        const auto& option = options[currentIndex];
        bool indicate = false;
        if (std::find(indexesToPointTo.cbegin(), indexesToPointTo.cend(), currentIndex) != indexesToPointTo.cend()) {
            indicate = true;
        }
        stream << (indicate ? "--> " : "") << "{ " << option.aliases_ << ", " << option.helpText_ << " }" << std::endl;
    }
    return stream.str();
}

class ArgParser {
public:
    ArgParser()
    {
        errorFunc_ = [&](Error error, const std::string& where) -> void {
            std::cout << where << std::endl;
            switch (error) {
            case Error::None :
                std::cout << "No Error (This should never be printed!)" << std::endl;
                break;
            case Error::ExpectedShortAlias :
                std::cout << "Expected at least one character following the short alias token." << std::endl;
                break;
            case Error::ExpectedAliasIndicator :
                std::cout << "Expected the arg to begin with a short or long alias token." << std::endl;
                break;
            case Error::OptionHasNoAliases :
                std::cout << "Please specify at least one short or long alias for this option, e.g. \"h,H,help\" specifies three aliases for the help option." << std::endl;
                break;
            case Error::AliasClash :
                std::cout << "Each Option must have unique aliases, they cannot share long or short aliases." << std::endl;
                break;
            case Error::EmptyAlias :
                std::cout << "Cannot use ',' as an alias, nor can an alias be an empty string." << std::endl;
                break;
            case Error::UnrecognisedAlias :
                std::cout << "This alias was not recognised, please check help to see available options." << std::endl;
                break;
            case Error::ExpectedArgumentValue :
                std::cout << "Option requires an argument value, e.g. \"alias=value\" or \"alias value\"." << std::endl;
                break;
            case Error::UnExpectedArgumentValue :
                std::cout << "Option does not accept an argument value." << std::endl;
                break;
            case Error::UnrecognisedArgumentValue :
                std::cout << "Option couldn't parse the specified argument value." << std::endl;
                break;
            case Error::NullOptionAction :
                std::cout << "Option's action is null, please specify a valid action for the Option." << std::endl;
                break;
            default :
                std::cout << "Custom Error code detected, consider specifying a custom ErrorHandler function." << std::endl;
                break;
            }
            this->StopParsing();
        };

        argsParser_ = [](int argc, char** argv, const ErrorHandler& errorFunc_) -> std::tuple<std::vector<std::tuple<int, std::string, std::string>>, std::vector<std::string>>
        {
            int index = 1;

            std::vector<std::tuple<int, std::string, std::string>> argsAndValues;
            for (; index < argc; index++) {
                const std::string arg = argv[index];

                if (arg.substr(0, 2) == "--") {
                    if (arg.size() == 2) {
                        // terminator "--" found, all other args are positional
                        break;
                    } else {
                        auto aliasFirst = 2u;
                        auto aliasLast = arg.find_first_of('=');
                        std::string alias = arg.substr(aliasFirst, aliasLast - aliasFirst);
                        std::string value;
                        if (aliasLast != arg.npos) {
                            auto valueFirst = aliasLast + 1;
                            auto valueLast = arg.npos;
                            value = arg.substr(valueFirst, valueLast - valueFirst);
                        }
                        argsAndValues.push_back({ index, alias, value });
                    }
                } else if (arg.substr(0, 1) == "-") {
                    if (arg.size() == 1 || arg.at(1) == '=') {
                        errorFunc_(Error::ExpectedShortAlias, PointToArg(argc, argv, index));
                    } else {
                        for (const char& shortAlias : arg.substr(1, arg.npos)) {
                            if (shortAlias == '=') {
                                auto& [index, alias, value] = argsAndValues.back();
                                (void)index;
                                (void)alias;
                                auto first = arg.find_first_of('=') + 1;
                                auto last = arg.npos;
                                value = arg.substr(first, last - first);
                                break;
                            } else if(std::isalpha(shortAlias)) {
                                argsAndValues.push_back({ index, {shortAlias}, "" });
                            } else {
                                errorFunc_(Error::ExpectedShortAlias, PointToArg(argc, argv, index));
                            }
                        }
                    }
                } else if (!argsAndValues.empty()) {
                    auto& [index, alias, value] = argsAndValues.back();
                    (void) index;
                    (void) alias;
                    if (value.empty()) {
                        value = arg;
                    } else {
                        errorFunc_(Error::ExpectedAliasIndicator, PointToArg(argc, argv, index));
                    }
                } else if (index == 1) {
                    // No Alias indicator found, all args are positional
                    break;
                } else {
                    errorFunc_(Error::ExpectedAliasIndicator, PointToArg(argc, argv, index));
                }
            }

            std::vector<std::string> positionalArgs;
            for (; index < argc; index++) {
                positionalArgs.push_back(std::string(argv[index]));
            }

            return { argsAndValues, positionalArgs };
        };
    }

    void SetErrorHandler(ErrorHandler&& errorHandler)
    {
        errorFunc_ = std::move(errorHandler);
    }
    void SetCommandLineParser(CmdLineParser&& commandLineParser)
    {
        argsParser_ = std::move(commandLineParser);
    }

    bool SetOptions(std::vector<Option>&& options)
    {
        options_ = std::move(options);
        aliasMap_.clear();

        bool success = true;
        for (unsigned currentIndex = 0; currentIndex < options_.size() && success; currentIndex++) {
            const auto& option = options_[currentIndex];

            if (option.aliases_.empty()) {
                errorFunc_(Error::OptionHasNoAliases, PointToOptions(options_, { currentIndex }));
                success = false;
                break;
            }

            if (option.onParse_ == nullptr) {
                errorFunc_(Error::NullOptionAction, PointToOptions(options_, { currentIndex }));
                success = false;
                break;
            }


            for (const auto& alias : ParseAliases(option.aliases_)) {
                if (aliasMap_.count(alias) > 0) {
                    errorFunc_(Error::AliasClash, PointToOptions(options_, { currentIndex, aliasMap_.at(alias) }));
                    success = false;
                    break;
                } else {
                    if (alias.empty()) {
                        errorFunc_(Error::EmptyAlias, PointToOptions(options_, { currentIndex }));
                        success = false;
                        break;
                    } else {
                        aliasMap_[alias] = currentIndex;
                    }
                }
            }
        }

        if (!success) {
            options_.clear();
            aliasMap_.clear();
        }

        return success;
    }

    void PrintHelpTable(std::ostream& out = std::cout, std::string additionalHelpText = "")
    {
        unsigned longestAlias = 0;
        for (const auto& [aliases, action, helpText] : options_) {
            (void) action;
            (void) helpText;
            longestAlias = std::max(longestAlias, aliases.size());
        }
        // work out longest alias,
        // work out if we are line wrapping
        // print table

        // TODO work out if there is a way to ascertain optional/required... from the function provided (lambda capture means there might be fuckery though so we can't guaruntee the tests aren't messing with users code)


        out << additionalHelpText;
    }

    /**
     * ParseArgs parses the arguments in the order they were specified on the
     * comand line. Once an option has been actioned, it will not be actioned
     * again, even if a different alias for the same option was used or a
     * different value was provided.
     *
     * @param argc The number of string arguments, take this unmodified from
     *             main.
     *
     * @param argv The string arguments to parse, take this unmodified from
     *             main.
     *
     * @param parser By default assumes a Posix style "-n" for short args,
     *               "--name" for long args and all space seperated strings
     *               after a plain "--" are considered positional arguments. See
     *               "SetCommandLineParser(...)" for custom behaviour.
     *
     * @return Any positional arguments.
     */
    std::vector<std::string> ParseArgs(int argc, char** argv)
    {
        interrupted_ = false;
        auto [namedArgsValues, positionalArgs] = argsParser_(argc, argv, errorFunc_);
        for (const auto& [index, alias, value] : namedArgsValues) {
            if (interrupted_) {
                break;
            }
            if (aliasMap_.count(alias) > 0) {
                unsigned aliasIndex = aliasMap_.at(alias);
                Error actionError = options_.at(aliasIndex).onParse_(value);
                if (actionError != Error::None) {
                    errorFunc_(actionError, PointToArg(argc, argv, static_cast<int>(index)));
                    StopParsing();
                }
            } else {
                errorFunc_(Error::UnrecognisedAlias, PointToArg(argc, argv, static_cast<int>(index)));
                StopParsing();
            }
        }
        return positionalArgs;
    }

    void StopParsing()
    {
        interrupted_ = true;
    }

private:
    ErrorHandler errorFunc_;
    CmdLineParser argsParser_;

    std::vector<Option> options_;
    //      <   alias   ,  index  >
    std::map<std::string, unsigned> aliasMap_;
    bool interrupted_;
};

} // namespace EzArgs

#endif // EZARGS_H
