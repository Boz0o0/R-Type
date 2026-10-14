#pragma once

#include <ampersand/core/ecs/ComponentId.hpp>
#include <ampersand/core/ecs/ComponentPool.hpp>
#include <ampersand/core/ecs/ECSConfig.hpp>
#include <ampersand/core/ecs/Entity.hpp>
#include <ampersand/core/ecs/Resource.hpp>
#include <ampersand/core/ecs/ResourceId.hpp>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <queue>
#include <utility>
#include <vector>

namespace ampersand::core::ecs {

/**
 * @brief Owns the entities of one game and their components.
 *
 * Components are stored in one pool per type. Each entity has a row in
 * the position table with one cell per component type: the position of its
 * component in the pool (+1, so remove 1 when using it), or 0 when it has none
 *
 * A World is used by a single thread.
 */
class World {
public:
    /**
     * @brief Creates an empty World.
     *
     * @param maxEntities Maximum number of entities alive at once.
     */
    explicit World(std::uint32_t maxEntities = kDefaultMaxEntities);

    /**
     * @brief Creates a new entity with no component.
     *
     * @return Handle to the new entity.
     * @throws EntityLimitReached if maxEntities entities are already alive.
     */
    Entity create();

    /**
     * @brief Destroys an entity and frees its row for a later create().
     *
     * The version of the row is incremented, so every handle to the destroyed
     * entity stops being alive, even after the row is reused.
     *
     * @param entity Entity to destroy.
     * @throws InvalidEntity if the entity is dead, null or out of range.
     */
    void destroy(Entity entity);

    /**
     * @brief Checks whether a handle designates a live entity.
     *
     * @param entity Handle to check
     * @return true if the entity exists, false otherwise.
     */
    [[nodiscard]] bool alive(Entity entity) const;

    /**
     * @brief Attaches a component to an entity.
     *
     * @tparam T Component type, (deduced from the component arg).
     * @param entity Entity receiving the component.
     * @param component Component to attach, moved into the World.
     * @throws InvalidEntity if the entity is dead, null or out of range.
     * @throws ComponentAlreadyExists if the entity already has a T.
     */
    template <typename T>
    void add(Entity entity, T component) {
        requireNoComponent(entity, componentId<T>(), "add");
        const auto position = poolOf<T>().push(entity, std::move(component));
        _rows[entity.index][componentId<T>()] = position + 1;  // 0 = absent
    }

    /**
     * @brief Accesses a component of an entity.
     *
     * @tparam T Component type.
     * @param entity Entity owning the component.
     * @return Reference to the component.
     * @throws InvalidEntity if the entity is dead, null or out of range.
     * @throws ComponentNotFound if the entity has no T.
     */
    template <typename T>
    T& get(Entity entity) {
        requireComponent(entity, componentId<T>(), "get");
        const auto cell = _rows[entity.index][componentId<T>()];
        return poolOf<T>().get(cell - 1);  // cells store position + 1,
    }

    /**
     * @brief Checks whether an entity has a component of type T.
     *
     * @tparam T Component type.
     * @param entity Entity to check.
     * @return true if the entity has a component of type T, false otherwise.
     */
    template <typename T>
    [[nodiscard]] bool has(Entity entity) const {
        return alive(entity) && _rows[entity.index][componentId<T>()] != 0;
    }

    /**
     * @brief Removes a component of an entity.
     *
     * The pools last component moves into the freed position, and the row of
     * the entity owning it is updated.
     *
     * @tparam T Component type.
     * @param entity Entity owning the component.
     * @throws InvalidEntity if the entity is dead, null or out of range.
     * @throws ComponentNotFound if the entity has no T.
     */
    template <typename T>
    void remove(Entity entity) {
        requireComponent(entity, componentId<T>(), "remove");
        removeFromPool(entity, componentId<T>());
    }

    /**
     * @brief Lists the entities owning a component of type T.
     *
     * This is the pool's own list, not a copy. Inside a loop over it, do not
     * remove<T>(), add<T>() or destroy() an entity owning a T, or entities
     * get skipped. Destroy after the loop, or loop backwards by index.
     *
     * To iterate over several component types, loop on the rarest one and
     * filter the others with has().
     *
     * @tparam T Component type.
     * @return The entities owning a T (empty if none).
     */
    template <typename T>
    [[nodiscard]] const std::vector<Entity>& owners() {
        return poolOf<T>().owners();
    }

