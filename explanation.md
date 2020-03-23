Usage of objects with monadic interfaces. Boost::optional as an example.
========================================================================

Motivation
----------

In C if a function may fail, it is common practice to have return parameter representing `error`, and in-out parameter representing `value`. Or vice-versa. Sometimes, more than one in-out parameter. Sometimes you can also see `NULL` pointers are used for the same purpose. In general it is a complete mess:

`int do_stuff(int* a);` // does return value `non-zero` mean success?

`int do_with_returning_error(int* a, int* b);` // what does return value represent?

`void another_function(int* a, int* b, int* c)` // yet another way to return error code

`int process(int a)` // sometimes special value represents error by itself

This approach is extremely poorly composable, besides, it's hard to keep a mental model of what is going on, and it is imossible to enforce *const-correctness*:

```
  int result = 0;
  int error = do_stuff(&result); 
  int result2 = 0; 
  int result3 = 0; 
  if (error == 0) { 
    error = do_with_returning_error(&result, &result2);     
    if (error != 0) {
        // handle error
    } else {
        another_function(&result, &result3, &error);
        if (error == 0) {
            error = process(result3);
            if (error != -1) {
                // use the value somehow
            }
        }
    }
  }
```

Not difficult to understand that cyclomatic complexity of such code grows the more error handling one has to do. Changing in the way errors are handled may require complete change of the control flow of the function. At the same time using different functions may require changing in logic of error handling and lead to more bugs.

In C++ in addition to all of these exceptions were added, which are also far from perfect and add even more problems when used without caution.

Solution from the point of view of type theory
----------------------------------------------

Type teory suggests to use option type encapsulating value and tagging it to signal if it is empty or not. I.e. we are extendig set of values `A` with exactly one more value, which represents `empty` value. In various programming languages it is represented in different ways: `Maybe`, `Option`, `optional`. There is distinction from `NullObject` pattern, since unlike `NullObject`, option type is:
1. enforced by type system.
2. doesn't require any special crafting to not affect calling functions.
3. as a consequence of 1. and 2., much harder to misuse .

Unlike returning or passing pointers to represent optional values, `option` type explicitly tells the user what is the contract of the function they are using.

Boost::optional
---------------

`boost::optional` is a option type provided by boost, it's implementation is header-only. While solving the issues of representing optional objects, it's usage still may be cumbersome. Since C++ doesn't have any form of pattern matching, engineer has to explicitly check if there is a value inside, and can call getters even without checking:

```
  boost::optional<int> a;    
  if (a != boost::none) {
    use_a(a.value());
  }
  a.value() // also compiles, but throws exception at runtime, not exactly what we want
  a.get() // nonchecked access, compiles and returns garbage
```

Besides this obvious problems, we are still facing a problem of having control flow interlacing with program logic itself, which creates noise and makes it difficult to enforce const-correctness. What is the solution? **MONADS**

What?
-----

Without going deep into category theory, **monad** is a wrapper around type: `M<T>`, which has *combinator* (let's call it `bind`) with signature: `bind(M<T>, function<M<U>(T)>) -> M<U>` and a *type converter* to wrap type into monad.

If you squint enough, you will notice, that `boost::optional` has both `flat_map` and `map`, which work as **combinator** and **type converter** respectively, thus providing a monadic interface. Besides that, `boost::optional` has `value_or` which lets you provide value returned in case there is `boost::none` inside, `value_or_eval` which lets you to provide a functor to execute. Sadly, it is missng `or_else` to handle only failing branch. It is worth noticing that functor passed into `value_or_eval` can not have return type `void`, because C++. But it is possible to workaround this issue by introducing empty type `Unit` (or by returning value provided as input).

So, how to use?
---------------

`boost::optional` provides a set of convenience methods to abstract boilerplate require for flow control, eversimplified they can be represented like this:

* `boost::optional<T>::map(std::function<U(T)> f) -> boost::optional<U>` functor `f` does operation on value of type `T` and returns value of type `U`, this value is then wrapped in optional. The function is called **only** if there is value inside, otherwise `boost::none` is returned.
* `boost::optional<T>::flat_map(std::function<boost::optional<U>(T)> f) -> boost::optional<U>` functor `f` does operation on value of type `T` and returns `boost::optional` wrapping type `U`, this value is then directly returned by `flat_map` (thus, it's "flatened", you do not get optional with optional inside). The function is called **only** if there is value inside, otherwise `boost::none` is returned.
* `boost::optional<T>::value_or(const T& default)` returns either value of optional, of `default`, if there is `boost::none` inside
* `boost::optional<T>::value_or_eval(std::function<U()> f)` like `value_or`, but accepts generator functor, which must return value of type, convertible to `T`

With this knowledge, it is possible to abstract all the checks. Let's say we take the code from C example and rewrite it with C++ and optional:

```
boost::optional<int> do_stuff();
boost::optional<std::tuple<int, int>> do_with_returning_error();
boost::optional<int> another_function(int a, int b);
boost::optional<int> process(int a);
  
{
  const auto result {
    do_stuff()
      .flat_map(do_with_returning_error)
      .flat_map([](const auto& value) { return another_function(std::get<0>(value), std::get<1>(value)); })
      .flat_map(process)};
  if (result != boost::none) {
    // use result.get()
  }
}
```

From this example you can see, that usage of optional erases the error type, but if you were returning null in case of error, or just true/false/special_value, then it's worth considering using it instead.

Alternatives?
--------------

[`std::optional`](https://en.cppreference.com/w/cpp/utility/optional), introduced in C++17, has approximately 50% of `boost::optional` functionality

[`tl::optional`](https://github.com/TartanLlama/optional), provides even better interface.
