#pragma once

#include <unordered_set>
#include <algorithm>
#include <vector>

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

  void Resize(size_t sz);

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

  Marker m_default;
  std::unordered_set<Marker, MarkerHash> m_markers;
  std::vector<Marker*> m_indexes;
  ;
};

template <typename Data, typename Hash>
MarkerSet<Data, Hash>::MarkerSet(const Data &default_data): m_default{default_data} {
  m_default.refc = 1;
}

template <typename Data, typename Hash>
const Data & MarkerSet<Data, Hash>::At(size_t index) {
  return m_indexes[index]->data;
}

template <typename Data, typename Hash>
void MarkerSet<Data, Hash>::Update(size_t begin, size_t end, const Data& data) {
  assert(end <= m_indexes.size());

  auto it = m_markers.emplace(data).first;
  // XXX: we don't modify Marker's data, so it's hash should stay intact.
  auto m = const_cast<Marker*>(&*it);

  for (size_t i = begin; i < end; i++) {
    SetMarker(i, m);
  }
}

template <typename Data, typename Hash>
void MarkerSet<Data, Hash>::Update(size_t index, Data data) {
  Update(index, index + 1, data);
}

template <typename Data, typename Hash>
template <typename F>
void MarkerSet<Data, Hash>::UpdateWith(size_t begin, size_t end, F func) {
  assert(end <= m_indexes.size());

  for (size_t index = begin; index < end; index++) {
    Data data = m_indexes[index]->data;
    func(data);
    Update(index, data);
  }
}

template <typename Data, typename Hash>
template <typename F>
void MarkerSet<Data, Hash>::UpdateWith(size_t index, F func) {
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
      SetMarker(i, &m_default);
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
  assert(m_indexes[begin]);
  Data *data = &m_indexes[begin]->data;
  while (end < m_indexes.size() && m_indexes[end] == m_indexes[begin]) {
    end++;
  }

  return new Span{begin, end, *data};
}

template <typename Data, typename Hash>
void MarkerSet<Data, Hash>::SetMarker(int index, Marker *m) {
  auto old_marker = m_indexes[index];
  if (old_marker == m) {
    return;
  }

  m_indexes[index] = m;
  if (m != nullptr) {
    m->refc++;
  }

  if (old_marker != nullptr && --old_marker->refc == 0) {
    assert(old_marker != &m_default);
    m_markers.erase(*old_marker);
  }
}
