template <int First, int Last>
struct static_for
{
    template <typename Lambda>
    static inline constexpr void apply(Lambda const& f) {
        if (First < Last)
        {
            f(std::integral_constant<int, First>{});
            static_for<First + 1, Last>::apply(f);
        }
    }
};
template <int N>
struct static_for<N, N>
{
    template <typename Lambda>
    static inline constexpr void apply(Lambda const& f) {}
};
