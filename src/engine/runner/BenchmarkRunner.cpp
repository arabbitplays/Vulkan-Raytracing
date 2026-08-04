#include "BenchmarkRunner.hpp"

#include <filesystem>
#include <omp.h>
#include <ranges>

#include "ImageUtil.hpp"
#include "PathUtil.hpp"
#include "ReferenceRunner.hpp"
#include "targets/RenderTargetKeys.hpp"

namespace RtEngine
{
    constexpr std::string SAMPLE_COUNT_OPTION_NAME = "Sample_Count";
    constexpr std::string REFERENCE_IMAGE_PATH_OPTION_NAME = "Reference_Image";

    BenchmarkRunner::BenchmarkRunner(const std::shared_ptr<EngineContext>& engine_context,
                                     const std::shared_ptr<SceneManager>& scene_manager)
        : Runner(engine_context, scene_manager)
    {
        if (std::filesystem::create_directories(TMP_FOLDER))
        {
            SPDLOG_INFO("Created directory {}");
        }

        if (std::filesystem::create_directories(OUT_FOLDER))
        {
            SPDLOG_INFO("Created directory {}");
        }
    }

    void BenchmarkRunner::loadScene(const std::string& scene_path)
    {
        Runner::loadScene(scene_path);

        scene_manager->getCurrentScene()->update();
        draw_context = createMainDrawContext();

        assert(draw_context->target_repositories.size() == 1);
        std::shared_ptr<RenderTargetRepository> target_repository = draw_context->target_repositories[0];
        if (biased_samples_per_diff_sample > 0)
        {
            // One biased sample per frame; the shader/DrawContext gate the diff sample
            // to fire only on frames where the biased count crosses a multiple of
            // biased_samples_per_diff_sample.
            target_repository->setSamplesPerFrame(1, 1);
            target_repository->setBiasedSamplesPerDiffSample(biased_samples_per_diff_sample);
        }
        else
        {
            target_repository->setSamplesPerFrame(1, 0);
            target_repository->setBiasedSamplesPerDiffSample(0);
        }
    }

    void BenchmarkRunner::renderScene()
    {
        if (update_flags->checkFlag(SCENE_UPDATE))
        {
            loadScene(scene_manager->getScenePath(scene_name));
        }
        std::shared_ptr<RenderTargetRepository> target_repository = draw_context->target_repositories[0];

        std::string target_to_output = raytracing_renderer->getMlmcPresentMode() == UNBIASED
                                           ? MAIN_TARGET_KEY
                                           : MLMC_TARGET_KEY;

        if (biased_samples_per_diff_sample == 0)
        {
            if (calculating_mlmc_diff)
            {
                if (error_calculation_frame_count == rendered_frame_count)
                {
                    recordCheckpointTime(final_biased_sample_count + rendered_frame_count);
                    raytracing_renderer->waitForIdle();
                    raytracing_renderer->outputRenderingTarget(target_repository,
                                                               target_to_output, getTmpImagePath(
                                                                   final_biased_sample_count, rendered_frame_count));
                    last_time_point = clock::now();

                    if (rendered_frame_count == final_diff_sample_count)
                    {
                        SPDLOG_INFO("Average biased frame time: {} ms, average diff frame time: {} ms",
                                    mean_biased_frame_time / 1000.0f, mean_diff_frame_time / 1000.0f);
                        finishRound();
                    }
                    else
                    {
                        error_calculation_frame_count *= 2;
                    }
                }
            }
            else
            {
                if (error_calculation_frame_count == rendered_frame_count)
                {
                    recordCheckpointTime(rendered_frame_count);
                    raytracing_renderer->waitForIdle();
                    raytracing_renderer->outputRenderingTarget(target_repository,
                                                               target_to_output,
                                                               getTmpImagePath(rendered_frame_count, 0));
                    last_time_point = clock::now();

                    if (rendered_frame_count == final_biased_sample_count)
                    {
                        if (final_diff_sample_count == 0)
                        {
                            finishRound();
                        }
                        else
                        {
                            error_calculation_frame_count = 1;
                            rendered_frame_count = 0;
                            calculating_mlmc_diff = true;
                            target_repository->setSamplesPerFrame(0, 1);
                        }
                    }
                    else
                    {
                        error_calculation_frame_count *= 2;
                    }
                }
            }
        }
        else
        {
            if (error_calculation_frame_count == rendered_frame_count)
            {
                // rendered_frame_count == biased sample count (one biased sample per frame).
                // A diff sample leads each ratio block, so at biased count k we've taken
                // ceil(k / biased_samples_per_diff_sample) diff samples.
                uint32_t diff_count =
                    (rendered_frame_count + biased_samples_per_diff_sample - 1) / biased_samples_per_diff_sample;
                recordCheckpointTime(rendered_frame_count + diff_count);
                raytracing_renderer->waitForIdle();
                raytracing_renderer->outputRenderingTarget(target_repository,
                                                           target_to_output,
                                                           getTmpImagePath(rendered_frame_count, diff_count));
                last_time_point = clock::now();

                if (rendered_frame_count == final_biased_sample_count)
                {
                    SPDLOG_INFO("Average combined frame time: {} ms",
                                mean_combined_frame_time / 1000.0f);
                    finishRound();
                }
                else
                {
                    error_calculation_frame_count *= 2;
                }
            }
        }


        drawFrame(draw_context);

        rendered_frame_count++;
    }

