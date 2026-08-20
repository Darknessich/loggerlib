#pragma once

namespace Logger {
    template <typename... TFs> struct SOverloaded : TFs... {
        using TFs::operator()...;
    };

    template <typename... TFs> SOverloaded(TFs...) -> SOverloaded<TFs...>;
} // namespace Logger
