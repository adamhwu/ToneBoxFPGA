// Independent streaming A1 WaveNet inference. Audio processing has no Python,
// PyTorch, or NAM runtime dependency. JSON parsing uses the bundled MIT-licensed
// nlohmann header; the network and its state handling are implemented here.
#include "vendor/json.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using Json = nlohmann::json;

namespace {

thread_local std::string last_error;

std::string lower(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}

void require(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}

Json field(const Json& object, const char* key, Json fallback = nullptr) {
  return object.contains(key) ? object.at(key) : std::move(fallback);
}

std::vector<Json> per_layer(const Json& value, std::size_t count,
                            const std::string& label) {
  if (value.is_array()) {
    require(value.size() == count, label + " length does not match dilations");
    return value.get<std::vector<Json>>();
  }
  return std::vector<Json>(count, value);
}

std::pair<bool, int> option(const Json& config, const char* key,
                            bool default_active) {
  const Json value = field(config, key);
  if (value.is_null()) return {default_active, 1};
  if (value.is_boolean()) return {value.get<bool>(), 1};
  return {value.value("active", default_active), value.value("groups", 1)};
}

struct Activation {
  std::string name;
  float slope = 0.01f;

  explicit Activation(const Json& spec) {
    if (spec.is_object()) {
      name = lower(spec.value("name", spec.value("type", std::string(""))));
      slope = spec.value("negative_slope", 0.01f);
    } else {
      name = lower(spec.get<std::string>());
    }
    require(name == "tanh" || name == "relu" || name == "sigmoid" ||
                name == "softsign" || name == "leakyrelu" ||
                name == "identity" || name == "linear",
            "unsupported activation: " + name);
  }

  float run(float x) const {
    if (name == "tanh") return std::tanh(x);
    if (name == "relu") return std::max(0.0f, x);
    if (name == "sigmoid") return 1.0f / (1.0f + std::exp(-x));
    if (name == "softsign") return x / (1.0f + std::abs(x));
    if (name == "leakyrelu") return x >= 0.0f ? x : x * slope;
    return x;
  }
};

struct WeightReader {
  const Json& weights;
  std::size_t offset = 0;

  explicit WeightReader(const Json& values) : weights(values) {
    require(weights.is_array(), ".nam weights must be an array");
  }

  void take(std::vector<float>& destination) {
    require(offset + destination.size() <= weights.size(),
            ".nam weights end before the configured architecture");
    for (float& value : destination) value = weights[offset++].get<float>();
  }

  float take_scalar() {
    require(offset < weights.size(), ".nam is missing head scale");
    return weights[offset++].get<float>();
  }

  void finish() const {
    require(offset == weights.size(), ".nam has unused weights; configuration mismatch");
  }
};

// PyTorch Conv1d weight order is [output][input within group][tap]. A ring
// stores the current sample plus all causal lookback for each input channel.
struct Conv {
  int inputs;
  int outputs;
  int kernel;
  int dilation;
  int groups;
  int ring_length;
  int cursor = 0;
  std::vector<float> weights;
  std::vector<float> bias;
  std::vector<float> ring;
  std::vector<float> result;
  std::vector<int> tap_indices;

  Conv(int in, int out, int width, int dil = 1, bool has_bias = true,
       int group_count = 1)
      : inputs(in), outputs(out), kernel(width), dilation(dil),
        groups(group_count), ring_length((width - 1) * dil + 1) {
    require(in > 0 && out > 0 && width > 0 && dil > 0 && group_count > 0 &&
                in % group_count == 0 && out % group_count == 0,
            "invalid convolution shape or groups");
    weights.resize(static_cast<std::size_t>(out) * (in / groups) * width);
    if (has_bias) bias.resize(out);
    ring.resize(static_cast<std::size_t>(in) * ring_length);
    result.resize(out);
    tap_indices.resize(width);
  }

  void load(WeightReader& reader) {
    reader.take(weights);
    reader.take(bias);
  }