    /**
     * @brief Creates the World's single instance of resource type T.
     *
     * @tparam T Resource type.
     * @param args Arguments forwarded to the constructor of T.
     * @return Reference to the new resource.
     * @throws ResourceAlreadyExists if the World already has a T.
     */
    template <typename T, typename... Args>
    T& addResource(Args&&... args) {
        const auto index = resourceId<T>();
        requireNoResource(index, "addResource");
        if (_resources.size() <= index) {
            _resources.resize(index + 1);
        }
        _resources[index] =
            std::make_unique<Resource<T>>(std::forward<Args>(args)...);
        return static_cast<Resource<T>&>(*_resources[index]).get();
    }

    /**
     * @brief Accesses the resource of type T.
     *
     * @tparam T Resource type.
     * @return Reference to the resource.
     * @throws ResourceNotFound if the World has no T.
     */
    template <typename T>
    T& getResource() {
        const auto index = resourceId<T>();
        requireResource(index, "getResource");
        return static_cast<Resource<T>&>(*_resources[index]).get();
    }

    /**
     * @brief Checks whether the World has a resource of type T.
     *
     * @tparam T Resource type.
     * @return true if the World has a T, false otherwise.
     */
    template <typename T>
    [[nodiscard]] bool hasResource() const {
        return resourceExists(resourceId<T>());
    }

    /**
     * @brief Destroys the resource of type T.
     *
     * @tparam T Resource type.
     * @throws ResourceNotFound if the World has no T.
     */
    template <typename T>
    void removeResource() {
        const auto index = resourceId<T>();
        requireResource(index, "removeResource");
        _resources[index].reset();
    }

private:
    /**
     * @brief Removes the component an entity has in a column (swap-and-pop)
     * and keeps the position table consistent.
     *
     * @param entity Entity owning the component.
     * @param column Column of the component type.
     */
    void removeFromPool(Entity entity, std::size_t column);

    /**
     * @brief Throws InvalidEntity if the entity is not alive.
     *
     * @param entity Entity to check.
     * @param where Name of the calling function, for the error message.
     */
    void requireAlive(Entity entity, const char* where) const;

    /**
     * @brief Throws if the entity is not alive or has no component in a
     * column.
     *
     * @param entity Entity to check.
     * @param column Column of the component type.
     * @param where Name of the calling function, for the error message.
     * @throws InvalidEntity, ComponentNotFound
     */
    void requireComponent(Entity entity, std::size_t column,
                          const char* where) const;

    /**
     * @brief Throws if the entity is not alive or already has a component in
     * a column.
     *
     * @param entity Entity to check.
     * @param column Column of the component type.
     * @param where Name of the calling function, for the error message.
     * @throws InvalidEntity, ComponentAlreadyExists
     */
    void requireNoComponent(Entity entity, std::size_t column,
                            const char* where) const;

    /**
     * @brief Checks whether a resource slot holds a resource.
     *
     * @param index Id of the resource type.
     * @return true if the slot exists and is filled, false otherwise.
     */
    [[nodiscard]] bool resourceExists(std::size_t index) const;

    /**
     * @brief Throws ResourceNotFound if the World has no resource of a type.
     *
     * @param index Id of the resource type.
     * @param where Name of the calling function, for the error message.
     */
    void requireResource(std::size_t index, const char* where) const;

    /**
     * @brief Throws ResourceAlreadyExists if the World already has a resource
     * of a type.
     *
     * @param index Id of the resource type.
     * @param where Name of the calling function, for the error message.
     */
    void requireNoResource(std::size_t index, const char* where) const;

    /**
     * @brief One cell per component type: position in its pool + 1, or 0
     * when the entity has no component of that type.
     */
    using Row = std::array<std::uint32_t, kMaxComponents>;

    /**
     * @brief Returns the pool of component type T, creating it on first use.
     *
     * @tparam T Component type.
     * @return Reference to the pool of T.
     */
    template <typename T>
    ComponentPool<T>& poolOf() {
        auto& slot = _pools[componentId<T>()];
        if (!slot) {
            slot = std::make_unique<ComponentPool<T>>();
        }
        return static_cast<ComponentPool<T>&>(*slot);
    }

    std::uint32_t _maxEntities;
    std::vector<std::uint32_t> _versions;
    std::queue<std::uint32_t> _free;
    std::vector<Row> _rows;
    std::array<std::unique_ptr<IComponentPool>, kMaxComponents> _pools;
    std::vector<std::unique_ptr<IResource>> _resources;
};

}  // namespace ampersand::core::ecs
