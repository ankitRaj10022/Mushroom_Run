# Model Output

Run `python ML/training/train_mlp.py` to generate:

- `ML/model/tactical_mlp.onnx`
- `ML/model/tactical_mlp_meta.json`

The C++ runtime loads `ML/model/tactical_mlp.onnx` automatically when ONNX Runtime is available.
