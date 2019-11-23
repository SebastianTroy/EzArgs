#include <iostream>

#include "EzArgs.h"

#include <optional>
#include <vector>

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

    std::cout << "x " << x << ", str " << str << ", bx " << (bx ? "true" : "false") << ", optInt " << (optInt ? std::to_string(optInt.value()) : "{}") << std::endl;

    EzArgs::ArgParser argParser;
    argParser.SetOptions({
        { "help,h",   [&](auto){ argParser.PrintHelpTable(); return EzArgs::Error::None; },      "prints help" },
        { "number,d", EzArgs::ParseRequiredValue(x),      "double" },
        { "b,bosch",  EzArgs::ParseRequiredValue(str),    "bosch" },
        { "nope,n",   EzArgs::ParseRequiredValue(bx),     "nope nope" },
        { "optint,o", EzArgs::ParseOptionalValue(optInt), "huh? {}S" },
        { "v,V",      EzArgs::DetectPresence(detected),   "vvvvvv" },
        { "e",        [](auto) -> EzArgs::Error {},       "empty" },
    });
    // TODO argParser.SetRules({}); // e.g. aliases are mutually exclusive, require one of, all or none... e.t.c.
    auto positionalArgs = argParser.ParseArgs(argc, argv);

    std::cout << "x " << x << ", str " << str << ", bx " << (bx ? "true" : "false") << ", optInt " << (optInt ? std::to_string(optInt.value()) : "{}") << std::endl;

    // std::tuple<int> pArgs = argParser.GetPositionalArguments(); // see if we can do some template wizardry to use our parse functions to turn a vector of expected size into a tuple

    // TODO run some exhaustive tests

    return 0;
}
