#include "EzArgs.h"
#include "Catch.h"

#include <iostream>
#include <sstream>
#include <optional>
#include <vector>

namespace EzArgs {

class TestHelper {
public:
    int argc;
    char** argv;
    ErrorHandler errorHandler;
    std::vector<Error> errors;


    TestHelper(std::vector<std::string>&& argVector)
        : argc(static_cast<int>(argVector.size()))
        , argv(new char*[static_cast<unsigned>(argc)])
        , errorHandler([&](Error err, auto)
    {
        errors.push_back(err);
    })
    {
        for (size_t i = 0; i < argVector.size(); i++) {
            argv[i] = new char[argVector[i].size() + 1];
            strncpy(argv[i], argVector[i].c_str(), argVector[i].size());
            argv[i][argVector[i].size()] = '\0';
        }
    }

    ~TestHelper()
    {
        for (int i = 0; i < argc; i++) {
            delete[] argv[i];
        }
        delete[] argv;
    }
};

template <typename T>
std::set<T> ToSet(std::vector<T> vec)
{
    std::set<T> set;
    for (auto i : vec) {
        set.insert(i);
    }
    return set;
}

TEST_CASE("TestHelpers", "[meta]")
{
    SECTION("Helper with no args")
    {
        auto&& [argc, argv, errFunc, errors] = TestHelper({});
        (void) argv;
        (void) errFunc;
        (void) errors;
        REQUIRE(argc == 0);
    }

    SECTION("Helper with args")
    {
        auto&& [argc, argv, errFunc, errors] = TestHelper({"one", "two"});

        REQUIRE(argc == 2);
        REQUIRE(argv[0][0] == 'o');
        REQUIRE(argv[0][1] == 'n');
        REQUIRE(argv[0][2] == 'e');
        REQUIRE(argv[0][3] == '\0');
        REQUIRE(argv[1][0] == 't');
        REQUIRE(argv[1][1] == 'w');
        REQUIRE(argv[1][2] == 'o');
        REQUIRE(argv[1][3] == '\0');

        errFunc(Error::None, "");

        REQUIRE(errors.size() == 1);
        REQUIRE(errors.front() == Error::None);

        auto movedFunc = std::move(errFunc);
        movedFunc(Error::NullOptionAction, "");

        REQUIRE(errors.size() == 2);
        REQUIRE(errors.front() == Error::None);
        REQUIRE(errors.back() == Error::NullOptionAction);
    }

    SECTION("ToSet")
    {
        REQUIRE(ToSet<int>({  }) == std::set<int>{  });
        REQUIRE(std::set<int>{ 3, 54, 99, 99, 3, 3 } == std::set<int>{ 3, 54, 99 });
        REQUIRE(ToSet<int>({ 99, 3, 3, 54, 3 }) == std::set<int>{ 3, 54, 99 });
    }
}

TEST_CASE("Default behaviour", "[default]")
{
    auto&& [argc, argv, errFunc, errors] = TestHelper({});
    ArgParser defaultParser(std::move(errFunc));


    SECTION("Construction")
    {
        REQUIRE(errors.empty());
    }

    SECTION("Set empty Options")
    {
        defaultParser.SetOptions({});
        REQUIRE(errors.empty());
    }

    SECTION("Set empty Rules")
    {
        defaultParser.SetRules({});
        REQUIRE(errors.empty());
    }

    SECTION("Parse Nothing")
    {
        defaultParser.ParseArgs(argc, argv);
        REQUIRE(errors.empty());
    }
}

TEST_CASE("Helpers work as expected", "[helpers]")
{
    SECTION("ParseAliases")
    {
        REQUIRE(ParseAliases("") == std::vector<std::string>{ });
        REQUIRE(ParseAliases("a") == std::vector<std::string>{ "a" });
        REQUIRE(ParseAliases("a,b") == std::vector<std::string>{ "a", "b" });
        REQUIRE(ParseAliases("a,") == std::vector<std::string>{ "a", "" });
        REQUIRE(ParseAliases(",b") == std::vector<std::string>{ "", "b" });
        REQUIRE(ParseAliases(",") == std::vector<std::string>{ "", "" });
        REQUIRE(ParseAliases("abc") == std::vector<std::string>{ "abc" });
        REQUIRE(ParseAliases("abc,def,ghi") == std::vector<std::string>{ "abc", "def", "ghi" });
        REQUIRE(ParseAliases("abc,d,ghi") == std::vector<std::string>{ "abc", "d", "ghi" });
        REQUIRE(ParseAliases("abc,,ghi") == std::vector<std::string>{ "abc", "", "ghi" });
        REQUIRE(ParseAliases(",,,,,,") == std::vector<std::string>{ "", "", "", "", "", "", "" });
    }

    SECTION("GetArgIndexesOf")
    {
        SECTION("EmptyArrays")
        {
            std::vector<std::string> f{};
            std::vector<ParsedArg> g{};
            REQUIRE(ToSet(GetArgIndexesOf(f, g)) == ToSet<unsigned>({  }));
        }

        SECTION("SingleItem")
        {
            SECTION("Found")
            {
                std::vector<std::string> f{
                    "hello",
                };
                std::vector<ParsedArg> g{
                    { 1, "hello", {} },
                };
                REQUIRE(ToSet(GetArgIndexesOf(f, g)) == ToSet<unsigned>({ 1 }));
            }

            SECTION("NotFound")
            {
                std::vector<std::string> f{
                    "nope",
                };
                std::vector<ParsedArg> g{
                    { 1, "hello", {} },
                };
                REQUIRE(ToSet(GetArgIndexesOf(f, g)) == ToSet<unsigned>({  }));
            }
        }

        SECTION("MultipleItem")
        {
            SECTION("AllFound")
            {
                std::vector<std::string> f{
                    "hello",
                    "goodbye",
                };
                std::vector<ParsedArg> g{
                    { 1, "hello", {} },
                    { 2, "goodbye", {} },
                };
                REQUIRE(ToSet(GetArgIndexesOf(f, g)) == ToSet<unsigned>({ 1, 2 }));
            }

            SECTION("NoneFound")
            {
                std::vector<std::string> f{
                    "nope",
                    "neither",
                };
                std::vector<ParsedArg> g{
                    { 1, "hello", "World!" },
                    { 2, "goodbye", {} },
                };
                REQUIRE(ToSet(GetArgIndexesOf(f, g)) == ToSet<unsigned>({  }));
            }

            SECTION("OneFound")
            {
                std::vector<std::string> f{
                    "nope",
                    "neither",
                    "maybe",
                    "finally",
                };
                std::vector<ParsedArg> g{
                    { 1, "hello", {} },
                    { 2, "goodbye", {} },
                    { 3, "d", {} },
                    { 3, "f", {} },
                    { 3, "b", "bug" },
                    { 4, "finally", "4.786" },
                };
                REQUIRE(ToSet(GetArgIndexesOf(f, g)) == ToSet<unsigned>({ 4 }));
            }

            SECTION("Duplicates")
            {
                std::vector<std::string> f{
                    "many",
                    "neither",
                    "maybe",
                    "finally",
                };
                std::vector<ParsedArg> g{
                    { 1, "many", {} },
                    { 2, "many", {} },
                    { 3, "many", {} },
                    { 3, "maybe", {} },
                    { 3, "many", "bug" },
                    { 4, "many", "4.786" },
                };
                REQUIRE(ToSet(GetArgIndexesOf(f, g)) == ToSet<unsigned>({ 1, 2, 3, 3, 3, 4 }));
            }

            SECTION("OutOfOrder")
            {
                std::vector<std::string> f{
                    "first",
                    "third",
                    "second",
                    "fourth",
                };
                std::vector<ParsedArg> g{
                    { 4, "fourth", {} },
                    { 2, "second", {} },
                    { 1, "first", {} },
                    { 3, "third", {} },
                };
                REQUIRE(ToSet(GetArgIndexesOf(f, g)) == ToSet<unsigned>({ 1, 3, 2, 4 }));
            }

            SECTION("IgnoreParams")
            {
                std::vector<std::string> f{
                    "a",
                    "b",
                    "cake",
                    "9.2",
                };
                std::vector<ParsedArg> g{
                    { 4, "fourth", "a" },
                    { 2, "second", {} },
                    { 1, "first", "cake" },
                    { 3, "third", "9.2" },
                };
                REQUIRE(ToSet(GetArgIndexesOf(f, g)) == ToSet<unsigned>({  }));
            }
        }
    }

    SECTION("SetValue")
    {
        SECTION("String")
        {
            std::string x = "Hello World!";
            auto setValueFunc = SetValue(x);

            REQUIRE(x == "Hello World!");

            auto error = setValueFunc("Hello  Henry!");
            REQUIRE(x == "Hello  Henry!");
            REQUIRE(error == Error::None);

            error = setValueFunc("");
            REQUIRE(x == "");
            REQUIRE(error == Error::None);
        }

        SECTION("Double")
        {
            double x = 0.0;
            auto setValueFunc = SetValue(x);

            REQUIRE(x == 0.0);

            auto error = setValueFunc("0.999");
            REQUIRE(x == Approx(0.999));
            REQUIRE(error == Error::None);

            error = setValueFunc("-0.12345");
            REQUIRE(x == Approx(-0.12345));
            REQUIRE(error == Error::None);

            error = setValueFunc("");
            REQUIRE(x == Approx(-0.12345)); // unchanged
            REQUIRE(error == Error::ParameterParseError);
        }

        SECTION("Boolean")
        {
            bool x = true;
            auto setValueFunc = SetValue(x);

            REQUIRE(x == true);

            auto error = setValueFunc("false");
            REQUIRE(x == false);
            REQUIRE(error == Error::None);

            error = setValueFunc("true");
            REQUIRE(x == true);
            REQUIRE(error == Error::None);

            error = setValueFunc("N");
            REQUIRE(x == false);
            REQUIRE(error == Error::None);

            error = setValueFunc("Yes");
            REQUIRE(x == true);
            REQUIRE(error == Error::None);

            error = setValueFunc("FalSE");
            REQUIRE(x == false);
            REQUIRE(error == Error::None);

            error = setValueFunc("y");
            REQUIRE(x == true);
            REQUIRE(error == Error::None);

            error = setValueFunc("vgsjd");
            REQUIRE(x == true); // unchanged
            REQUIRE(error == Error::ParameterParseError);

            error = setValueFunc("0");
            REQUIRE(x == true); // unchanged
            REQUIRE(error == Error::ParameterParseError);

            error = setValueFunc("falsetrue");
            REQUIRE(x == true); // unchanged
            REQUIRE(error == Error::ParameterParseError);
        }

        SECTION("CustomParser")
        {
            unsigned x = 0;
            auto setValueFunc = SetValue<unsigned>(x, [](const std::string&, unsigned& out) -> Error { out++; return static_cast<Error>(-91); });

            REQUIRE(x == 0);

            auto error = setValueFunc("99");
            REQUIRE(error == static_cast<Error>(-91));
            REQUIRE(x == 1);

            setValueFunc("99");
            setValueFunc("jasehfjhkwsheflkjs");
            setValueFunc("true");
            setValueFunc("3");
            setValueFunc("-99");
            REQUIRE(x == 6);
        }
    }

    SECTION("SetValueWithDefault")
    {
        float x = 1.23f;
        auto setValueFunc = SetValue(x, 33.33f);

        REQUIRE(x == Approx(1.23));
        auto error = setValueFunc({});
        REQUIRE(x == Approx(33.33));
        REQUIRE(error == Error::None);

        error = setValueFunc("9.876");
        REQUIRE(x == Approx(9.876));
        REQUIRE(error == Error::None);


        error = setValueFunc("dkjfghkasjdghksdjhg");
        REQUIRE(x == Approx(9.876)); // no change
        REQUIRE(error == Error::ParameterParseError);
    }

    SECTION("SetOptionalValue")
    {
        std::optional<bool> x;
        auto setValueFunc = SetOptionalValue(x);

        REQUIRE(!x);
        auto error = setValueFunc("true");
        REQUIRE(x == true);
        REQUIRE(error == Error::None);

        setValueFunc({});
        REQUIRE(!x);
        REQUIRE(error == Error::None);
    }

    SECTION("DetectPresence")
    {
        bool x = false;
        auto detectFunc = DetectPresence(x);

        REQUIRE(x == false);
        auto error = detectFunc();
        REQUIRE(error == Error::None);
        REQUIRE(x == true);
    }
}

TEST_CASE("Option errors", "[option]")
{
    auto&& [argc, argv, errFunc, errors] = TestHelper({});
    (void) argc;
    (void) argv;
    ArgParser defaultParser(std::move(errFunc));

    SECTION("Set valid Option")
    {
        defaultParser.SetOptions({
                                     {"h,help", PrintHelp(defaultParser), ""},
                                 });
        REQUIRE(errors.empty());
    }

    SECTION("Set valid Options")
    {
        defaultParser.SetOptions({
                                     {"h,help", PrintHelp(defaultParser), ""},
                                     {"n,nope", PrintHelp(defaultParser), ""},
                                 });
        REQUIRE(errors.empty());
    }

    SECTION("Set null OptionAction")
    {
        defaultParser.SetOptions({
                                     {"h,help", OptionActionNoParam{}, ""},
                                 });
        REQUIRE(errors.size() == 1);
        REQUIRE(errors.front() == Error::NullOptionAction);
    }

    SECTION("Null OptionActions")
    {
        defaultParser.SetOptions({
                                     {"h,help", OptionActionNoParam{}, ""},
                                     {"n,nope", OptionActionNoParam{}, ""},
                                     {"d,dope", OptionActionNoParam{}, ""},
                                     {"r,rope", OptionActionNoParam{}, ""},
                                 });
        REQUIRE(errors.size() == 4);
        REQUIRE(errors[0] == Error::NullOptionAction);
        REQUIRE(errors[1] == Error::NullOptionAction);
        REQUIRE(errors[2] == Error::NullOptionAction);
        REQUIRE(errors[3] == Error::NullOptionAction);
    }

    SECTION("No Alias")
    {
        defaultParser.SetOptions({
                                     {"", PrintHelp(defaultParser), ""},
                                 });
        REQUIRE(errors.size() == 1);
        REQUIRE(errors.front() == Error::OptionHasNoAliases);
    }

    SECTION("No Aliases")
    {
        defaultParser.SetOptions({
                                     {"", PrintHelp(defaultParser), ""},
                                     {"", PrintHelp(defaultParser), ""},
                                 });
        REQUIRE(errors.size() == 2);
        REQUIRE(errors.front() == Error::OptionHasNoAliases);
        REQUIRE(errors.back() == Error::OptionHasNoAliases);
    }

    SECTION("Empty Alias")
    {
        defaultParser.SetOptions({
                                     {"f,", PrintHelp(defaultParser), ""},
                                 });
        REQUIRE(errors.size() == 1);
        REQUIRE(errors.front() == Error::EmptyAlias);
    }

    SECTION("Empty Aliases")
    {
        defaultParser.SetOptions({
                                     {",,,", PrintHelp(defaultParser), ""},
                                 });
        REQUIRE(errors.size() == 4);
        REQUIRE(errors[0] == Error::EmptyAlias);
        REQUIRE(errors[1] == Error::EmptyAlias);
        REQUIRE(errors[2] == Error::EmptyAlias);
        REQUIRE(errors[3] == Error::EmptyAlias);
    }


    // TODO are there more Option errors?

}

} // namespace EzArgs

int main(int argc, char* argv[])
{
    return Catch::Session().run(argc, argv);
}
