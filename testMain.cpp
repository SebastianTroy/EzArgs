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
    std::string party = "We are going to a ";

    std::cout << "x " << x << ", q " << q << ", str " << str << ", bx " << (bx ? "true" : "false") << ", optInt " << (optInt ? std::to_string(optInt.value()) : "{}") << ", Custom{ " << custom.n << ", " << custom.str << " } " << party << std::endl;

    ArgParser argParser;
    argParser.SetOptions({
        { "help,h",      PrintHelp(argParser, false, std::cerr, "Hello!"), "prints help" },
        { "shorthelp,s", PrintHelp(argParser),                             "prints help to cout and exits" },
        { "number,d",    SetValue(x),                                      "double" },
        { "nonumber,q",  SetValue(q, -0.1010),                             "qouble, defaults to -0.1010" },
        { "b,bosch",     SetValue(str),                                    "bosch" },
        { "nope,n",      SetValue(bx),                                     "nope nope" },
        { "custom,c",    SetValue(custom),                                 "custom type parsing" },
        { "optint,o",    SetOptionalValue(optInt),                         "huh? {}S" },
        { "v,V",         DetectPresence(detected),                         "vvvvvv" },
        { "e",
          OptionActionNoParam{[&]() -> Error
          {
              party += "Barmitsva";
              return Error::None;
          }
        },                                                                "NoParam empty" },
        { "f",
          OptionActionRequiredParam{[&](const std::string& param) -> Error
          {
              party += param;
              return Error::None;
          }
        },                                                                "RequiredParam empty" },
        { "g",
          OptionActionOptionalParam{[&](const std::optional<std::string>& param) -> Error
          {
              param ? party += param.value() : "Seance!";
              return Error::None;
          }
        },                                                                "OptionalParam empty" },
    });
    argParser.SetRules({
        RuleMutuallyExclusive({"e", "f", "g"}),
    });
    auto positionalArgs = argParser.ParseArgs(argc, argv);

    std::cout << "x " << x << ", q " << q << ", str " << str << ", bx " << (bx ? "true" : "false") << ", optInt " << (optInt ? std::to_string(optInt.value()) : "{}") << ", Custom{ " << custom.n << ", " << custom.str << " } " << party << std::endl;

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
