# EzArgs
A modern, lightweight, and single header args parsing library

## Aim
Why another args parsing library?

The args problem boiled down to its simplest form is that whan a user specifies an argument, you want to do something, usually something very simple like set a variable.

Most other solutions follow the old C style posix pattern, requiring repeated calls to `getopt(...)` with a big switch statement, a seperate structure defining the valid options a user can provide the program, and yet another structure/function elsewhere which prints the help text, all of which need to be maintained seperately despite all referring to the same options.

The aim is to provide a single structure which encapsulates the valid options, the help text and the action to carry out when the user specifies said option in as compact and readable form as possible.

## Basic Usage
By default posix style arguments are expected, with `-a` being a short alias, `--alias` being a long alias. A value can be paired with an alias in a few ways, `--alias=value`, `--alias value`, `-a=value`, `-a value`. Multiple short arguments can be specified at a time, where `a`, `b` and `c` are all short aliases the following is valid `-abc`, in the case of `-abc=value` or `-abc value` then `value` is applied to `c` only. `--` is a terminator, meaning the parsing will stop and all following args are considered positional, and are returned in a vector. A custom args parser can be specified to override this behaviour.

The minimum requirement to use this library is that you create a list of `Option`s. An option has one or more aliases, an action and some help text.

The following is a simple example of a program with one accepted argument, where it requires that argument to be paired with a value which can be parsed into a `double`.

    int main(int argc, char** argv)
    {
        double x = -1.0;
    
        EzArgs::ArgParser argParser;
        argParser.SetOptions({
            { "number,d", EzArgs::ParseRequiredValue(x), "Sets a double" },
        });
        argParser.ParseArgs(argc, argv);
    }

In this case `EzArgs::ParseRequiredValue(x)` is a helper function, which returns an action that sets the variable `x` if the option `--number` or `-d` is specifed by the user.

Default error handling is to print to `std::cout` and to stop parsing args. The default error messages print the list of Options or args as necessary and point to the erroneous entry. There is also a short English text explanation of the error. A custom error hndling function can be specified to override this behaviour.

## Helper Functions
 - `EzArgs::ParseRequiredValue(x)` Is templated and creates an action which populates a value by reference. By default it uses a `std::stringstream` based implementation to construct the value from the `std::string`. Therefore it is possible to extend the default behaviour for custom types by simply defining a function overload, e.g.

`struct CustomType {
    int n;
    std::string str;
};`
    
    std::stringstream& operator>>(std::stringstream& str, CustomType& custom)
    {
        custom = {};
        str >> custom.n;
        str >> custom.str;
        return str;
    }
 - `EzArgs::ParseOptionalValue(x)` Is templated `std::optional<Type>` so that if a value is parsed, it is set using the same mechanism as above, however if no argument value is specified, the value is left default `{}` aka null. This is likely to change in the future as currently there is no way to tell if the Option was present without a value, or if the Option was never specified at all.
 
  - `EzArgs::DetectPresence(bool)` Simply sets a `bool` value by reference, sets it to `true` if the option was specified and `false` when the helper function is created.

## Future Additions
 - A default windows style arg parser,
 - Rules, the ability to set on the parser a set of rules which the args must abide by, like having args which must be specified, or args which cannot both be specified together but are valid independantly,
 - make an enum for Value::Required, ::Optional & ::None, moving the onus of value presence checking away from the Option's action,
