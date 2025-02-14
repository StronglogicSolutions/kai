#include <variant>
#include <type_traits>
#include <functional>
#include <string>

enum class type
{
  spike = 0x00,
  development = 0x01,
  defect = 0x02
};

template <typename... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

using args_t = std::vector<std::string>;

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

using defct_fn_t = std::function<void(std::integral_constant<type, type::defect>)>;
using spike_fn_t = std::function<void(std::integral_constant<type, type::spike>)>;
using devel_fn_t = std::function<void(std::integral_constant<type, type::development>)>;

using overloaded_vis_t = overloaded<defct_fn_t, devel_fn_t, spike_fn_t>;

class delegate
{
  public:
    delegate()
    : visitor_(make_visitor())
    {}

    static overloaded_vis_t make_visitor()
    {
      return overloaded{
        defct_fn_t{[](std::integral_constant<type, type::defect>) {
        }},
        devel_fn_t{[](std::integral_constant<type, type::development>) {
        }},
        spike_fn_t{[](std::integral_constant<type, type::spike>) {
        }}
      };
    }

    void process(auto type, auto info)
    {
      visit(type);
    }

  private:
    void work() {}

    using visitor_t = decltype(make_visitor());

    visitor_t visitor_;
};
