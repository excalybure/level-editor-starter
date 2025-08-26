export module runtime.ecs;
import std;
import <cstdint>;

export namespace ecs {
export struct Entity {
    std::uint32_t id{};
    auto operator<=>(const Entity&) const = default;
};

export template<class T>
concept Component = std::is_trivially_copyable_v<T>;

export template<Component C>
struct Storage {
    std::vector<C> dense;
    std::vector<uint32_t> sparse; // id -> index

    [[nodiscard]] Entity create(C c = {}) {
        std::uint32_t id = static_cast<std::uint32_t>(sparse.size());
        sparse.push_back(static_cast<std::uint32_t>(dense.size()));
        dense.push_back(c);
        return {id};
    }
    [[nodiscard]] bool has(Entity e) const { return e.id < sparse.size(); }
    C& get(Entity e) { return dense[sparse[e.id]]; }
};
} // namespace ecs
