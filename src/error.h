#pragma once

#include "uterm.h"

#include <absl/types/any.h>

#include <errno.h>

class Error {
public:
  Error();
  Error(const char* str): Error{string{str}} {}
  Error(const string& error);
  ~Error()=default;

  static Error New() { return std::move(Error{}); }
  template <typename Arg>
  static Error New(Arg&& arg) { return std::move(Error{arg}); }
  static Error Errno() { return Error::New(strerror(errno)); }

  Error& Extend(const string& error);

  size_t traces();
  string& trace(size_t i);

  operator bool();
private:
  template <typename T>
  friend class Expect;

  bool m_present;
  std::vector<string> m_trace;
};

template <typename T>
class Expect {
public:
  Expect(const T& value);
  Expect(const Error& error);

  template <typename Arg>
  static Expect<T> New(Arg&& arg) { return std::move(Expect<T>{arg}); }

  static Expect WithError(const char* error) { return New(::Error{error}); }
  static Expect WithError(string&& error) { return New(::Error{std::move(error)}); }

  operator bool();
  Error Error();

  T& operator *();
  T* operator ->();
private:
  absl::any m_value;
};
