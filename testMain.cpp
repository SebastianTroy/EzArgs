#include <iostream>
#include <sstream>

#include "EzArgs.h"

#include <optional>
#include <vector>

struct CustomType {
    int n = -1;
    std::string str = "";
};

using namespace EzArgs;

int main(int argc, char** argv)
{
    for (int i = 0; i < argc; i++) {
        std::cout << std::string(argv[i]) << std::endl;
    }
    std::cout << std::endl;

    double x;
    double q;
    std::string str;
    bool bx;
    bool detected;
    std::optional<int> optInt;
    CustomType custom;

    std::cout << "x " << x << ", q " << q << ", str " << str << ", bx " << (bx ? "true" : "false") << ", optInt " << (optInt ? std::to_string(optInt.value()) : "{}") << ", Custom{ " << custom.n << ", " << custom.str << " }" << std::endl;

    ArgParser argParser;
    argParser.SetOptions({
        { "help,h",     PrintHelp(argParser, false, std::cerr, "Hello!"),                    "prints help" },
        { "number,d",   SetValue(x),                                                         "double" },
        { "nonumber,q", SetValue(q, -0.1010),                                                "qouble, defaults to -0.1010" },
        { "b,bosch",    SetValue(str),                                                       "bosch" },
        { "nope,n",     SetValue(bx),                                                        "nope nope" },
        { "custom,c",   SetValue(custom),                                                    "custom type parsing" },
        { "optint,o",   SetOptionalValue(optInt),                                            "huh? {}S" },
        { "v,V",        DetectPresence(detected),                                            "vvvvvv" },
        { "e",          { Parameter::None, [](auto) -> Error { return Error::None; } }, "empty" },
    });
    // TODO argParser.SetRules({}); // e.g. aliases are mutually exclusive, require one of, all or none... e.t.c.
    auto positionalArgs = argParser.ParseArgs(argc, argv);

    std::cout << "x " << x << ", q " << q << ", str " << str << ", bx " << (bx ? "true" : "false") << ", optInt " << (optInt ? std::to_string(optInt.value()) : "{}") << ", Custom{ " << custom.n << ", " << custom.str << " }" << std::endl;

    // std::tuple<int> pArgs = argParser.GetPositionalArguments(); // see if we can do some template wizardry to use our parse functions to turn a vector of expected size into a tuple

    // TODO run some exhaustive tests

    return 0;
}

std::stringstream& operator >> (std::stringstream& str, CustomType& custom)
{
    custom = {};
    str >> custom.n;
    str >> custom.str;
    return str;
}
