#include <variant>
#include "helpers.hpp"
#include <concepts>
#include <type_traits>

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

using args_t = std::vector<std::string>;
class task
{
  public:
    enum class type
    {
      spike = 0x00,
      development = 0x01,
      defect = 0x02
    };
    task(type task_type)
    : type_(task_type)
    {
      auto create_task = [](const auto& name)
      {
        // 1. create fs
        //
        // defect/<name>
        // defect/<name>/info.md
        // defect/<name>/logs/{console,debug,recovery}
        // defect/<name>/steps/<date.md>
        //
        // 2. identify technologies
        //
        // 3. identify question
        //
        // 4. known/solvable?
        //   - try to solve 
        //   - evaluate solution and ask
        //
        // 5. 
      };
      switch (type_)
      {
        case type::spike:

        break;
        case type::development:

        break;

        case type::defect:

        break;
      }
    }

  private:
    type type_;
};


template <typename... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename F, typename... Variants>
concept invocablewith = (requires(F f, Variants... vars) { std::visit(f, vars...); });

template <typename F, typename... Variants>
requires invocablewith<F, Variants...>
decltype(auto) visit(F&& f, Variants&&... variants)
{
  return std::visit(std::forward<F>(f), std::forward<Variants>(variants)...);
}

class delegate
{
  private:
    void work()
    {
      auto visitor = overloaded{
      [](std::integral_constant<task::type, task::type::defect>)
      {

      },
      [](std::integral_constant<task::type, task::type::development>)
      {

      },
      [](std::integral_constant<task::type, task::type::spike>)
      {

      }
      };
    }
};
