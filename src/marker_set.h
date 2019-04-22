#pragma once

#include <absl/algorithm/algorithm.h>
#include <absl/container/inlined_vector.h>
#include <absl/hash/hash.h>
#include <algorithm>
#include <unordered_set>
#include <vector>

#define PHMAP_USE_ABSL_HASHEQ

// XXX
namespace absl {
  template <typename T>
  struct EqualTo {
    bool operator()(const T& a, const T& b) const {
      return a == b;
    }
  };
}

#include <parallel_hashmap/phmap.h>

template <typename Data, typename Hash = std::hash<Data>>
class MarkerSet {
public:
  MarkerSet(const Data &default_data);

  struct Span {
    size_t begin, end;
    const Data &data;
  };

  const Data & At(size_t index);

  void Update(size_t begin, size_t end, const Data &data);
  void Update(size_t index, Data data);

  template <typename F>
  void UpdateWith(size_t begin, size_t end, F func);
  template <typename F>
  void UpdateWith(size_t index, F func);

  size_t size() { return m_indexes.size(); }
  void Resize(size_t sz);

  Span * NextSpan(Span *prev);
private:
  struct Marker {
    int *refc;
    Data *data;

    Marker(std::nullptr_t=nullptr): data{nullptr}, refc{nullptr} {}
    Marker(const Data &d): data{new Data{d}}, refc{new int{1}} {}

    Marker(const Marker &rhs) { *this = rhs; }
    Marker(Marker &&rhs) { *this = rhs; }

    Marker &operator=(const Marker &rhs) {
      data = rhs.data;
      refc = rhs.refc;

      if (refc != nullptr) {
        ++*refc;
      }

      return *this;
    }

    Marker &operator=(Marker &&rhs) {
      data = rhs.data;
      return *this;
    }

    ~Marker() {
      if (--refc == 0) {
        delete data;
      }
    }

    bool operator==(const Marker &rhs) const {
      return data == rhs.data || (data != nullptr && rhs.data != nullptr && *data == *rhs.data);
    }
  };

  template <typename H>
  friend H AbslHashValue(H h, const Marker& marker) {
    return H::combine(std::move(h), *marker.data);
  }
  void SetMarker(int index, const Marker &m);

  Marker m_default;
  phmap::parallel_flat_hash_set<Marker> m_markers;
  absl::InlinedVector<Marker, 4096> m_indexes;
};

template <typename Data, typename Hash>
MarkerSet<Data, Hash>::MarkerSet(const Data &default_data):
  m_default{default_data} {}

template <typename Data, typename Hash>
const Data & MarkerSet<Data, Hash>::At(size_t index) {
  return *m_indexes[index].data;
}

template <typename Data, typename Hash>
void MarkerSet<Data, Hash>::Update(size_t begin, size_t end, const Data& data) {
  assert(end <= m_indexes.size());

  const Marker &m = *m_markers.emplace(data).first;

  for (size_t i = begin; i < end; i++) {
    SetMarker(i, m);
  }
}

template <typename Data, typename Hash>
void MarkerSet<Data, Hash>::Update(size_t index, Data data) {
  if (index + 1 > m_indexes.size()) {
    return;
  }
  Update(index, index + 1, data);
}

template <typename Data, typename Hash>
template <typename F>
void MarkerSet<Data, Hash>::UpdateWith(size_t begin, size_t end, F func) {
  assert(end <= m_indexes.size());

  for (size_t index = begin; index < end; index++) {
    Data data = *m_indexes[index].data;
    func(data);
    Update(index, data);
  }
}

template <typename Data, typename Hash>
template <typename F>
void MarkerSet<Data, Hash>::UpdateWith(size_t index, F func) {
  if (m_indexes.size() == 0) {
    return;
  }
  UpdateWith(index, index + 1, func);
}

template <typename Data, typename Hash>
void MarkerSet<Data, Hash>::Resize(size_t sz) {
  if (m_indexes.size() > sz) {
    for (int i = sz; i < m_indexes.size(); i++) {
      SetMarker(i, nullptr);
    }

    m_indexes.resize(sz);
  } else if (m_indexes.size() < sz) {
    size_t first_sz = m_indexes.size();
    m_indexes.resize(sz, nullptr);

    for (int i = first_sz; i < sz; i++) {
      SetMarker(i, m_default);
    }
  }
}

template <typename Data, typename Hash>
typename MarkerSet<Data, Hash>::Span * MarkerSet<Data, Hash>::NextSpan(
          MarkerSet<Data, Hash>::Span *prev) {
  size_t begin = prev == nullptr ? 0 : prev->end;
  delete prev;

  if (begin >= m_indexes.size()) {
    return nullptr;
  }

  size_t end = begin+1;
  assert(m_indexes[begin].data);
  const Data &data = *m_indexes[begin].data;
  while (end < m_indexes.size() && m_indexes[end] == m_indexes[begin]) {
    end++;
  }

  return new Span{begin, end, data};
}

template <typename Data, typename Hash>
void MarkerSet<Data, Hash>::SetMarker(int index, const Marker& m) {
  auto old_marker = m_indexes[index];
  if (old_marker == m) {
    return;
  }

  m_indexes[index] = m;

  if (old_marker.data != nullptr && *old_marker.refc == 2) {
    m_markers.erase(old_marker);
  }
}