  void reset() {
    std::fill(ring.begin(), ring.end(), 0.0f);
    cursor = 0;
  }

  const std::vector<float>& run(const float* input) {
    const int in_per_group = inputs / groups;
    const int out_per_group = outputs / groups;
    if (kernel == 1) {
      for (int out = 0; out < outputs; ++out) {
        float sum = bias.empty() ? 0.0f : bias[out];
        const int group_start = (out / out_per_group) * in_per_group;
        const float* taps = weights.data() + static_cast<std::size_t>(out) * in_per_group;
        for (int in = 0; in < in_per_group; ++in)
          sum += taps[in] * input[group_start + in];
        result[out] = sum;
      }
      return result;
    }
    for (int c = 0; c < inputs; ++c)
      ring[static_cast<std::size_t>(c) * ring_length + cursor] = input[c];
    for (int tap = 0; tap < kernel; ++tap) {
      int index = cursor - (kernel - 1 - tap) * dilation;
      if (index < 0) index += ring_length;
      tap_indices[tap] = index;
    }
    for (int out = 0; out < outputs; ++out) {
      float sum = bias.empty() ? 0.0f : bias[out];
      const int group_start = (out / out_per_group) * in_per_group;
      for (int in = 0; in < in_per_group; ++in) {
        const float* channel = ring.data() +
                               static_cast<std::size_t>(group_start + in) * ring_length;
        const float* taps = weights.data() +
                            (static_cast<std::size_t>(out) * in_per_group + in) * kernel;
        for (int tap = 0; tap < kernel; ++tap)
          sum += taps[tap] * channel[tap_indices[tap]];
      }
      result[out] = sum;
    }
    if (++cursor == ring_length) cursor = 0;
    return result;
  }
};

struct Layer {
  int bottleneck;
  int channels;
  std::string gating;
  Conv dilated;
  Conv mixin;
  std::unique_ptr<Conv> residual;
  std::unique_ptr<Conv> head_1x1;
  Activation activation;
  Activation secondary;
  std::vector<float> activated;

  Layer(int channel_count, int condition_count, int bottleneck_count,
        int kernel, int dilation, const Json& activation_spec,
        std::string gate, const Json& secondary_spec, int conv_groups,
        int mixin_groups, bool residual_active, int residual_groups,
        const Json& head_config)
      : bottleneck(bottleneck_count), channels(channel_count),
        gating(lower(std::move(gate))),
        dilated(channel_count, bottleneck_count * (gating == "none" ? 1 : 2),
                kernel, dilation, true, conv_groups),
        mixin(condition_count, bottleneck_count * (gating == "none" ? 1 : 2),
              1, 1, false, mixin_groups),
        activation(activation_spec), secondary(secondary_spec),
        activated(bottleneck_count) {
    require(gating == "none" || gating == "gated" || gating == "blended",
            "unsupported gating mode: " + gating);
    if (residual_active)
      residual = std::make_unique<Conv>(bottleneck, channels, 1, 1, true,
                                        residual_groups);
    else
      require(bottleneck == channels,
              "inactive residual projection needs bottleneck == channels");
    if (head_config.is_object() && head_config.value("active", false))
      head_1x1 = std::make_unique<Conv>(
          bottleneck, head_config.at("out_channels").get<int>(), 1, 1, true,
          head_config.value("groups", 1));
  }

  void load(WeightReader& reader) {
    dilated.load(reader);
    mixin.load(reader);
    if (residual) residual->load(reader);
    if (head_1x1) head_1x1->load(reader);
  }

  void reset() {
    dilated.reset();
    mixin.reset();
    if (residual) residual->reset();
    if (head_1x1) head_1x1->reset();
  }

