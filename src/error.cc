#include "error.h"
#include <iostream>

Error::Error(): m_present{false} {}

Error::Error(string&& error): m_present{true} {
  Extend(std::move(error));
}

void Error::Extend(string&& error) {
  m_trace.push_back(error);
}

size_t Error::traces() {
  assert(m_present);
  return m_trace.size();
}

string& Error::trace(size_t i) {
  assert(m_present && i > 0 && i < m_trace.size());
  return m_trace[i];
}

Error::operator bool() { return m_present; }

template <typename T>
Expect<T>::Expect(T&& value): m_value{value} {}

template <typename T>
Expect<T>::Expect(::Error&& error) {
  error.m_copyable = true;
  m_value = error;
}

template <typename T>
Expect<T>::operator bool() {
  assert(m_value);
  return absl::any_cast<::Error>(&m_value) != nullptr;
}

template <typename T>
Error&& Expect<T>::Error() {
  ::Error* ptr = absl::any_cast<::Error>(&m_value);
  assert(ptr != nullptr);
  ::Error&& res = std::move(*ptr);

  m_value.reset();

  res.m_copyable = false;
  return std::move(res);
}

template <typename T>
T& Expect<T>::operator *() {
  T* ptr = absl::any_cast<T>(&m_value);
  assert(ptr != nullptr);
  return *ptr;
}

template <typename T>
T* Expect<T>::operator ->() { return &**this; }
