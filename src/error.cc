#include "error.h"
#include <errno.h>

#include <absl/debugging/stacktrace.h>

// XXX
#include <absl/debugging/internal/elf_mem_image.h>
#ifdef ABSL_HAVE_ELF_MEM_IMAGE
#include <absl/debugging/internal/vdso_support.h>
#endif

constexpr int kStackMax = 32;

Error::Error(): m_present{false} {}

Error::Error(const string& error, int skip): m_present{true},
                                             m_stack(kStackMax, nullptr) {
  int stack_len;
  std::vector<int> sizes(kStackMax, 0);
  int depth = absl::GetStackFrames(m_stack.data(), sizes.data(), kStackMax, skip);
  m_stack.resize(depth);

  Extend(error);
}

Error& Error::Extend(const string& error) {
  m_trace.emplace_back(error);
  return *this;
}

void Error::Print() {
  assert(m_present);

  fmt::print("error: {}\n", trace(0));
  for (int i = 1; i < traces(); i++) {
    fmt::print("     - {}\n", trace(i));
  }

  if (!m_stack.empty()) {
    #ifdef ABSL_HAVE_ELF_MEM_IMAGE
    absl::debugging_internal::VDSOSupport vdso;
    absl::debugging_internal::ElfMemImage::SymbolInfo symbol;
    #endif

    fmt::print("stack trace:\n");
    for (auto ptr : m_stack) {
      #ifdef ABSL_HAVE_ELF_MEM_IMAGE
      if (vdso.IsPresent() && vdso.LookupSymbolByAddress(ptr, &symbol)) {
        fmt::print("  {} [{}]\n", ptr, symbol.name);
      } else {
      #endif
        fmt::print("  {} [unknown]\n", ptr);
      #ifdef ABSL_HAVE_ELF_MEM_IMAGE
      }
      #endif
    }
  }
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
Expect<T>::Expect(const T& value): m_value{value} {}

template <typename T>
Expect<T>::Expect(const ::Error& error): m_value{error} {}

template <typename T>
Expect<T>::operator bool() {
  assert(m_value.has_value());
  return absl::any_cast<::Error>(&m_value) == nullptr;
}

template <typename T>
Error Expect<T>::Error() {
  ::Error* ptr = absl::any_cast<::Error>(&m_value);
  if (ptr == nullptr) {
    return Error::New();
  }

  ::Error res{std::move(*ptr)};
  m_value.reset();
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

// XXX
template class Expect<string>;
