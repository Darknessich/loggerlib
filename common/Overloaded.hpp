#pragma once

namespace Common {
    /// @brief Bundles lambdas into one callable, the visitor of a std::variant.
    /// @tparam TFs callables, one per alternative of the variant
    template <typename... TFs> struct SOverloaded : TFs... {
        using TFs::operator()...;
    };

    /// @brief Deduces SOverloaded from the lambdas it is built from.
    /// @tparam TFs callables the visitor is built from
    template <typename... TFs> SOverloaded(TFs...) -> SOverloaded<TFs...>;
} // namespace Common