    void BenchmarkRunner::finishRound()
    {
        std::shared_ptr<RenderTargetRepository> target_repository = draw_context->target_repositories[0];

        calculateErrors();
        done_rounds++;

        if (done_rounds == averaging_rounds)
        {
            running = false;
            outputErrorsToCsv();
        }
        else
        {
            resetForNextRound(target_repository);
            SPDLOG_INFO("------------------------------------------------------------------------------");
            SPDLOG_INFO("{} / {} rounds finished!", done_rounds, averaging_rounds);
            SPDLOG_INFO("------------------------------------------------------------------------------");
        }
    }

    void BenchmarkRunner::drawFrame(const std::shared_ptr<DrawContext>& draw_context)
    {
        raytracing_renderer->waitForNextFrameStart();

        if (last_time_point.has_value())
        {
            const auto dur = duration_cast<std::chrono::microseconds>(clock::now() - last_time_point.value()).count();
            accumulated_frame_time_us += static_cast<double>(dur);
            if (biased_samples_per_diff_sample != 0)
            {
                mean_combined_frame_time += dur / final_biased_sample_count;
            }
            else
            {
                if (calculating_mlmc_diff && rendered_frame_count > 0)
                {
                    mean_diff_frame_time += dur / final_diff_sample_count;
                }
                else if (rendered_frame_count > 0)
                {
                    mean_biased_frame_time += dur / final_biased_sample_count;
                }
            }
        }
        last_time_point = clock::now();

        VkCommandBuffer cmd = raytracing_renderer->getNewCommandBuffer();
        std::shared_ptr<RenderTargetRepository> target_repository = draw_context->target_repositories[0];

        bool present_image = error_calculation_frame_count - 1 == rendered_frame_count;

        int32_t swapchain_image_idx = 0;
        if (present_image)
        {
            swapchain_image_idx = raytracing_renderer->aquireNextSwapchainImage();
            if (swapchain_image_idx < 0)
            {
                handle_resize();
                return;
            }
        }

        prepareFrame(cmd, draw_context);

        raytracing_renderer->writeRenderTarget(target_repository);
        raytracing_renderer->recordCommandBuffer(cmd, target_repository, swapchain_image_idx, present_image);

        finishFrame(cmd, draw_context, static_cast<uint32_t>(swapchain_image_idx), present_image);
    }

