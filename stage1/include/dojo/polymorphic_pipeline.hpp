#pragma once
#include <memory>
#include <span>
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
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
inline std::vector<float> BiasStage::process(std::span<const float> input) const { (void)input; return {}; /* TODO */ }
inline ClampStage::ClampStage(float low,float high) : low_(low),high_(high) { /* TODO: validate */ }
inline std::vector<float> ClampStage::process(std::span<const float> input) const { (void)input; return {}; /* TODO */ }
inline void Pipeline::add(std::unique_ptr<Stage> stage) { (void)stage; /* TODO */ }
inline std::vector<float> Pipeline::run(std::span<const float> input) const { (void)input; return {}; /* TODO */ }

}
