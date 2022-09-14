/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// intentionally omit header guard
// DO NOT MANULLY INCLUDE THIS FILE

namespace fixmath {

using int32_t = int;
using uint32_t = unsigned int;
using int64_t = decltype(0LL);
using uint64_t = decltype(0ULL);

static_assert(sizeof(int32_t) == 4 && sizeof(int64_t) == 8, "what the fudge");

enum class arithmetic_mode {
    Ignore,
    StrictMode,
    SaturationMode,
};

enum class rounding_mode {
    RoundToZero,
    RoundToEven,
};

template<
    class underlying_type,
    underlying_type fraction,
    arithmetic_mode arith_mode,
    rounding_mode rounding_mode
>
struct fixed_policy {
    using raw_t = underlying_type;
    const static raw_t fraction_bits = fraction;
    const static bool ignore_mode = arith_mode == arithmetic_mode::Ignore;
    const static bool strict_mode = arith_mode == arithmetic_mode::StrictMode;
    const static bool saturation_mode = arith_mode == arithmetic_mode::SaturationMode;
    const static bool rounding = rounding_mode == rounding_mode::RoundToEven;

    static_assert(::std::is_signed_v<raw_t>, "underlying_type should be signed");
    static_assert(0 < fraction_bits && fraction_bits < sizeof(raw_t) * CHAR_BIT, "fraction bits should between [1, BITS - 1]");
};

template<class T>
struct is_fixed_policy : ::std::false_type {};

template<
    class underlying_type,
    underlying_type fraction,
    arithmetic_mode arith_mode,
    rounding_mode rounding_mode
>
struct is_fixed_policy<fixed_policy<underlying_type, fraction, arith_mode, rounding_mode>> : ::std::true_type {};

template<class T>
constexpr bool is_fixed_policy_v = is_fixed_policy<T>::value;

template<class T>
concept FixedPolicy = is_fixed_policy_v<T>;

template<FixedPolicy policy>
class fixed;

template<class T>
struct is_fixed : ::std::false_type {};

template<FixedPolicy T>
struct is_fixed<fixed<T>> : ::std::true_type {};

template<class T>
constexpr bool is_fixed_v = is_fixed<T>::value;

template<class T>
concept Fixed = is_fixed_v<T>;

template<class T>
concept PromotesToInt32 = ::std::same_as<decltype(+::std::declval<T>()), int32_t>;

template<class T, class U>
concept FixedImplicitBinaryOperable = (Fixed<T> && PromotesToInt32<U>)
                                    || (Fixed<U> && PromotesToInt32<T>);

} // namespace fixmath

namespace std {

template<class T1, class T2> requires ::fixmath::FixedImplicitBinaryOperable<T1, T2>
struct common_type<T1, T2> {
    using type = ::std::conditional_t<::fixmath::is_fixed<T1>::value, T1, T2>;
};

}// namespace std