  const std::vector<float>& run(std::vector<float>& hidden,
                                 const float* condition) {
    const auto& conv = dilated.run(hidden.data());
    const auto& conditioned = mixin.run(condition);
    for (int i = 0; i < bottleneck; ++i) {
      const float first = conv[i] + conditioned[i];
      const float a = activation.run(first);
      if (gating == "none") {
        activated[i] = a;
      } else {
        const float second = conv[i + bottleneck] + conditioned[i + bottleneck];
        const float b = secondary.run(second);
        activated[i] = gating == "gated" ? a * b : a * b + first * (1.0f - b);
      }
    }
    if (residual) {
      const auto& update = residual->run(activated.data());
      for (int i = 0; i < channels; ++i) hidden[i] += update[i];
    }
    return head_1x1 ? head_1x1->run(activated.data()) : activated;
  }
};

struct Stack {
  int input_size;
  int condition_size;
  int channels;
  int head_channels;
  int head_size;
  int receptive_field = 1;
  Conv rechannel;
  std::vector<Layer> layers;
  Conv head_rechannel;
  std::vector<float> hidden;
  std::vector<float> skip_sum;

  explicit Stack(const Json& config)
      : input_size(config.at("input_size").get<int>()),
        condition_size(config.at("condition_size").get<int>()),
        channels(config.at("channels").get<int>()),
        head_channels([&] {
          const Json h = field(config, "head1x1");
          return h.is_object() && h.value("active", false)
                     ? h.at("out_channels").get<int>()
                     : config.value("bottleneck", channels);
        }()),
        head_size([&] {
          const Json h = field(config, "head");
          return h.is_object() ? h.at("out_channels").get<int>()
                               : config.at("head_size").get<int>();
        }()),
        rechannel(input_size, channels, 1, 1, false),
        head_rechannel(head_channels, head_size,
                       [&] {
                         const Json h = field(config, "head");
                         return h.is_object() ? h.at("kernel_size").get<int>() : 1;
                       }(),
                       1,
                       [&] {
                         const Json h = field(config, "head");
                         return h.is_object() ? h.at("bias").get<bool>()
                                              : config.at("head_bias").get<bool>();
                       }()),
        hidden(channels), skip_sum(head_channels) {
    static const char* film_names[] = {
        "conv_pre_film", "conv_post_film", "input_mixin_pre_film",
        "input_mixin_post_film", "activation_pre_film", "activation_post_film",
        "layer1x1_post_film", "head1x1_post_film"};
    for (const char* name : film_names) {
      const Json value = field(config, name);
      require(value.is_null() || value == false ||
                  (value.is_object() && !value.value("active", true)),
              std::string("FiLM option is outside classic A1: ") + name);
    }

    const auto dilations = config.at("dilations").get<std::vector<int>>();
    require(!dilations.empty(), "A1 stack has no dilations");
    const auto kernels = per_layer(
        field(config, "kernel_sizes", field(config, "kernel_size")),
        dilations.size(), "kernel sizes");
    const auto activations =
        per_layer(config.at("activation"), dilations.size(), "activations");
    const auto gates = per_layer(
        field(config, "gating_mode",
              config.value("gated", false) ? Json("gated") : Json("none")),
        dilations.size(), "gating modes");
    const auto secondaries = per_layer(
        field(config, "secondary_activation", Json("Sigmoid")),
        dilations.size(), "secondary activations");
    const auto [residual_active, residual_groups] = option(config, "layer1x1", true);
    const Json head_config = field(config, "head1x1");
    const int bottleneck = config.value("bottleneck", channels);
    layers.reserve(dilations.size());
    for (std::size_t i = 0; i < dilations.size(); ++i) {
      const int kernel = kernels[i].get<int>();
      const int dilation = dilations[i];
      layers.emplace_back(channels, condition_size, bottleneck, kernel, dilation,
                          activations[i], gates[i].get<std::string>(), secondaries[i],
                          config.value("groups_input", 1),
                          config.value("groups_input_mixin", 1), residual_active,
                          residual_groups, head_config);
      receptive_field += (kernel - 1) * dilation;
    }
    receptive_field += head_rechannel.kernel - 1;
  }

