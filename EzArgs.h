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
    ExpectedLongAlias,
    ExpectedAliasIndicator,
    OptionHasNoAliases,
    AliasClash,
    EmptyAlias,
    UnrecognisedAlias,
    ExpectedParameter,
    UnexpectedParameter,
    NullOptionAction,
    ParameterParseError,
    InvalidParameterEnumValue,
    RuleExpectedAtLeastOneOf,
    RuleOptionsMutuallyExclusive,
    RuleExpectedAllOrNoneOf,
};

enum class Parameter {
    None,
    Optional,
    Required,
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
using ParameterParser = const std::function<Error(const std::string& parameter, T& valueOut)>;

/**
 * Should return a vector of {index, aliases, param} objects, and a vector of
 * positional args. Just make sure the format is correct, don't check for
 * duplicates, unrecognised args, or invalid parameter values here.
 */
using ParsedArg = std::tuple<int, std::string, std::optional<std::string>>;
using ArgsParser = std::function<std::tuple<std::vector<ParsedArg>, std::vector<std::string>>(int argc, char** argv, const ErrorHandler& errorFunc_)>;

/**
 * @brief A rule must return true if the parsed args meet its criteria, and
 *        should call the errorHandler before returning false for any reason.
 */
using Rule = std::function<bool(const std::vector<ParsedArg>& parsedArgs, ErrorHandler& errorHandler)>;

using OptionActionNoParam = std::function<Error()>;
using OptionActionOptionalParam = std::function<Error(const std::optional<std::string>&)>;
using OptionActionRequiredParam = std::function<Error(const std::string&)>;

/**
 * A constructor per Parameter::None, Parameter::Optional, & Parameter::Required
 * This helper class exists to allow a cleaner definition between actions which
 * require parameters or not, and to reduce the ammount of boilerplate required
 * to construct an Option.
 */
class OptionAction {
public:
    /**
     * Converts the action to a OptionActionOptionalParam and specifies
     * Parameter::None.
     */
    OptionAction(OptionActionNoParam&& optionAction)
        : paramRequirements_(Parameter::None)
    {
        if (optionAction != nullptr) {
            action_ = [action = std::move(optionAction)](auto param) -> Error
            {
                if (param) {
                    return Error::UnexpectedParameter;
                } else {
                    return action();
                }
            };
        }
    }

    /**
     * Uses the provided OptionActionOptionalParam as it, and specifies
     * Parameter::Optional.
     */
    OptionAction(OptionActionOptionalParam&& optionAction)
        : paramRequirements_(Parameter::Optional)
        , action_(std::move(optionAction))
    {}

    /**
     * Converts the action to a OptionActionOptionalParam and specifies
     * Parameter::Required.
     */
    OptionAction(OptionActionRequiredParam&& optionAction)
        : paramRequirements_(Parameter::Required)
    {
        if (optionAction != nullptr) {
            action_ = [action = std::move(optionAction)](auto param) -> Error
            {
                if (param) {
                    return action(param.value());
                } else {
                    return Error::ExpectedParameter;
                }
            };
        }
    }

    Parameter GetParameterRequirements() const
    {
        return paramRequirements_;
    }

