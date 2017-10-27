#pragma once

#include "uterm.h"

#include <absl/container/inlined_vector.h>
#include <absl/types/any.h>


class Error {
public:
  Error();
  Error(const char* str): Error{string{str}} {}
  Error(string&& error);

  static Error&& New() { return std::move(Error{}); }
  template <typename Arg>
  static Error&& New(Arg&& arg) { return std::move(Error{arg}); }

  void Extend(string&& error);

  size_t traces();
  string& trace(size_t i);

  operator bool();
private:
  template <typename T>
  friend class Expect;

  bool m_present, m_copyable{false};
  absl::InlinedVector<string, 8> m_trace;
};

template <typename T>
class Expect {
public:
  Expect(T&& value);
  Expect(Error&& error);

  template <typename Arg>
  static Expect<T>&& New(Arg&& arg) { return std::move(Expect<T>{arg}); }

  static Expect&& WithError(const char* error) { return New(::Error{error}); }
  static Expect&& WithError(string&& error) { return New(::Error{std::move(error)}); }

  operator bool();
  Error&& Error();

  T& operator *();
  T* operator ->();
private:
  absl::any m_value;
};
