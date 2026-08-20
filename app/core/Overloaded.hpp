#pragma once

namespace App {
    template <typename... TFs> struct SOverloaded : TFs... {
        using TFs::operator()...;
    };

    template <typename... TFs> SOverloaded(TFs...) -> SOverloaded<TFs...>;
} // namespace App
