#include <ampersand/render/components/Animation.hpp>

namespace ampersand::render {

bool isFinished(const Animation& animation) {
    return !animation.loop &&
           animation.elapsed >= animation.frameDuration *
                                    static_cast<float>(animation.frames.size());
}

}  // namespace ampersand::render
