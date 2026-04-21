#pragma once

#include <memory>
#include <string>
#include <vector>

#if TACTICAL_HAS_ONNX
#include <onnxruntime_cxx_api.h>
#endif

namespace Game {

class MLModel {
 public:
  bool load(const std::string& modelPath);
  bool available() const;
  float predict(const std::vector<float>& features) const;
  const std::string& lastError() const;

 private:
  bool loaded_{false};
  std::string lastError_;

#if TACTICAL_HAS_ONNX
  std::unique_ptr<Ort::Env> env_;
  std::unique_ptr<Ort::Session> session_;
  std::string inputName_;
  std::string outputName_;
#endif
};

}  // namespace Game
