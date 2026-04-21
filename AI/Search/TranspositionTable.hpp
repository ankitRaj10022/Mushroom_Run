#pragma once

#include "AI/Search/SearchTypes.hpp"

#include <cstdint>
#include <unordered_map>

namespace Game {

enum class BoundType {
  Exact,
  Lower,
  Upper
};

struct TTEntry {
  std::uint64_t key{0};
  int depth{0};
  float score{0.0f};
  BoundType bound{BoundType::Exact};
  Action bestAction{};
};

class TranspositionTable {
 public:
  const TTEntry* probe(std::uint64_t key) const;
  void store(const TTEntry& entry);
  void clear();

 private:
  std::unordered_map<std::uint64_t, TTEntry> table_{};
};

}  // namespace Game
