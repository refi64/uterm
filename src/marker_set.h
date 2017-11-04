#pragma once

#include <unordered_set>
#include <algorithm>
#include <vector>

template <typename Data, typename Hash = std::hash<Data>>
class MarkerSet {
public:
  struct Span {
    size_t begin, end;
    const Data &data;
  };

  void Update(size_t index, Data data);
  void Shrink(size_t sz);

  Span * NextSpan(Span *prev);
private:
  struct Marker {
    int refc{0};
    Data data;

    Marker(const Data &d): data{d} {}
    bool operator==(const Marker &rhs) const { return data == rhs.data; }
  };

  struct MarkerHash {
    Hash hash;

    size_t operator()(const Marker& m) const { return hash(m.data); }
  };

  void SetMarker(int index, Marker *m);

  std::unordered_set<Marker, MarkerHash> m_markers;
  std::vector<Marker*> m_indexes;
};


template <typename Data, typename Hash>
void MarkerSet<Data, Hash>::Update(size_t index, Data data) {
  if (index >= m_indexes.size()) {
    assert(m_indexes.size() == index);
    m_indexes.resize(index+1, nullptr);
  }

  auto it = m_markers.emplace(data).first;
  // XXX: we don't modify Marker's data, so it's hash should stay intact.
  auto m = const_cast<Marker*>(&*it);
  m->refc++;
  SetMarker(index, m);
}

template <typename Data, typename Hash>
void MarkerSet<Data, Hash>::Shrink(size_t sz) {
  if (m_indexes.size() > sz) {
    for (int i = sz; i < m_indexes.size(); i++) {
      SetMarker(i, nullptr);
    }

    m_indexes.resize(sz);
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
  Data *data = &m_indexes[begin]->data;
  while (end < m_indexes.size() && m_indexes[end] == m_indexes[begin]) {
    end++;
  }

  return new Span{begin, end, *data};
}

template <typename Data, typename Hash>
void MarkerSet<Data, Hash>::SetMarker(int index, Marker *m) {
  auto old_marker = m_indexes[index];
  m_indexes[index] = m;

  if (old_marker != nullptr && --old_marker->refc == 0) {
    m_markers.erase(*old_marker);
  }
}