    const OptionActionOptionalParam& GetAction() const
    {
        return action_;
    }

private:
    const Parameter paramRequirements_;
    OptionActionOptionalParam action_;
};

/**
 * @brief The Option struct represents a thing you'd like to do in response to a
 *        argument specified when your program is run
 *
 * @param aliases_ A string representing the option's name(s), for example using
 *                 Posix style arg parsing "h,H,help" would result in this
 *                 options action being triggered by the presence of "-h", "-H"
 *                 or "--help". The names are comma seperated and any number of
 *                 names can be specified.
 *
 * @param onParse_ Use this to do something when this option is specified. A
 *                 number of helper functions have been provided for common use
 *                 cases, e.g SetValue(dMyNum) or DetectPresence(bArgDetected)
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
inline ParameterParser<T> GetDefaultParser()
{
    return [](const std::string& param, T& valueOut) -> Error
    {
        auto stream = std::stringstream(param);
        T temp;
        stream >> temp;
        if (!stream.fail() && stream.eof()) {
            valueOut = temp;
            return Error::None;
        } else {
            return Error::ParameterParseError;
        }
    };
}

template <>
inline ParameterParser<bool> GetDefaultParser()
{
    return [](const std::string& param, bool& valueOut) -> Error
    {
        std::string boolString = param;
        std::transform(boolString.begin(), boolString.end(), boolString.begin(), [](auto c){ return std::tolower(c); });

        if (boolString == "true" || boolString == "y" || boolString == "yes") {
            valueOut = true;
            return Error::None;
        } else if (boolString == "false" || boolString == "n" || boolString == "no") {
            valueOut = false;
            return Error::None;
        }
        return Error::ParameterParseError;
    };
}

template <>
inline ParameterParser<std::string> GetDefaultParser()
{
    return [](const std::string& param, std::string& valueOut) -> Error
    {
        valueOut = param;
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

///
/// errorFunc "where" helpers
///

std::string PointToArg(int argc, char** argv, int argToPointTo)
{
    std::stringstream stream;
    unsigned pointerIndex = 0;
    for (int i = 0; i < argc; i++) {
        std::string arg(argv[i]);
        if (i < argToPointTo) {
            pointerIndex += arg.size() + 1;
        }
        stream << arg << " ";
    }
    stream << std::endl << std::string(pointerIndex, ' ') << "^";
    return stream.str();
}

std::string PointToParsedArgs(const std::vector<ParsedArg>& parsedArgs, const std::vector<unsigned>& pointTo)
{
    std::stringstream stream;
    int lastIndex = 0;
    for (const auto& [index, aliases, param] : parsedArgs) {
        if (lastIndex != index) {
            if (lastIndex != 0) {
                stream << std::endl;
            }
            if (std::find(pointTo.cbegin(), pointTo.cend(), index) != pointTo.cend()) {
                stream << "-->";
            } else {
                stream << "   ";
            }
            lastIndex = index;
        }
        stream << aliases << (param ? " = " + param.value() : "");
    }
    return stream.str();
}

std::string PointToOptions(const std::vector<Option>& options, std::vector<unsigned> pointTo)
{
    std::stringstream stream;
    for (unsigned currentIndex = 0; currentIndex < options.size(); currentIndex++) {
        const auto& option = options[currentIndex];
        if (std::find(pointTo.cbegin(), pointTo.cend(), currentIndex) != pointTo.cend()) {
            stream << "-->";
        } else {
            stream << "   ";
        }
        stream << "{ " << option.aliases_ << ", " << option.helpText_ << " }" << std::endl;
    }
    return stream.str();
}

inline std::string PrintVector(const std::vector<std::string>& vec)
{
    std::stringstream stream;
    stream << "{ ";
    for (unsigned i = 0; i < vec.size(); i++) {
        stream << (i == 0 ? "" : ", ") << vec[i];
    }
    stream << " }";
    return stream.str();
}

/**
 * @brief The ArgParser class is where the meat of this library is. It is
 *        responsible for parsing the args, with the provided Options and
 *        checking that the parsed args meet all of the provided Rules. It also
 *        is responsible for ensuring that the Options provided are valid.
 */
class ArgParser {
public:
    /**
     * Defaults errorFunc_ and argsParser_ to simple implementations that should
     * meet most users requirements.
     */
    ArgParser()
    {
        errorFunc_ = [&](Error error, const std::string& where) -> void {
            std::cout << "-----------------" << std::endl;
            std::cout << "----- ERROR -----" << std::endl;
            std::cout << "-----------------" << std::endl;
            std::cout << where << std::endl;
            switch (error) {
            case Error::None :
                std::cout << "No Error (This should never be printed!)" << std::endl;
                break;
            case Error::ExpectedShortAlias :
                std::cout << "Expected at least one character following the short alias token." << std::endl;
                break;
            case Error::ExpectedLongAlias :
                std::cout << "Expected at least one character following the long alias token." << std::endl;
                break;
            case Error::ExpectedAliasIndicator :
                std::cout << "Expected the arg to begin with a short or long alias token." << std::endl;
                break;
            case Error::OptionHasNoAliases :
                std::cout << "Please specify at least one short or long alias for this option, e.g. \"h,H,help\" specifies three aliases for a help option." << std::endl;
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
            case Error::ExpectedParameter :
                std::cout << "Option requires a parameter, e.g. \"alias=value\" or \"alias value\"." << std::endl;
                break;
            case Error::UnexpectedParameter :
                std::cout << "Option does not accept a parameter." << std::endl;
                break;
            case Error::NullOptionAction :
                std::cout << "Option's action is null, please specify a valid action for the Option." << std::endl;
                break;
            case Error::ParameterParseError :
                std::cout << "Failed to parse parameter." << std::endl;
                break;
            case Error::InvalidParameterEnumValue :
                std::cout << "Parameter enum value must be None, Optional, or Required." << std::endl;
                break;
            case Error::RuleExpectedAtLeastOneOf :
                std::cout << "Program expects at least one of these Options be specified at runtime." << std::endl;
                break;
            case Error::RuleOptionsMutuallyExclusive :
                std::cout << "Program expects either none, or a single one of these Options be specified at runtime." << std::endl;
                break;
            case Error::RuleExpectedAllOrNoneOf :
                std::cout << "Program expects either none, or all of these Options be specified at runtime." << std::endl;
                break;
            }
            std::cout << "-----------------" << std::endl;
            this->StopParsing();
        };

        argsParser_ = [](int argc, char** argv, const ErrorHandler& errorFunc_) -> std::tuple<std::vector<ParsedArg>, std::vector<std::string>>
        {
            int index = 1;

            std::vector<ParsedArg> argsAndParams;
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
                        std::optional<std::string> param;
                        if (aliasLast != arg.npos) {
                            auto paramFirst = aliasLast + 1;
                            auto paramLast = arg.npos;
                            param = arg.substr(paramFirst, paramLast - paramFirst);
                        }
                        argsAndParams.push_back({ index, alias, param });
                    }
                } else if (arg.substr(0, 1) == "-") {
                    if (arg.size() == 1 || arg.at(1) == '=') {
                        errorFunc_(Error::ExpectedShortAlias, PointToArg(argc, argv, index));
                    } else {
                        for (const char& shortAlias : arg.substr(1, arg.npos)) {
                            if (shortAlias == '=') {
                                auto first = arg.find_first_of('=') + 1;
                                auto last = arg.npos;
                                std::get<2>(argsAndParams.back()) = arg.substr(first, last - first);
                                break;
                            } else if(std::isalpha(shortAlias)) {
                                argsAndParams.push_back({ index, {shortAlias}, {} });
                            } else {
                                errorFunc_(Error::ExpectedShortAlias, PointToArg(argc, argv, index));
                            }
                        }
                    }
                } else if (!argsAndParams.empty()) {
                    if (!std::get<2>(argsAndParams.back())) {
                        std::get<2>(argsAndParams.back()) = arg;
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

            return { argsAndParams, positionalArgs };
        };
    }

