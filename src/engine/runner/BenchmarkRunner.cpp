#include "BenchmarkRunner.hpp"

#include <filesystem>
#include <omp.h>
#include <ranges>

#include "ImageUtil.hpp"
#include "PathUtil.hpp"
#include "ReferenceRunner.hpp"
#include "targets/RenderTargetKeys.hpp"

namespace RtEngine {
    constexpr std::string SAMPLE_COUNT_OPTION_NAME = "Sample_Count";
    constexpr std::string REFERENCE_IMAGE_PATH_OPTION_NAME = "Reference_Image";

    BenchmarkRunner::BenchmarkRunner(const std::shared_ptr<EngineContext> &engine_context,
                                     const std::shared_ptr<SceneManager> &scene_manager)
        : Runner(engine_context, scene_manager) {
        if (std::filesystem::create_directories(TMP_FOLDER)) {
            SPDLOG_INFO("Created directory {}");
        }

        if (std::filesystem::create_directories(OUT_FOLDER)) {
            SPDLOG_INFO("Created directory {}");
        }
    }

    void BenchmarkRunner::loadScene(const std::string &scene_path) {
        Runner::loadScene(scene_path);

        scene_manager->getCurrentScene()->update();
        draw_context = createMainDrawContext();

        assert(draw_context->target_repositories.size() == 1);
        std::shared_ptr<RenderTargetRepository> target_repository = draw_context->target_repositories[0];
        target_repository->setSamplesPerFrame(1, 0);
    }

    void BenchmarkRunner::renderScene() {
        if (update_flags->checkFlag(SCENE_UPDATE)) {
            loadScene(scene_manager->getScenePath(scene_name));
        }
        std::shared_ptr<RenderTargetRepository> target_repository = draw_context->target_repositories[0];

        std::string target_to_output = raytracing_renderer->getMlmcPresentMode() == UNBIASED
                                           ? MAIN_TARGET_KEY
                                           : MLMC_TARGET_KEY;

        if (calculating_mlmc_diff) {
            if (error_calculation_frame_count == diff_sample_count) {
                raytracing_renderer->waitForIdle();

                raytracing_renderer->outputRenderingTarget(target_repository,
                                                           target_to_output, getTmpImagePath(
                                                               final_biased_sample_count, diff_sample_count));

                if (diff_sample_count == final_diff_sample_count) {
                    finishRound();
                } else {
                    error_calculation_frame_count *= 2;
                }
            }
        } else {
            if (error_calculation_frame_count == biased_sample_count) {
                raytracing_renderer->waitForIdle();

                raytracing_renderer->outputRenderingTarget(target_repository,
                                                           target_to_output, getTmpImagePath(biased_sample_count, 0));

                if (biased_sample_count == final_biased_sample_count) {
                    if (final_diff_sample_count == 0) {
                        finishRound();
                    } else {
                        error_calculation_frame_count = 1;
                        calculating_mlmc_diff = true;
                        target_repository->setSamplesPerFrame(0, 1);
                    }
                } else {
                    error_calculation_frame_count *= 2;
                }
            }
        }

        drawFrame(draw_context);

        if (calculating_mlmc_diff) {
            diff_sample_count++;
        } else {
            biased_sample_count++;
        }
    }

    void BenchmarkRunner::finishRound() {
        std::shared_ptr<RenderTargetRepository> target_repository = draw_context->target_repositories[0];

        calculateErrors();
        done_rounds++;

        if (done_rounds == averaging_rounds) {
            running = false;
            outputErrorsToCsv();
        } else {
            resetForNextRound(target_repository);
            SPDLOG_INFO("------------------------------------------------------------------------------");
            SPDLOG_INFO("{} / {} rounds finished!", done_rounds, averaging_rounds);
            SPDLOG_INFO("------------------------------------------------------------------------------");
        }
    }

    void BenchmarkRunner::drawFrame(const std::shared_ptr<DrawContext> &draw_context) {
        raytracing_renderer->waitForNextFrameStart();

        VkCommandBuffer cmd = raytracing_renderer->getNewCommandBuffer();
        std::shared_ptr<RenderTargetRepository> target_repository = draw_context->target_repositories[0];

        bool present_image = calculating_mlmc_diff
                                 ? error_calculation_frame_count - 1 == diff_sample_count
                                 : error_calculation_frame_count - 1 == biased_sample_count;

        int32_t swapchain_image_idx = 0;
        if (present_image) {
            swapchain_image_idx = raytracing_renderer->aquireNextSwapchainImage();
            if (swapchain_image_idx < 0) {
                handle_resize();
                return;
            }
        }

        prepareFrame(cmd, draw_context);

        raytracing_renderer->writeRenderTarget(target_repository);
        raytracing_renderer->recordCommandBuffer(cmd, target_repository, swapchain_image_idx, present_image);

        finishFrame(cmd, draw_context, static_cast<uint32_t>(swapchain_image_idx), present_image);
    }

    void BenchmarkRunner::prepareFrame(VkCommandBuffer cmd, const std::shared_ptr<DrawContext> &draw_context) {
        raytracing_renderer->writeResources(draw_context, update_flags);
        engine_context->rendering_manager->recordBeginCommandBuffer(cmd);
        update_flags->resetFlags();
    }

