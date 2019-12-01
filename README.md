# EzArgs
A modern, lightweight, and single header args parsing library.

The easiest usage would be to copy the `EzArgs.h` header directly into your project .

## Aim
Why another args parsing library?

The args problem boiled down to its simplest form is that whan a user specifies an argument, you want to do something, usually something very simple like set a variable. It would also be nice to give the user helpful advice when they specify invalid arguments.

Most other solutions follow the old C style posix pattern, requiring repeated calls to `getopt(...)` with a big switch statement, a seperate structure defining the valid options a user can provide the program, and yet another structure/function elsewhere which prints the help text, all of which need to be maintained seperately despite all referring to the same options.

Other solutions convet the `int argc, char** argv` into a mapping of `{key, value}` pairs, but still require a lot of boilerplate code for you to process the values and give feedback to the user.

The aim is to allow the programmer to create a single list of `Option`s which encapsulate the valid args, option names, parameter requirements, help text and the action to carry out when the user specifies the option, in as compact and readable form as possible.

## Naming conventions
Name             | Meaning
------------     | -------------
Arg, Argument    | A string passed to the program via the command line.
Alias            | A name for an `Option`, each `Option` can have more than one alias.
Param, Parameter | A value coupled with an alias on the command line, i.e. `--alias=param`
`Option`         | An object defined as `{aliases, action, helpText}`.
`OptionAction`   | A function that is run by an option. It can accept no argument, an `std::optional<std::string>`, or a `std::string`.
`Rule`           | A function that is run on the command line arguments before any actions are carried out.


## Basic Usage
The minimum requirement to use this library is that you create a list of `Option`s. An option has one or more aliases, an action and some help text.

The following is a simple example of a program with one accepted argument, where it requires that argument to be paired with a value which can be parsed into a `double`.

    int main(int argc, char** argv)
    {
        double x = -1.0;
    
        EzArgs::ArgParser argParser;
        argParser.SetOptions({
            { "number,d", EzArgs::SetValue(x), "Sets a double" },
        });
        argParser.ParseArgs(argc, argv);
    }

In this case `EzArgs::SetValue(x)` is a helper function, which returns an `OptionAction` that requires a parameter and sets the variable `x` if the option `--number` or `-d` is specifed by the user.

An `Error` will occur if the user specifies any alias that isn't recognised, if they don't provide a parameter for `--number` or `-d`, and if the parameter they provide cannot be parsed into a `double`.

Default error handling is to print to `std::cout` and to stop parsing args. The default error messages print the list of Options or args as necessary and point to the erroneous entry. There is also a short English text explanation of the error. A custom error handling function can be specified to override this behaviour.

## Helper Functions
### These each return an `OptionAction`

 - `EzArgs::SetValue(x)` Is templated, specifies `Parameter::Required` and creates an action which populates a value by reference.
 - `EzArgs::SetValue(x, 42)` Is templated, specifies `Parameter::Optional` and if there is no parameter to parse, will set the variable `x` to the default value, in this case `42`.
 - `EzArgs::SetOptionalValue(x)` Is templated with `std::optional<Type>` and specifies `Parameter::Optional`, so if a value is parsed, it is set using the same mechanism as above, however if no argument value is specified, the value is left default `{}` aka null. This is likely to change in the future as currently there is no way to tell if the Option was present without a value, or if the Option was never specified at all.
 
Each of the `Set...` helpers is templated, but the type can usually be deduced by the compiler automatically. Each also has an optional argument `ParameterParser<T>& parser = GetDefaultParser<T>()` which uses a `std::stringstream` based implementation to construct the value from the `std::string`. Therefore it is possible to extend the default behaviour for custom types by simply defining a function overload, e.g.


    struct CustomType {
        int n;
        std::string str;
    };
    
    std::stringstream& operator>>(std::stringstream& str, CustomType& custom)
    {
        custom = {};
        str >> custom.n;
        str >> custom.str;
        return str;
    }
    
  - `EzArgs::DetectPresence(bool)` Simply sets a `bool` value by reference, sets it to `true` if the option was specified and `false` when the helper function is created.

  - `EzArgs::PrintHelp(const ArgParser&)` Prints a pretty printed table of `Option`s from the specified `ArgParser`. By default it prints to `std::cout` and exits the program after the table is printed. It has the optional arguments `bool exitAfter = true, std::ostream& ostr = std::cout, const std::string& additionalHelpText = ""`.

### These each return a `Rule`.

 - `RuleRequireAtLeastOne(const std::vector<std::string>& ruleAliases)` Will fail if the user hasn't specified at least one of the supplied options.
 - `RuleMutuallyExclusive(const std::vector<std::string>& ruleAliases)` Will fail if the user specifies more than one of the specified `options
 - `RuleRequireAllOrNone(const std::vector<std::string>& ruleAliases)` Will fail unless the user specifies none of the specified options, or all of them.
 
## Arg Parsing
By default posix style arguments are expected, with `-a` being a short alias, `--alias` being a long alias. A value can be paired with an alias in a few ways, `--alias=value`, `--alias value`, `-a=value`, `-a value`. Multiple short arguments can be specified at a time, where `a`, `b` and `c` are all short aliases the following is valid `-abc`, in the case of `-abc=value` or `-abc value` then `value` is applied to `c` only. `--` is a terminator, meaning the parsing will stop and all following args are considered positional, and are returned in a vector. 

A custom args parser can be specified to override this behaviour.

## Future Additions
 - [ ] Should provide default parsers for both Posix and Windows style args
 - [ ] Tests
 - [ ] Build script for building a static or shared EzArgs library
 - [ ] Class named ArgsParser too similarly named to function ArgParser
