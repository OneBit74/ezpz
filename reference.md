# ezpz Reference Documentation
## Fundamentals
### **What is a parser?**
A parser is a program that reads tokens, decides whether the sequence conforms to a grammar and produces some kind of output. So in **ezpz** a parser looks like this
```c++
struct my_parser{
    using ezpz_output = ezpz::TLIST<int, float, string>;
    using ezpz_prop = ezpz::TLIST<>; // this is optional

    bool _parse(auto& ctx, int&, float&, string&);
}
```
The output is defined in a type list in a using declaration of ezpz_output. The parser can have many outputs but also zero. The ezpz_prop declaration can be defined optionally. Its purpose is to convey additional information e.g. the parser will always return true, then we can raise a compiletime error, if a always-accepting-parser is the lhs of an or-combinator. Next, we have to define the actual _parse member-function, which returns whether the parse was successful or not. The first argument is always the current parsing context. I recommend to use auto here, because it allows you to pass different kinds of contexts and currently **recover** relies on passing a special context that wraps your context. The rest of the parameters are your output parameters in the order they were defined in the ezpz_output as a reference.

As you can see, **ezpz** does not lock you into a specific way of writing your parsers. You can always opt-out and use a handwritten parser for a part of your grammar or even a completely different parsing library.

### **What is a context?**
A context defines the token type and manages the token stream that parsers consume. Furthermore, a context can handle all sorts of tasks that concern the entire parsing process. For example a context can issue error messages, hold some kind of semi global state or take a look at [include/ezpz/extra/graph_context](), which prints the parse tree in the [*dot attributed graph language*](https://graphviz.org/doc/info/lang.html).

The interface of a context in ezpz can not be defined strictly, certain parts of the library use certain interfaces others don't. If you do not use certain library features, you won't have to specify those.
- ctx.getPosition() -> pos: the current position in the token stream
- ctx.setPosition(pos): argument type should be return type of getPosition()
- ctx.token(): returns the current token
- ctx.advance(): advances the token stream to the next token
- ctx.done() -> bool: should return true if there are no more tokens left to read. A call to ctx.token() would be undefined behavior in this case.
- ctx.notify_enter(parser) -> something(optional): called whenever a parser is about to be entered can return something
- ctx.notify_leave(parser,bool success, something(optional)): called after the parser is finished with the data from the notify_enter call

Take a look at the implementation of [min_context](include/ezpz/context.hpp) to see a very minimalistic definition of a context.

### **How do I parse?**
Now that you know what a parser and a context are, you need to know the idiomatic way of calling the parser. You should *never* call the _parse member function of your parser types. Instead, use the ezpz::parse free function.
```c++
ezpz::parse(ctx,parser)
ezpz::parse(ctx,parser,output...)
```
These functions take care of notifying the context (ctx.notify_enter, ctx.notify_leave) and instantiating the proper outputs if you didn't provide any. You have to either provide no outputs or all the outputs defined in the ezpz_output of your parser.

There is also the ezpz::parse_or_undo free function, which delegates to ezpz::parse that automatically resets the parser to its original position, in case the parse fails. 
```c++
ezpz::parse_or_undo(ctx,parser)
ezpz::parse_or_undo(ctx,parser,output...)
```
# Reference
|Group|Elements|
|---|---|
|Core|[operator+](#operatorp1p2--and_p) [operator\|](#operatorp1p2--or_p) [operator!](#operatorp1--forget) [operator\*](#operatorp1-f--consume_p) [ref(p)](#refp1) [rpo<>](#rpoctxoutputs-and-polymorphic_rpoctxoutputs) [make_rpo<>()](#make_rpooutputsf) 
|Matcher|[text_p](#text_p) [text_ci_p](#text_ci_p) [number<num_t,base>](#numbernum_t-base--decimalnum_t) [eoi](#eoi) [ws](#ws) [string](#string) [alpha](#alpha) [digit](#digit) [graph_letter](#graph_letter) [single](#single) [token(t)](#tokenx) [accept_if](#accept_iff)
|Consumer| [assign(dst)](#assigndst) [insert(dst)](#insertdst) [print_all](#print_all) [cast\<T\>](#cast) [into\<T\>](#into)
|Quantifers| [any(p)](#anyp1) [plus(p)](#plusp1) [notf(p)](#notfp1) [peek(p)](#peekp1) [optional(p)](#optionalp1) [min_p](#minamountp1-minamountp1) [max_p](#maxamountp1-maxamountp1) [times](#timesamountp1-timesamountp1)
|Helper| [recover(p)](#recoverp1) [merge(p)](#mergep1) [agg(p,f)](#aggp1f-and-agg_intotp1f) [print("")](#printtext) [capture(p)](#capturep1) [ret<...>](#retvals) 
|Context| [min_context](#min_context) [basic_context](#basic_context) [forward_range_context<R>](#forward_range_context)
|Extra| [graph_context](#graph_contextctx_t-t) [cin_context](#cin_context)
## Core
### **operator+(p1,p2) / and_p**
This combinator can be thought of as "parse p1 then p2". It starts by parsing p1. If that parse was successful the ctx state is maintained and p2 is parsed. If the parse of p2 was successful as well, and_p will return true. The output of and_p is the concatenation of the outputs of p1 and p2.

### **operator|(p1,p2) / or_p**
This combinator can be thought of as "parse p1 or p2".
It starts by parsing p1. Only if that parse was unsuccessful p2 will be parsed. If one of those parses was successful, or_p will return true.

The output of or_p is quite complicated. The easy cases are when p1 and p2 have the same outputs. In which case, or_p outputs the same. For different outputs, or_p's output type might consist of std::variant, std::optional, std::tuple (**TODO**).

Additionally, there is a static_assertion in operator|, which checks whether p1 returns always true, by checking if the ezpz_prop list contains ezpz::always_true. This would mean that, rhs is never entered. So keep that in mind in case you define non-standard parsers that always return true.

### **operator!(p1) / forget**
This combinator silences the output of p1, by setting ezpz_output to an empty list.

### **operator\*(p1, f) / consume_p**
This operator takes a parser p1 and a **consumer** callback f. f should be callable by a subarray of the ezpz_output types of p1. The beginning of that subarray must start at the left_most type.
For example, if p1 provides outputs o1;o2;o3, then f should be callable by one of these
- f() 
- f(o1) 
- f(o1,o2) 
- f(o1,o2,o3)

If f is callable by multiple amounts of outputs, the candidate that consumes the most amount of outputs is chosen. This allows the callback to be variadic.

The consumer can also produce a new output as its return value. This output will be placed at the top of the ezpz_output list. So if the new output is x the outputs should look like this
- (x,o1,o2,o3)
- (x,o2,o3)
- (x,o3)
- (x)
### **ref(p1)**
Normally parsers get copied or moved into more complex parsers. With ref(...) the resulting parser stores a reference to p1, and delegates the parsing call to p1 accordingly.
### **rpo<ctx,outputs...>** and **polymorphic_rpo<ctx,outputs...>**
These types support operator= for parsers that have _parse for the given context and outputs. A _parse call to these types will be delegated to the assigned parsers. This type erasure allows for recursive grammars. Note that rpo uses std::funtion, which might allocate memory. polymorphic_rpo only supports assignment to polymorphic types, which can be created by using **make_poly<ctx,outputs...>(p)**. Since polymorphic_rpo only stores a pointer to a polymorphic parser the creator has to ensure that the assigned parsers lifetime exceeds the lifetime of the **polymorphic_rpo**.
### **make_rpo<outputs...>(f)**
Factory function for anonymous parsers. f basically implements _parse, but optionally f can accept one more argument between the ctx and the outputs. This argument is the parser f implements itself, so that you can use it for recursion.
- f(ctx, outputs...)
- f(ctx,*this, outputs...)
This is generally the safest approach to recursive grammars, since you don't have the risk of dereferencing invalid pointers.
---
## Matchers
### **text_p**
Parses a static string of text. ctx.token() must be equality comparable with **char**.
You can create these parsers by using the user defined literal _p. Some combinators will convert string_literals to text_p implicitly.
```c++
"this is a text parser"_p
```
### **text_ci_p**
Like text_p but case insensitive. Constructable with _cip user defined literal operator.
### **number<num_t, base> / decimal<num_t>**
number<> matches numbers with digits between base and 0. You provide the target number type, which can be an integer or a floating point type. Depending on whether num_t is a floating point type, number<> will also check for a decimal point.

decimal<> is a specialization of number for base 10. base must be between 10 and 2.
### **eoi**
Matches the end of input for the context, by returning ctx.done().
### **ws**
Matches all available whitespace (' ', '\t', '\n', '\r').
### **string**
Matches \\\" and all characters until the closing \\\"
### **alpha**
Matches a single alphabetical character.
### **digit**
Matches a single digit.
### **graph_letter**
Matches a single graphical letter.
### **single**
Matches a single token.
### **token(x)**
Matches a token that is equal to x.
### **accept_if(f)**
f is called with the current token. If f returns true, this token is accepted.

---
## Consumer
### **assign(dst)**
Returns a callable that takes one argument and assigns it to dst.
### **insert(dst)**
Returns a callable that takes one argument and inserts it into dst. It is overloaded for different standard containers and calls the right method. If dst is a map, the callback will expect a key and a value.
### **print_all**
Prints all outputs to std::cout.
### **cast\<T\>**
Expects one output and casts it to T. If the output is a std::variant, its inner values will be visited and cast to T.
### **into\<T\>**
Expects the maximum amount of args for which T is constructable and returns a T.

---
## Quantifiers
### **any(p1)**
Parses p1 zero to infinity many times. Returns the same output as p1.
### **plus(p1)**
Parses p1 one to infinity many times.
### **notf(p1)**
If p1 parses successful, the parser fails.
If p1 does not parse successfully, the parser succedes.
This parser does not consume any tokens.
### **peek(p1)**
Parses p1, but does not consume any tokens.
### **optional(p1)**
Parses p1 zero or one time. The output type is std::optional<...> for a single output and std::optional<std::tuple<...>> for multiple outputs.
### **min\<amount\>(p1) min(amount,p1)**
Parses p1 $amount to infinity many times. Returns the same output as p1.
### **max\<amount\>(p1) max(amount,p1)**
Parses p1 zero to $amount many times. Returns the same output as p1.
### **times\<amount\>(p1) times(amount,p1)**
Parses p1 exactly $amount many times. Returns the same output as p1.

---
## Helper
### **print(text)**
Is a parser that doesnt parse any tokens. It just prints text and returns true.
### **capture(p1)**
Only works with basic_context. The output of this parser is a std::string_view from the first to the last token that was matched by p1.
### **ret<vals...>**
Does not match any tokens. The output of this parser are the vals... parameters.
### **recover(p1)**
If p1 fails to parse, a error occoured. This parser always returns true, so the parsing can continue. The recover parser tries to inspect the p1 parser by performing a secondary parse with a modified context. In this secondary parse, leaf parsers that fail will be tracked and reported to the original ctx. The ctx then can use the candidate information to construct an error message. 
- ctx.error(std::vector\<candidate\>)
```c++
struct candidate {
    std::string parser_description;
    int goodness;
    decltype(ctx.getPosition()) start_pos, end_pos;
}
```

You can modify the behavior of recover by providing a type configuration. (**TODO**)

### **merge(p1)**
If p1 has multiple outputs of the same type T, the resulting parser will have only one output of type T. The returning parsers that produced Ts, will be given a reference to the same T. For example ...
```c++
merge(decimal<int>+" "+decimal<int>)
```
This parser outputs a single int. If a parse is successful the final output value will be that of the parser that last produced.
### **agg(p1,f)** and **agg_into\<T\>(p1,f)**
These parsers are intended to buildup aggregate values. They are usefull in conjunction with **merge**. **agg** outputs the same as p1. f should be callable with the outputs of p1 twice. First f will be given as arguments the aggregate result of this parse and then the result of p1. **agg_into** outputs a value of T. f should be callable with T and the output values of p1. Example:
```c++
// summing over numbers
auto add = [](int& agg, int val){
    agg += val;
};
auto sum = merge(decimal<int> + any(ws+"+"+ws+agg(decimal<int>,add)));
int total = 0;
EXPECT_TRUE(parse("1+2 + 3+ 4 +5",sum,total));
EXPECT_EQ(total,15);
```

```c++
// building a list of integers
auto push = [](std::vector<int>& agg, int val){
    agg.push_back(val);
};
auto element = agg_into<std::vector<int>>(decimal<int>,push);
auto list = merge(element+ any(ws+","+ws+element));

std::vector<int> result;
EXPECT_TRUE(parse("1,2 , 3, 4 ,5",list,result));
std::vector<int> expected{1,2,3,4,5};
EXPECT_EQ(result,expected);
```
---
## Context
### **min_context**
This context manages a sequence of char tokens. It is lightweight and minimal. Provides no debugging or error handling utilities.

### **basic_context**
This context derives from min_context. You can enable a *debug-mode* by setting ctx.debug = true. This context supports errors from recover. Additional interfaces are:
- ctx.failed() == true => a recover was triggered.
- ctx.reset_failed() => resets internal error flag
- ctx.setResource(std::string) => the string is used in error messages in case the token stream stems from a certain file.

### **forward_range_context<range>**
This context provides basic functionality for any forward range. The value type of the given range is the token type of this context.

--- 
## Extra
### **graph_context<ctx_t, T...>**
This context derives from the provided ctx_t. It has a print() interface, which will print the parse tree in the [*dot attributed graph language*](https://graphviz.org/doc/info/lang.html). The types T... are used to construct the parse tree. If a parser is encounter, whose type is contained in T..., it will appear in the parse tree. There are some addtional types that can be provided in this list.
1. graph_option_show_failures: when this type is provided the parse tree will also contain nodes of failed parse attempts
2. graph_option_show_all: when this type is provided you dont have to provide any parser types yourself. Every parser is recorded.

### **cin_context**
This context buffers characters from std::cin.
