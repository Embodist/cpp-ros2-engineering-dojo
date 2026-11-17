#pragma once
#include <algorithm>
#include <memory>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>
namespace dojo {
struct Stage {
    virtual ~Stage() = default;
    virtual std::vector<float> process(std::span<const float> input) const = 0;
};
class BiasStage final : public Stage {
public:
    explicit BiasStage(float bias) : bias_(bias) {}
    std::vector<float> process(std::span<const float> input) const override; // TODO
private: float bias_;
};
class ClampStage final : public Stage {
public:
    ClampStage(float low,float high);   // TODO: low <= high
    std::vector<float> process(std::span<const float> input) const override; // TODO
private: float low_ = 0; float high_ = 0;
};
class Pipeline {
public:
    void add(std::unique_ptr<Stage> stage); // TODO: reject nullptr
    std::vector<float> run(std::span<const float> input) const; // TODO
private: std::vector<std::unique_ptr<Stage>> stages_;
};
inline std::vector<float> BiasStage::process(std::span<const float> input) const {
    std::vector<float> out;
    out.reserve(input.size());
    for (float value : input) out.push_back(value + bias_);
    return out;
}

inline ClampStage::ClampStage(float low,float high) : low_(low),high_(high) {
    if (low > high) throw std::invalid_argument("low > high");
}

inline std::vector<float> ClampStage::process(std::span<const float> input) const {
    std::vector<float> out;
    out.reserve(input.size());
    for (float value : input) out.push_back(std::clamp(value, low_, high_));
    return out;
}

inline void Pipeline::add(std::unique_ptr<Stage> stage) {
    if (!stage) throw std::invalid_argument("null stage");
    stages_.push_back(std::move(stage));
}

inline std::vector<float> Pipeline::run(std::span<const float> input) const {
    std::vector<float> current(input.begin(), input.end());
    for (const auto& stage : stages_) {
        current = stage->process(current);
    }
    return current;
}

}