    void BenchmarkRunner::resetForNextRound(std::shared_ptr<RenderTargetRepository> target_repository) {
        diff_sample_count = 0;
        biased_sample_count = 0;
        error_calculation_frame_count = 1;
        calculating_mlmc_diff = false;
        target_repository->setSamplesPerFrame(1, 0);
        target_repository->resetAccumulatedFrames();
    }

    std::string BenchmarkRunner::getTmpImagePath(uint32_t biased_samples, uint32_t diff_samples) {
        std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
        return std::format("{}/bm_{}_{}_{}.png", TMP_FOLDER, biased_samples, diff_samples, scene_name);
    }

    std::string BenchmarkRunner::getOutputFilePath() {
        std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
        return std::format("{}/{}_{}_bm_out.csv", OUT_FOLDER, scene_name, benchmark_name);
    }

    std::string BenchmarkRunner::getRefFilePath() {
        std::string scene_name = PathUtil::getFileName(scene_manager->getCurrentScene()->path);
        return std::format("{}/{}_{}.png", REF_FOLDER, expected_ref_sample_count, scene_name);
    }

    void BenchmarkRunner::calculateErrors() {
        std::string ref_path = getRefFilePath();

        int ref_width, ref_height;
        uint8_t *ref_data = ImageUtil::loadPNG(ref_path, &ref_width, &ref_height);

        assert(ref_data != nullptr);

        for (uint32_t i = 1; i <= final_biased_sample_count; i *= 2) {
            int width, height;
            uint8_t *data = ImageUtil::loadPNG(getTmpImagePath(i, 0), &width, &height);

            if (ref_width != width || ref_height != height) {
                SPDLOG_DEBUG("Ref size {} {} image size {} {} samples {}", ref_width, ref_height, width, height, i);

                stbi_image_free(data);
                continue;
            }
            assert(data != nullptr);

            float mse = calculateMSE(ref_data, data, width * height * 4);
            uint32_t samples = i;
            if (!mse_averages.contains(samples))
                mse_averages[samples] = 0;
            mse_averages[samples] += mse / static_cast<float>(averaging_rounds);

            stbi_image_free(data);
        }

        for (uint32_t i = 1; i <= final_diff_sample_count; i *= 2) {
            int width, height;
            uint8_t *data = ImageUtil::loadPNG(getTmpImagePath(final_biased_sample_count, i), &width, &height);

            if (ref_width != width || ref_height != height) {
                SPDLOG_DEBUG("Ref size {} {} image size {} {} samples {}", ref_width, ref_height, width, height, i);

                stbi_image_free(data);
                continue;
            }
            assert(data != nullptr);

            float mse = calculateMSE(ref_data, data, width * height * 4);
            uint32_t samples = final_biased_sample_count + i;
            if (!mse_averages.contains(samples))
                mse_averages[samples] = 0;
            mse_averages[samples] += mse / static_cast<float>(averaging_rounds);
            stbi_image_free(data);
        }
        stbi_image_free(ref_data);

        clearTmpfolder();
    }

    void BenchmarkRunner::outputErrorsToCsv() {
        std::string output_path = getOutputFilePath();
        std::ofstream out(output_path);
        if (!out)
            throw std::runtime_error("Failed to open CSV file");
        out << "name,samples,mse\n";

        std::vector<uint32_t> keys;
        keys.reserve(mse_averages.size());

        for (const auto &key: mse_averages | std::views::keys) {
            keys.push_back(key);
        }

        std::sort(keys.begin(), keys.end());

        int hacky_skip_count = 2;
        for (const auto &key: keys) {
            if (hacky_skip_count > 0)
            {
                hacky_skip_count--;
                continue;
            }
            out << std::format("{},{},{}\n", benchmark_name, key, mse_averages[key]);
        }
        SPDLOG_INFO("Saved benchmark data to {}!", output_path);;
    }

    void BenchmarkRunner::clearTmpfolder() {
        namespace fs = std::filesystem;
        QuickTimer timer("MSE Calculation");

        if (!fs::exists(TMP_FOLDER) || !fs::is_directory(TMP_FOLDER))
            return;

        for (const fs::directory_entry &entry: fs::directory_iterator(TMP_FOLDER)) {
            if (entry.is_regular_file()) {
                fs::remove(entry.path());
            }
        }
    }

    float BenchmarkRunner::calculateMSE(uint8_t *ref_data, uint8_t *data, uint32_t size) {
        float result = 0;
#pragma omp parallel for reduction(+:result)
        for (uint32_t i = 0; i < size; i++) {
            int32_t difference = static_cast<int32_t>(ref_data[i]) - static_cast<int32_t>(data[i]);
            result += static_cast<float>(difference * difference) / static_cast<float>(size);
        }

        return result;
    }

    void BenchmarkRunner::initProperties(const std::shared_ptr<IProperties> &config,
                                         const UpdateFlagsHandle &update_flags) {
        Runner::initProperties(config, update_flags);

        if (config->startChild("benchmark")) {
            config->addString("name", &benchmark_name);
            config->addUint("expected_ref_samples", &expected_ref_sample_count);
            config->addUint("biased_samples", &final_biased_sample_count);
            config->addUint("diff_samples", &final_diff_sample_count);
            config->addUint("averaging_rounds", &averaging_rounds);
            config->endChild();
        }
    }
} // namespace RtEngine