    void BenchmarkRunner::prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext>& draw_context)
    {
        raytracing_renderer->writeResources(draw_context, update_flags);
        engine_context->rendering_manager->recordBeginCommandBuffer(cmd);
        update_flags->resetFlags();
    }

    void BenchmarkRunner::recordCheckpointTime(uint32_t samples)
    {
        if (last_time_point.has_value())
        {
            const auto now = clock::now();
            accumulated_frame_time_us += static_cast<double>(
                duration_cast<std::chrono::microseconds>(now - last_time_point.value()).count());
            last_time_point = now;
        }
        time_averages[samples] += accumulated_frame_time_us / static_cast<double>(averaging_rounds);
    }

    void BenchmarkRunner::resetForNextRound(std::shared_ptr<RenderTargetRepository> target_repository)
    {
        rendered_frame_count = 0;
        error_calculation_frame_count = 1;
        calculating_mlmc_diff = false;
        accumulated_frame_time_us = 0;
        last_time_point.reset();
        if (biased_samples_per_diff_sample == 0)
        {
            target_repository->setSamplesPerFrame(1, 0);
            target_repository->setBiasedSamplesPerDiffSample(0);
        }
        else
        {
            target_repository->setSamplesPerFrame(1, 1);
            target_repository->setBiasedSamplesPerDiffSample(biased_samples_per_diff_sample);
        }
        target_repository->resetAccumulatedFrames();
    }

    std::string BenchmarkRunner::getTmpImagePath(uint32_t biased_samples, uint32_t diff_samples)
    {
        std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
        return std::format("{}/bm_{}_{}_{}.png", TMP_FOLDER, biased_samples, diff_samples, scene_name);
    }

    std::string BenchmarkRunner::getOutputFilePath()
    {
        std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
        return std::format("{}/{}_{}_bm_out.csv", OUT_FOLDER, scene_name, benchmark_name);
    }

    std::string BenchmarkRunner::getRefFilePath()
    {
        std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
        return std::format("{}/{}_{}.png", REF_FOLDER, expected_ref_sample_count, scene_name);
    }

    void BenchmarkRunner::calculateErrorBetweenImages(uint8_t* ref_data, uint32_t ref_width, uint32_t ref_height,
                                                      uint8_t* data, uint32_t width, uint32_t height,
                                                      uint32_t samples)
    {
        if (ref_width != width || ref_height != height)
        {
            SPDLOG_DEBUG("Ref size {} {} image size {} {} samples {}", ref_width, ref_height, width, height, samples);
            stbi_image_free(data);
            return;
        }
        assert(data != nullptr);

        float mse = calculateMSE(ref_data, data, width * height * 4);
        if (!mse_averages.contains(samples))
            mse_averages[samples] = 0;
        mse_averages[samples] += mse / static_cast<float>(averaging_rounds);

        stbi_image_free(data);
    }

    void BenchmarkRunner::calculateErrors()
    {
        std::string ref_path = getRefFilePath();

        int ref_width, ref_height;
        uint8_t* ref_data = ImageUtil::loadPNG(ref_path, &ref_width, &ref_height);

        assert(ref_data != nullptr);

        if (biased_samples_per_diff_sample == 0)
        {
            for (uint32_t i = 1; i <= final_biased_sample_count; i *= 2)
            {
                int width, height;
                uint8_t* data = ImageUtil::loadPNG(getTmpImagePath(i, 0), &width, &height);
                calculateErrorBetweenImages(ref_data, ref_width, ref_height, data, width, height, i);
            }

            for (uint32_t i = 1; i <= final_diff_sample_count; i *= 2)
            {
                int width, height;
                uint8_t* data = ImageUtil::loadPNG(getTmpImagePath(final_biased_sample_count, i), &width, &height);
                calculateErrorBetweenImages(ref_data, ref_width, ref_height, data, width, height,
                                            final_biased_sample_count + i);
            }
        }
        else
        {
            // Same exponential schedule as the two-phase mode. The image at biased
            // count i was captured after i biased and ceil(i / biased_samples_per_diff_sample)
            // diff samples (a diff sample leads each block); the CSV x-axis is their sum.
            for (uint32_t i = 1; i <= final_biased_sample_count; i *= 2)
            {
                uint32_t diff_count = (i + biased_samples_per_diff_sample - 1) / biased_samples_per_diff_sample;
                int width, height;
                uint8_t* data = ImageUtil::loadPNG(getTmpImagePath(i, diff_count), &width, &height);
                calculateErrorBetweenImages(ref_data, ref_width, ref_height, data, width, height,
                                            i + diff_count);
            }
        }
        stbi_image_free(ref_data);

        // clearTmpfolder();
    }

    void BenchmarkRunner::outputErrorsToCsv()
    {
        std::string output_path = getOutputFilePath();
        std::ofstream out(output_path);
        if (!out)
            throw std::runtime_error("Failed to open CSV file");
        out << "name,samples,mse,time_ms\n";

        std::vector<uint32_t> keys;
        keys.reserve(mse_averages.size());

        for (const auto& key : mse_averages | std::views::keys)
        {
            keys.push_back(key);
        }

        std::sort(keys.begin(), keys.end());

        int hacky_skip_count = 2;
        for (const auto& key : keys)
        {
            if (hacky_skip_count > 0)
            {
                hacky_skip_count--;
                continue;
            }
            out << std::format("{},{},{},{}\n", benchmark_name, key, mse_averages[key],
                               time_averages[key] / 1000.0);
        }
        SPDLOG_INFO("Saved benchmark data to {}!", output_path);;
    }

    void BenchmarkRunner::clearTmpfolder()
    {
        namespace fs = std::filesystem;
        QuickTimer timer("MSE Calculation");

        if (!fs::exists(TMP_FOLDER) || !fs::is_directory(TMP_FOLDER))
            return;

        fs::path keep_path;
        if (biased_samples_per_diff_sample != 0)
        {
            uint32_t final_diff = (final_biased_sample_count + biased_samples_per_diff_sample - 1)
                                  / biased_samples_per_diff_sample;
            keep_path = fs::path(getTmpImagePath(final_biased_sample_count, final_diff)).lexically_normal();
        }
        else
        {
            keep_path = fs::path(getTmpImagePath(final_biased_sample_count, final_diff_sample_count)).
                lexically_normal();
        }

        SPDLOG_INFO("Keep image at path {}", keep_path.string());

        for (const fs::directory_entry& entry : fs::directory_iterator(TMP_FOLDER))
        {
            if (!entry.is_regular_file())
                continue;
            if (entry.path().lexically_normal() == keep_path)
                continue;
            fs::remove(entry.path());
        }
    }

    float BenchmarkRunner::calculateMSE(uint8_t* ref_data, uint8_t* data, uint32_t size)
    {
        float result = 0;
#pragma omp parallel for reduction(+:result)
        for (uint32_t i = 0; i < size; i++)
        {
            int32_t difference = static_cast<int32_t>(ref_data[i]) - static_cast<int32_t>(data[i]);
            result += static_cast<float>(difference * difference) / static_cast<float>(size);
        }

        return result;
    }

    void BenchmarkRunner::initProperties(const std::shared_ptr<IProperties>& config,
                                         const UpdateFlagsHandle& update_flags)
    {
        Runner::initProperties(config, update_flags);

        if (config->startChild("benchmark"))
        {
            config->addString("name", &benchmark_name);
            config->addUint("expected_ref_samples", &expected_ref_sample_count);
            config->addUint("biased_samples", &final_biased_sample_count);
            config->addUint("diff_samples", &final_diff_sample_count);
            config->addUint("biased_samples_per_diff_sample", &biased_samples_per_diff_sample);
            config->addUint("averaging_rounds", &averaging_rounds);
            config->endChild();
        }
    }
} // namespace RtEngine
