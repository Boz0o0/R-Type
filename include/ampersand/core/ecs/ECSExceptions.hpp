#pragma once

#include <stdexcept>
#include <string>

namespace ampersand::core::ecs {

/**
 * @brief Base class of every error thrown by the ECS.
 *
 * Catching it lets a server log a faulty system and keep the game running.
 */
class ECSError : public std::runtime_error {
public:
    /**
     * @brief Creates an ECS error.
     *
     * @param message Description of the error.
     */
    explicit ECSError(const std::string& message)
        : std::runtime_error(message) {}
};

/**
 * @brief Thrown when an entity handle is dead, null or out of range.
 *
 * Raised by add, get, remove and destroy (a double destroy included).
 */
class InvalidEntity : public ECSError {
public:
    /**
     * @brief Creates the error.
     *
     * @param details The faulty handle and the operation that used it.
     */
    explicit InvalidEntity(const std::string& details)
        : ECSError("Invalid entity: " + details) {}
};

/** @brief Thrown by add when the entity already has this component type. */
class ComponentAlreadyExists : public ECSError {
public:
    /**
     * @brief Creates the error.
     *
     * @param details The entity and the component type.
     */
    explicit ComponentAlreadyExists(const std::string& details)
        : ECSError("Component already exists: " + details) {}
};

/** @brief Thrown by get and remove when the entity lacks the component. */
class ComponentNotFound : public ECSError {
public:
    /**
     * @brief Creates the error.
     *
     * @param details The entity and the component type.
     */
    explicit ComponentNotFound(const std::string& details)
        : ECSError("Component not found: " + details) {}
};

/** @brief Thrown by create when the World's entity limit is reached. */
class EntityLimitReached : public ECSError {
public:
    /**
     * @brief Creates the error.
     *
     * @param details The limit that was reached.
     */
    explicit EntityLimitReached(const std::string& details)
        : ECSError("Entity limit reached: " + details) {}
};

/** @brief Thrown on the first use of a component type beyond the limit. */
class ComponentLimitReached : public ECSError {
public:
    /**
     * @brief Creates the error.
     *
     * @param details The limit that was reached.
     */
    explicit ComponentLimitReached(const std::string& details)
        : ECSError("Component type limit reached: " + details) {}
};

/** @brief Thrown by addResource when the World already has this type. */
class ResourceAlreadyExists : public ECSError {
public:
    /**
     * @brief Creates the error.
     *
     * @param details The resource type.
     */
    explicit ResourceAlreadyExists(const std::string& details)
        : ECSError("Resource already exists: " + details) {}
};

/** @brief Thrown by getResource and removeResource when the World has no
 * resource of this type. */
class ResourceNotFound : public ECSError {
public:
    /**
     * @brief Creates the error.
     *
     * @param details The resource type.
     */
    explicit ResourceNotFound(const std::string& details)
        : ECSError("Resource not found: " + details) {}
};

}  // namespace ampersand::core::ecs
