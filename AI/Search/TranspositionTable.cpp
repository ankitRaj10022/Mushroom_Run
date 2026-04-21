#include "AI/Search/TranspositionTable.hpp"

namespace Game {

const TTEntry* TranspositionTable::probe(std::uint64_t key) const {
  const auto iterator = table_.find(key);
  return iterator == table_.end() ? nullptr : &iterator->second;
}

void TranspositionTable::store(const TTEntry& entry) {
  const auto iterator = table_.find(entry.key);
  if (iterator == table_.end() || iterator->second.depth <= entry.depth) {
    table_[entry.key] = entry;
  }
}

void TranspositionTable::clear() {
  table_.clear();
}

}  // namespace Game
