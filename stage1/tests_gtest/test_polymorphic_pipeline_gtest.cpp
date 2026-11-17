#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "dojo/polymorphic_pipeline.hpp"

TEST(Pipeline, RunsStagesInOrder) {
    dojo::Pipeline pipeline;
    pipeline.add(std::make_unique<dojo::BiasStage>(2));
    pipeline.add(std::make_unique<dojo::ClampStage>(0, 3));
    std::vector<float> input{-4, 0, 3};
    auto output = pipeline.run(input);
    ASSERT_EQ(output.size(), 3u);
    EXPECT_FLOAT_EQ(output[0], 0.0f);
    EXPECT_FLOAT_EQ(output[1], 2.0f);
    EXPECT_FLOAT_EQ(output[2], 3.0f);
}

TEST(Pipeline, EmptyPipelineCopiesInput) {
    dojo::Pipeline pipeline;
    std::vector<float> input{1, 2};
    auto output = pipeline.run(input);
    EXPECT_EQ(output, input);
}

TEST(Pipeline, RejectsNullStage) {
    dojo::Pipeline pipeline;
    EXPECT_THROW(pipeline.add(nullptr), std::invalid_argument);
}

TEST(Pipeline, RejectsInvalidClampRange) {
    EXPECT_THROW(dojo::ClampStage(5, 2), std::invalid_argument);
}

TEST(Pipeline, StageOrderMatters) {
    dojo::Pipeline bias_then_clamp;
    bias_then_clamp.add(std::make_unique<dojo::BiasStage>(2));
    bias_then_clamp.add(std::make_unique<dojo::ClampStage>(0, 3));

    dojo::Pipeline clamp_then_bias;
    clamp_then_bias.add(std::make_unique<dojo::ClampStage>(0, 3));
    clamp_then_bias.add(std::make_unique<dojo::BiasStage>(2));

    std::vector<float> input{5};
    EXPECT_FLOAT_EQ(bias_then_clamp.run(input)[0], 3.0f);
    EXPECT_FLOAT_EQ(clamp_then_bias.run(input)[0], 5.0f);
}