    void SetErrorHandler(ErrorHandler&& errorHandler)
    {
        errorFunc_ = std::move(errorHandler);
    }
    void SetArgsParser(ArgsParser&& argsParser)
    {
        argsParser_ = std::move(argsParser);
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

            if (option.onParse_.GetAction() == nullptr) {
                errorFunc_(Error::NullOptionAction, PointToOptions(options_, { currentIndex }));
                success = false;
                break;
            }

            if (auto parameterPresence = option.onParse_.GetParameterRequirements(); parameterPresence != Parameter::None && parameterPresence != Parameter::Optional && parameterPresence != Parameter::Required) {
                errorFunc_(Error::InvalidParameterEnumValue, PointToOptions(options_, { currentIndex }));
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

    void SetRules(std::vector<Rule>&& rules)
    {
        rules_ = std::move(rules);
    }

    void PrintHelpTable(std::ostream& out = std::cout, std::string additionalHelpText = "") const
    {
        std::map<Parameter, std::string> parameterStrings {
            {Parameter::None,     "None     "},
            {Parameter::Optional, "Optional "},
            {Parameter::Required, "Required "},
        };
        const unsigned paramColWidth = 9;
        unsigned aliasColWidth = 0;
        unsigned helpColWidth = 0;
        for (const auto& [aliases, action, helpText] : options_) {
            (void) action;
            aliasColWidth = std::max(aliasColWidth, aliases.size());
            helpColWidth = std::max(helpColWidth, helpText.size());
        }

        std::string aliasTitle = "Aliases";
        std::string paramTitle = "Parameter";
        std::string helpTitle = "Usage";
        out << " _" << std::string(aliasColWidth, '_') << "___"  << std::string(paramColWidth, '_') << "___"  << std::string(helpColWidth, '_') << "_ " << std::endl;
        out << "| " << aliasTitle << std::string(aliasColWidth - aliasTitle.size(), ' ') << " | " << paramTitle << std::string(paramColWidth - paramTitle.size(), ' ') << " | " << helpTitle << std::string(helpColWidth - helpTitle.size(), ' ') << " |" << std::endl;
        out << "|_" << std::string(aliasColWidth, '_') << "_|_"  << std::string(paramColWidth, '_') << "_|_"  << std::string(helpColWidth, '_') << "_|" << std::endl;
        for (const auto& [aliases, onParse, helpText] : options_) {
            out << "| " << aliases << std::string(aliasColWidth - aliases.size(), ' ') << " | " << parameterStrings.at(onParse.GetParameterRequirements()) << " | " << helpText << std::string(helpColWidth - helpText.size(), ' ') << " |"<< std::endl;
        }

        out << "|_" << std::string(aliasColWidth, '_') << "_|_"  << std::string(paramColWidth, '_') << "_|_"  << std::string(helpColWidth, '_') << "_|" << std::endl;
        out << std::endl << additionalHelpText << std::endl << std::endl;
    }

    /**
     * ParseArgs parses the arguments in the order they were specified on the
     * comand line. Once an option has been actioned, it will not be actioned
     * again, even if a different alias for the same option was used or a
     * different parameter value was provided.
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
        auto [parsedArgs, positionalArgs] = argsParser_(argc, argv, errorFunc_);
        for (const Rule& rule : rules_) {
            if (!rule(parsedArgs, errorFunc_)) {
                StopParsing();
            }
        }
        for (const auto& [index, alias, parameter] : parsedArgs) {
            if (interrupted_) {
                break;
            }
            if (aliasMap_.count(alias) > 0) {
                unsigned aliasIndex = aliasMap_.at(alias);
                auto optionAction = options_.at(aliasIndex).onParse_.GetAction();
                Error actionError = optionAction(parameter);
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
    ArgsParser argsParser_;

    std::vector<Option> options_;
    //      <   alias   ,  index  >
    std::map<std::string, unsigned> aliasMap_;
    bool interrupted_;
    std::vector<Rule> rules_;
};

///
/// OptionAction Helpers
///

template <typename T>
inline OptionActionRequiredParam SetValue(T& valueOut, ParameterParser<T>& parser = GetDefaultParser<T>())
{
    return [&](const std::string& argValue) -> Error
    {
        return parser(argValue, valueOut);
    };
}

template <typename T>
inline OptionActionOptionalParam SetValue(T& valueOut, const T& defaultValue, ParameterParser<T>& parser = GetDefaultParser<T>())
{
    return [&](const std::optional<std::string>& param) -> Error
    {
        if (!param) {
            valueOut = defaultValue;
            return Error::None;
        } else {
            return parser(param.value(), valueOut);
        }
    };
}

template <typename T>
inline OptionActionOptionalParam SetOptionalValue(std::optional<T>& valueOut, ParameterParser<T>& parser = GetDefaultParser<T>())
{
    return [&](const std::optional<std::string> param) -> Error
    {
        if (!param) {
            valueOut = {};
            return Error::None;
        } else {
            T temp;
            Error error = parser(param.value(), temp);
            if (error == Error::None) {
                valueOut = temp;
            }
            return error;
        }
    };
}

inline OptionActionNoParam DetectPresence(bool& valueOut)
{
    return [&]() -> Error
    {
        valueOut = true;
        return Error::None;
    };
}

inline OptionActionNoParam PrintHelp(const ArgParser& parser, bool exitAfter = true, std::ostream& ostr = std::cout, const std::string& additionalHelpText = "")
{
    return [&, exitAfter, additionalHelpText]() -> Error
    {
        parser.PrintHelpTable(ostr, additionalHelpText);
        if (exitAfter) {
            exit(0);
        }
        return Error::None;
    };
}

///
/// ArgsRule Helpers
///

// private namespace for hidden internal helpers
namespace {

inline std::vector<unsigned> GetArgIndexesOf(const std::vector<std::string>& ruleAliases, const std::vector<ParsedArg>& parsedArgs)
{
        std::vector<unsigned> argIndexes;
        for (const auto& [index, aliases, param] : parsedArgs) {
            (void) param; // unused
            // If the whole comma seperated aliases string has been provided
            if (std::find(ruleAliases.cbegin(), ruleAliases.cend(), aliases) != ruleAliases.cend()) {
                argIndexes.push_back(static_cast<unsigned>(index));
                continue;
            }
            // If a single alias for an option has been [rovided
            for (const std::string& alias : ParseAliases(aliases)) {
                if (std::find(ruleAliases.cbegin(), ruleAliases.cend(), alias) != ruleAliases.cend()) {
                    argIndexes.push_back(static_cast<unsigned>(index));
                    continue;
                }
            }
        }
        return argIndexes;
}

} // end private namespace

inline Rule RuleRequireAtLeastOne(const std::vector<std::string>& ruleAliases)
{
    return [=](const std::vector<ParsedArg>& parsedArgs, const ErrorHandler& errorHandler) -> bool
    {
        auto indexes = GetArgIndexesOf(ruleAliases, parsedArgs);
        if (indexes.size() == 0) {
            errorHandler(Error::RuleExpectedAtLeastOneOf, PointToParsedArgs(parsedArgs, indexes) + "\n" + PrintVector(ruleAliases));
            return false;
        }
        return true;
    };
}

inline Rule RuleMutuallyExclusive(const std::vector<std::string>& ruleAliases)
{
    return [=](const std::vector<ParsedArg>& parsedArgs, const ErrorHandler& errorHandler) -> bool
    {
        auto indexes = GetArgIndexesOf(ruleAliases, parsedArgs);
        if (indexes.size() > 1) {
            errorHandler(Error::RuleOptionsMutuallyExclusive, PointToParsedArgs(parsedArgs, indexes) + "\n" + PrintVector(ruleAliases));
            return false;
        }
        return true;
    };
}

inline Rule RuleRequireAllOrNone(const std::vector<std::string>& ruleAliases)
{
    return [=](const std::vector<ParsedArg>& parsedArgs, const ErrorHandler& errorHandler) -> bool
    {
        auto indexes = GetArgIndexesOf(ruleAliases, parsedArgs);
        if (indexes.size() != 0 && indexes.size() != ruleAliases.size()) {
            errorHandler(Error::RuleExpectedAllOrNoneOf, PointToParsedArgs(parsedArgs, indexes) + "\n" + PrintVector(ruleAliases));
            return false;
        }
        return true;
    };
}

} // namespace EzArgs

#endif // EZARGS_H