  void load(WeightReader& reader) {
    rechannel.load(reader);
    for (auto& layer : layers) layer.load(reader);
    head_rechannel.load(reader);
  }

  void reset() {
    rechannel.reset();
    for (auto& layer : layers) layer.reset();
    head_rechannel.reset();
  }

  const std::vector<float>& run(const float* input, const float* condition,
                                 const float* previous_head) {
    const auto& projected = rechannel.run(input);
    std::copy(projected.begin(), projected.end(), hidden.begin());
    std::fill(skip_sum.begin(), skip_sum.end(), 0.0f);
    for (auto& layer : layers) {
      const auto& skip = layer.run(hidden, condition);
      for (int c = 0; c < head_channels; ++c) skip_sum[c] += skip[c];
    }
    if (previous_head)
      for (int c = 0; c < head_channels; ++c) skip_sum[c] += previous_head[c];
    return head_rechannel.run(skip_sum.data());
  }
};

struct PostLayer {
  Conv conv;
  Activation activation;
  std::vector<float> activated;

  PostLayer(int inputs, int outputs, int kernel, const Json& activation_spec)
      : conv(inputs, outputs, kernel), activation(activation_spec),
        activated(inputs) {}

  void load(WeightReader& reader) { conv.load(reader); }
  void reset() { conv.reset(); }
  const std::vector<float>& run(const float* input) {
    for (std::size_t c = 0; c < activated.size(); ++c)
      activated[c] = activation.run(input[c]);
    return conv.run(activated.data());
  }
};

struct Model {
  int sample_rate;
  int receptive_field = 1;
  int input_channels;
  int output_channels;
  float head_scale;
  float mix = 0.0f;
  std::vector<Stack> stacks;
  std::vector<PostLayer> post_head;
  std::vector<float> scaled_head;

  explicit Model(const char* path) : sample_rate(0), input_channels(0),
                                     output_channels(0), head_scale(1.0f) {
    std::ifstream file(path);
    require(file.good(), std::string("cannot open .nam file: ") + path);
    Json capture;
    file >> capture;
    require(capture.value("architecture", std::string("")) == "WaveNet",
            "expected an A1 WaveNet .nam capture");
    sample_rate = capture.value("sample_rate", 48000);
    const Json& config = capture.at("config");
    require(field(config, "condition_dsp").is_null(),
            "condition_dsp is outside classic A1");
    const Json& stack_config = config.at("layers");
    require(stack_config.is_array() && !stack_config.empty(),
            "expected WaveNet config.layers");
    stacks.reserve(stack_config.size());
    for (const auto& entry : stack_config) stacks.emplace_back(entry);
    input_channels = stacks.front().input_size;
    require(input_channels == stacks.front().condition_size,
            "first stack condition size must equal input size");
    for (std::size_t i = 1; i < stacks.size(); ++i) {
      require(stacks[i - 1].channels == stacks[i].input_size,
              "adjacent stacks have incompatible main channels");
      require(stacks[i - 1].head_size == stacks[i].head_channels,
              "adjacent stacks have incompatible head channels");
      require(stacks[i].condition_size == input_channels,
              "all stacks must condition on the original input");
    }
    for (const auto& stack : stacks)
      receptive_field += stack.receptive_field - 1;
    scaled_head.resize(stacks.back().head_size);

    const Json head = field(config, "head");
    if (head.is_object()) {
      const auto widths = head.at("kernel_sizes").get<std::vector<int>>();
      require(!widths.empty(), "post head must have at least one layer");
      int channels = stacks.back().head_size;
      for (std::size_t i = 0; i < widths.size(); ++i) {
        const int next = i + 1 == widths.size()
                             ? head.at("out_channels").get<int>()
                             : head.at("channels").get<int>();
        post_head.emplace_back(channels, next, widths[i], head.at("activation"));
        receptive_field += widths[i] - 1;
        channels = next;
      }
      output_channels = channels;
    } else {
      output_channels = stacks.back().head_size;
    }
    require(input_channels == 1 && output_channels == 1,
            "live A1 processor requires mono model input and output");

    WeightReader reader(capture.at("weights"));
    for (auto& stack : stacks) stack.load(reader);
    for (auto& layer : post_head) layer.load(reader);
    head_scale = reader.take_scalar();
    reader.finish();
    reset();
  }

