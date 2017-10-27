#include "error.h"
#include <errno.h>

Error::Error(): m_present{false} {}

Error::Error(string&& error): m_present{true} {
  Extend(std::move(error));
}

/* Error::Error(Error&& other) { *this = std::move(other); } */

/* Error& Error::operator=(Error&& other) { */
/*   m_present = other.m_present; */
/*   m_copyable = other.m_copyable; */
/*   m_trace = std::move(other.m_trace); */
/*   return *this; */
/* } */

void Error::Extend(string&& error) {
  m_trace.emplace_back(std::move(error));
}

size_t Error::traces() {
  assert(m_present);
  return m_trace.size();
}

string& Error::trace(size_t i) {
  assert(m_present && i >= 0 && i < m_trace.size());
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
Error Expect<T>::Error() {
  ::Error* ptr = absl::any_cast<::Error>(&m_value);
  assert(ptr != nullptr);
  ::Error&& res = std::move(*ptr);

  m_value.reset();

  res.m_copyable = false;
  return res;
}

template <typename T>
T& Expect<T>::operator *() {
  T* ptr = absl::any_cast<T>(&m_value);
  assert(ptr != nullptr);
  return *ptr;
}

template <typename T>
T* Expect<T>::operator ->() { return &**this; }
