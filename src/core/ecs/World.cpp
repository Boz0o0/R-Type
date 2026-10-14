#include <ampersand/core/ecs/ECSExceptions.hpp>
#include <ampersand/core/ecs/World.hpp>
#include <cstddef>
#include <string>

namespace ampersand::core::ecs {

namespace {

std::string describe(Entity entity) {
    return "{" + std::to_string(entity.index) + ", " +
           std::to_string(entity.version) + "}";
}

}  // namespace

World::World(std::uint32_t maxEntities) : _maxEntities(maxEntities) {
    _versions.reserve(maxEntities);
    _rows.reserve(maxEntities);
}

Entity World::create() {
    if (!_free.empty()) {
        const auto index = _free.front();
        _free.pop();
        return {.index = index, .version = _versions[index]};
    }
    if (_versions.size() >= _maxEntities) {
        throw EntityLimitReached(std::to_string(_maxEntities) + " entities");
    }
    const auto index = static_cast<std::uint32_t>(_versions.size());
    _versions.push_back(1);
    _rows.emplace_back();  // every cell is 0
    return {.index = index, .version = _versions[index]};
}

void World::destroy(Entity entity) {
    requireAlive(entity, "destroy");
    // Remove every component of the entity
    for (std::size_t column = 0; column < kMaxComponents; column++) {
        if (_rows[entity.index][column] != 0) {
            removeFromPool(entity, column);
        }
    }
    auto& version = _versions[entity.index];
    version++;
    if (version == 0) {
        version = 1;
    }
    _free.push(entity.index);
}

void World::removeFromPool(Entity entity, std::size_t column) {
    const auto position = _rows[entity.index][column] - 1;
    const Entity moved = _pools[column]->removeAt(position);
    if (moved != Entity{}) {
        _rows[moved.index][column] = position + 1;
    }
    _rows[entity.index][column] = 0;
}

void World::requireAlive(Entity entity, const char* where) const {
    if (!alive(entity)) {
        throw InvalidEntity(describe(entity) + " in " + where);
    }
}

void World::requireComponent(Entity entity, std::size_t column,
                             const char* where) const {
    requireAlive(entity, where);
    if (_rows[entity.index][column] == 0) {
        throw ComponentNotFound(describe(entity) + " in " + where +
                                ", component #" + std::to_string(column));
    }
}

void World::requireNoComponent(Entity entity, std::size_t column,
                               const char* where) const {
    requireAlive(entity, where);
    if (_rows[entity.index][column] != 0) {
        throw ComponentAlreadyExists(describe(entity) + " in " + where +
                                     ", component #" + std::to_string(column));
    }
}

bool World::resourceExists(std::size_t index) const {
    return index < _resources.size() && _resources[index] != nullptr;
}

void World::requireResource(std::size_t index, const char* where) const {
    if (!resourceExists(index)) {
        throw ResourceNotFound("#" + std::to_string(index) + " in " + where);
    }
}

void World::requireNoResource(std::size_t index, const char* where) const {
    if (resourceExists(index)) {
        throw ResourceAlreadyExists("#" + std::to_string(index) + " in " +
                                    where);
    }
}

bool World::alive(Entity entity) const {
    return entity.index < _versions.size() &&
           _versions[entity.index] == entity.version;
}

}  // namespace ampersand::core::ecs
