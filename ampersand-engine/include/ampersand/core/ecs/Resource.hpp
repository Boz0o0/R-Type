#pragma once

#include <utility>

namespace ampersand::core::ecs {

/**
 * @brief Type-independent base of every Resource (to allow storing them in a
 * single array).
 */
class IResource {
public:
    /** @brief Destroys the resource. */
    virtual ~IResource() = default;

    IResource(const IResource&) = delete;
    IResource& operator=(const IResource&) = delete;

protected:
    /** @brief Creates the base of a resource. */
    IResource() = default;
};

/**
 * @brief Holds the single instance of a resource type in a World.
 *
 * @tparam T Resource type.
 */
template <typename T>
class Resource : public IResource {
public:
    /**
     * @brief Builds the resource from its constructor arguments.
     *
     * @param args Arguments forwarded to the constructor of T.
     */
    template <typename... Args>
    explicit Resource(Args&&... args) : _value(std::forward<Args>(args)...) {}

    /** @brief Accesses the resource. */
    [[nodiscard]] T& get() { return _value; }

private:
    T _value;
};

}  // namespace ampersand::core::ecs
