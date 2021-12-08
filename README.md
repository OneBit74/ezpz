# EZPZ
ezpz is a C++20 parser combinator library that gives flexibility and simplicity the highest priority. ezpz is still in development and everything is subject to change.

## Building
ezpz is (going to be) a header-only library. To build the tests and examples you need a compiler supporting c++20 language features (e.g. gcc 11.1). To build you need to have cmake and make installed. Go into the project root directory. Then, run the following commands:
```bash
mkdir build
cd build
cmake ..
make
```

## Basic Syntax
The syntax is similar to ENFS, where you define nonterminals composed of other nonterminals and terminals. Lets Look at a simple grammar in EBNF format:

```uml
my_nonterminal = number, whitespace, number | number;
number = ["-"], digit, {digit};
whitespace = " " | "\t" | "\n";
```
In ezpz everything is called a parser and complex parsers are made up of simpler parsers. Instead of ',' ezpz uses the **'+'-operator**. Parsers are represented by objects.
```c++
// Note that we have to define whitespace earlier
// because the more complex 'my_first_parser'-parser depends on it
//
// Also operator| cant be overloaded for string literals as rhs and lhs simultanous
// so using ezpz::_p literal operator a parser type is generated.
auto ws = " "_p | "\t" | "\n";
auto my_first_parser = decimal<int>+ws+decimal<int> | decimal<int>;

match("123",my_first_parser);       // => true
match("123 456",my_first_parser);   // => true
match("hallo",my_first_parser);     // => false
```
There is also builtin support for other quantifiers such as optional, any, plus, ...

## Unparsing
As you might have noticed earlier we have used the builtin parser `ezpz::decimal<int>`. But why do we specify an integer type-parameter? This is because ezpz offers excellent unparsing and context-management. Such a parser is called a returning parser object or 'RPO' for short. It reads and accepts input as any other parser, but also **produces** an int. So, how do we access the result? You can simply combine the RPO with a lambda (or callable type) that takes an int as an argument. The result will be a parser.
```c++
match("123",decimal<int>*[](int value){std::cout << value << std::endl;});
```
This allows you to write short an powerfull grammars. Where you can flexibly manage the parsing context by potentially capturing arbitrary context. Additionally, ezpz supports **parameter-packs** of the callee, by choosing the largest contiguous sequence of types available for unparsing for which the lambda is callable. So if an rpo produces an *int* a *string_view* and a *float*. And you pass an lambda like this [](int, string_view){...}. At compile-time ezpz tries first (*int*) than (*int*,*string_view*) and than (*int*,*sting_view*,*float*) and check wether the given type is callable for these arguments and select the largest amount.
## Active RPOs
There maybe times where you dont care about all of the outputs of all of your RPOs. For this reason you can toggle your interest in the output by using the **'!'-operator**.
```c++
match("123 456",(!decimal<int>+ws+!decimal<int>)*print_all);
// => 123 456
match("123 456",( decimal<int>+ws+!decimal<int>)*print_all);
// => 456
match("123 456",(!decimal<int>+ws+ decimal<int>)*print_all);
// => 123
match("123 456",( decimal<int>+ws+ decimal<int>)*print_all);
// => 
```
So a non-activated rpo that gets combined with some other parser forgets about the output.

## Recursive Grammars
Recursion is a powerful tool in the definition of grammars. Since the parsing model of ezpz is based on PEG, indirectly left recursive grammars might loop forever. But ezpz still allows for recursion, even though it comes at a cost.
```c++
// type erased rpo possibly allocates on assignment
rpo<> parser;
// since type composition is done by copy or move and 'parser'
// is empty at the moment we need a form of indirection
// here the ezpz::ref parser meta function helps us out by
// wrapping a parse object around a reference of rpo<>
parser = "a"+optional(ref(parser));//parses a+
match("aaa",parser); // => true
```

# Reference
## Core
### ezpz::match
### ezpz::parse
### ezpz::match_or_undo
### ezpz::parse_or_undo
### ezpz::context
### ezpz::f_parser
### ezpz::fr_parser
### ezpz::parser_c
### ezpz::rparser_c
### ezpz::operator+
### ezpz::operator|
### ezpz::operator!
### ezpz::operator*
## Helper
### ezpz::ref
### ezpz::rpo
### ezpz::capture
### ezpz::print
### ezpz::fail
## Matcher
### ezpz::number<integer_t,base>
### ezpz::decimal<integer_t> = ezpz::number<integer_t,10>
### ezpz::alpha
### ezpz::single
### ezpz::digit
### ezpz::string
### ezpz::regex
### ezpz::eoi
## Quantifier
### ezpz::any
### ezpz::plus
### ezpz::optional
### ezpz::min
### ezpz::max
### ezpz::range
### ezpz::exact
### ezpz::not_v
## Unparser
### ezpz::print_all
### ezpz::assign
### ezpz::ret