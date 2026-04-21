#include "AI/ML/MLModel.hpp"

#include <array>
#include <filesystem>
#include <stdexcept>

namespace Game {

bool MLModel::load(const std::string& modelPath) {
  loaded_ = false;
  lastError_.clear();

#if TACTICAL_HAS_ONNX
  try {
    if (!std::filesystem::exists(modelPath)) {
      lastError_ = "Model file not found: " + modelPath;
      return false;
    }

    env_ = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "TacticalHybridAI");
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

#ifdef _WIN32
    const std::wstring widePath(modelPath.begin(), modelPath.end());
    session_ = std::make_unique<Ort::Session>(*env_, widePath.c_str(), options);
#else
    session_ = std::make_unique<Ort::Session>(*env_, modelPath.c_str(), options);
#endif

    Ort::AllocatorWithDefaultOptions allocator;
    inputName_ = session_->GetInputNameAllocated(0, allocator).get();
    outputName_ = session_->GetOutputNameAllocated(0, allocator).get();
    loaded_ = true;
  } catch (const std::exception& error) {
    lastError_ = error.what();
    loaded_ = false;
  }
#else
  (void)modelPath;
  lastError_ = "ONNX Runtime support is disabled at build time.";
#endif

  return loaded_;
}

bool MLModel::available() const {
  return loaded_;
}

float MLModel::predict(const std::vector<float>& features) const {
#if TACTICAL_HAS_ONNX
  if (!loaded_) {
    return 0.0f;
  }

  Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
  std::array<int64_t, 2> inputShape{1, static_cast<int64_t>(features.size())};
  Ort::Value inputTensor =
      Ort::Value::CreateTensor<float>(memoryInfo, const_cast<float*>(features.data()), features.size(), inputShape.data(),
                                      inputShape.size());

  const char* inputNames[] = {inputName_.c_str()};
  const char* outputNames[] = {outputName_.c_str()};
  auto outputValues = session_->Run(Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);
  const float* outputData = outputValues.front().GetTensorData<float>();
  return outputData[0];
#else
  (void)features;
  return 0.0f;
#endif
}

const std::string& MLModel::lastError() const {
  return lastError_;
}

}  // namespace Game
