#include <iostream>
#include <sstream>

#include "EzArgs.h"

#include <optional>
#include <vector>

struct CustomType {
    int n = -1;
    std::string str = "";
};

int main(int argc, char** argv)
{
    for (int i = 0; i < argc; i++) {
        std::cout << std::string(argv[i]) << std::endl;
    }
    std::cout << std::endl;

    double x;
    std::string str;
    bool bx;
    bool detected;
    std::optional<int> optInt;
    CustomType custom;

    std::cout << "x " << x << ", str " << str << ", bx " << (bx ? "true" : "false") << ", optInt " << (optInt ? std::to_string(optInt.value()) : "{}") << ", Custom{ " << custom.n << ", " << custom.str << " }" << std::endl;

    EzArgs::ArgParser argParser;
    argParser.SetOptions({
        { "help,h",   EzArgs::PrintHelp(argParser, false, std::cerr, "Hello!"),  "prints help" },
        { "number,d", EzArgs::ParseRequiredValue(x),                             "double" },
        { "b,bosch",  EzArgs::ParseRequiredValue(str),                           "bosch" },
        { "nope,n",   EzArgs::ParseRequiredValue(bx),                            "nope nope" },
        { "custom,c", EzArgs::ParseRequiredValue(custom),                        "custom type parsing" },
        { "optint,o", EzArgs::ParseOptionalValue(optInt),                        "huh? {}S" },
        { "v,V",      EzArgs::DetectPresence(detected),                          "vvvvvv" },
        { "e",        [](auto) -> EzArgs::Error { return EzArgs::Error::None; }, "empty" },
    });
    // TODO argParser.SetRules({}); // e.g. aliases are mutually exclusive, require one of, all or none... e.t.c.
    auto positionalArgs = argParser.ParseArgs(argc, argv);

    std::cout << "x " << x << ", str " << str << ", bx " << (bx ? "true" : "false") << ", optInt " << (optInt ? std::to_string(optInt.value()) : "{}") << ", Custom{ " << custom.n << ", " << custom.str << " }" << std::endl;

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