  float run(float sample) {
    const float* main = &sample;
    const float* head = nullptr;
    for (auto& stack : stacks) {
      head = stack.run(main, &sample, head).data();
      main = stack.hidden.data();
    }
    for (std::size_t c = 0; c < scaled_head.size(); ++c)
      scaled_head[c] = head[c] * head_scale;
    head = scaled_head.data();
    for (auto& layer : post_head) head = layer.run(head).data();
    return head[0];
  }

  void reset() {
    for (auto& stack : stacks) stack.reset();
    for (auto& layer : post_head) layer.reset();
    mix = 0.0f;
    for (int i = 0; i < receptive_field - 1; ++i) run(0.0f);
  }

  void process_mono(const float* input, float* output, int frames) {
    require(input && output && frames >= 0, "invalid mono audio buffer");
    for (int i = 0; i < frames; ++i) output[i] = run(input[i]);
  }

  void process_audio(const float* input, float* output, int frames,
                     int input_count, int selected, int output_count,
                     bool enabled) {
    if (!output || frames < 0 || input_count <= 0 || output_count <= 0 ||
        selected < 0 || selected >= input_count)
      throw std::runtime_error("invalid live audio buffer or channel selection");
    if (!input) {
      std::fill(output, output + static_cast<std::size_t>(frames) * output_count,
                0.0f);
      return;
    }
    const float target = enabled ? 1.0f : 0.0f;
    const float start = mix;
    for (int i = 0; i < frames; ++i) {
      const float dry = input[static_cast<std::size_t>(i) * input_count + selected];
      const float wet = run(dry);
      const float alpha = frames == 1 ? 1.0f : static_cast<float>(i) / (frames - 1);
      const float blend = start + (target - start) * alpha;
      const float sample = std::clamp(dry * (1.0f - blend) + wet * blend,
                                      -1.0f, 1.0f);
      for (int channel = 0; channel < output_count; ++channel)
        output[static_cast<std::size_t>(i) * output_count + channel] = sample;
    }
    mix = target;
  }
};

}  // namespace

extern "C" {

const char* a1_last_error() { return last_error.c_str(); }

void* a1_create(const char* path) {
  try {
    last_error.clear();
    require(path != nullptr, "model path is null");
    return new Model(path);
  } catch (const std::exception& error) {
    last_error = error.what();
    return nullptr;
  }
}

void a1_destroy(void* model) { delete static_cast<Model*>(model); }

int a1_sample_rate(void* model) {
  return model ? static_cast<Model*>(model)->sample_rate : 0;
}

int a1_receptive_field(void* model) {
  return model ? static_cast<Model*>(model)->receptive_field : 0;
}

int a1_reset(void* model) {
  try {
    require(model != nullptr, "model is null");
    static_cast<Model*>(model)->reset();
    return 0;
  } catch (const std::exception& error) {
    last_error = error.what();
    return -1;
  }
}

int a1_process_mono(void* model, const float* input, float* output, int frames) {
  try {
    require(model != nullptr, "model is null");
    static_cast<Model*>(model)->process_mono(input, output, frames);
    return 0;
  } catch (const std::exception& error) {
    last_error = error.what();
    return -1;
  }
}

int a1_process_audio(void* model, const float* input, float* output,
                     int frames, int input_count, int selected,
                     int output_count, int enabled) {
  try {
    require(model != nullptr, "model is null");
    static_cast<Model*>(model)->process_audio(input, output, frames,
                                               input_count, selected,
                                               output_count, enabled != 0);
    return 0;
  } catch (const std::exception& error) {
    last_error = error.what();
    return -1;
  }
}

}  // extern "C"
