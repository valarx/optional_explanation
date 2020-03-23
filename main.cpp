#include <algorithm>
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <boost/range/algorithm.hpp>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <iterator>
#include <string>

// Example 1
boost::optional<double> safe_square_root(const std::int32_t a_param) noexcept {
  if (a_param < 0) {
    return boost::none;
  } else {
    return std::sqrt(a_param);
  }
}

double get_default() { return .0; }

// Example 2
struct Person {
  std::string first_name;
  boost::optional<std::string> middle_name;
  std::string last_name;

  boost::optional<std::string> GetMiddleName() const { return middle_name; }
};

// may fail, if the failure is nor normal scenario,
// then error handling should be done with exceptions
boost::optional<Person> maybe_load_from_file() {
  return Person{"John", boost::none, "Doe"};
}

int main() {
  const auto invalid_res{safe_square_root(-1)};
  assert(invalid_res == boost::none);
  const auto valid_res{safe_square_root(1)};
  assert(valid_res.is_initialized());

  // example of sad evaluation chaining
  {
    double evaluations_sequence = .0;
    bool has_error{false};

    if (valid_res != boost::none) {
      const auto tmp_res{valid_res.get()};
      const auto negated_tmp_res{std::negate<double>{}(tmp_res)};
      const auto new_res{safe_square_root(negated_tmp_res)};

      if (new_res != boost::none) {
        evaluations_sequence = new_res.get() * 2;
      } else {
        has_error = true;
      }
    } else {
      has_error = true;
    }

    if (has_error) {
      printf("sad value is %f\n", evaluations_sequence);
    } else {
      printf("sad value is invalid\n");
    }
  }

  // example of proper evaluations chaining
  {
    const auto evaluations_sequence{
        valid_res
            // `map` is used if the functor returns value of type `T`, then it
            // wraps it into `boost::optional<T>`
            .map([](const auto value) { return value + 3; })
            .map(std::negate<double>{})
            // `flat_map` is used if the functor
            // returns value of type `optional<T>`, then it forwards it directly
            .flat_map(safe_square_root)
            .map([](const auto value) { return value * 2; })};

    // sadly we do not have `or_else` method, and both map and flat_map must
    // return something, so when we end evaluation chain, we have to
    // explicitly check for the value present, if we want to handle both
    // branches:
    if (evaluations_sequence != boost::none) {
      printf("value is %f\n", evaluations_sequence.get());
    } else {
      printf("value is invalid\n");
    }

    // However, if in case of failure it is possible to have a default value,
    // then one can use `value_or`:
    {
      const auto result{evaluations_sequence.value_or(NAN)};
      printf("got value %f\n", result);
    }
    // It is possible to call generators, if they have `operator ()` returning
    // type convertible to the type of optional
    {
      const auto result{evaluations_sequence.value_or_eval(get_default)};
      printf("got value %f\n", result);
    }
  }

  // More complex example

  // Traditional approach with for loop
  {
    const auto person{maybe_load_from_file()};

    if (person != boost::none) {
      const auto middle_name{person->GetMiddleName()};

      if (middle_name != boost::none) {
        std::string temp;

        for (const auto val : middle_name.value()) {
          temp.push_back(std::toupper(val));
        }
      }
    }
  }

  // Monadic approach with ranges
  {
    const auto person{maybe_load_from_file()};
    const auto capitalized_middle_name{
        person.flat_map([](const auto &value) { return value.GetMiddleName(); })
            .map([](const auto &value) {
              std::string temp;
              boost::transform(
                  value, std::back_inserter(temp),
                  [](const auto value) { return std::toupper(value); });
              return temp;
            })};
    assert(capitalized_middle_name == boost::none);
  }

  // Possible way to combine multiple optionals
  // option<int> + option<int>

  boost::optional<int> a{5};
  boost::optional<int> b{boost::none};

  // With ifs
  {
    boost::optional<int> result;

    if (a != boost::none && b != boost::none) {
      result = a.value() + b.value();
    } else {
      result = boost::none;
    }
    assert(result == boost::none);
  }

  // Functional approach
  {
    const auto result{a.flat_map([&b](const auto value) {
      return b.map([&value](const auto value_1) {
        return value + value_1;
      }); // this can grow your stack if overused, since no TCO guaranteed in
          // C++
    })};
    assert(result == boost::none);
  }
  return 0;
}