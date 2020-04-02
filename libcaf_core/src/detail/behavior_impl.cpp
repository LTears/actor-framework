/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright 2011-2018 Dominik Charousset                                     *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#include "caf/detail/behavior_impl.hpp"

#include <utility>

#include "caf/message_handler.hpp"

namespace caf::detail {

namespace {

class combinator final : public behavior_impl {
public:
  match_result invoke(detail::invoke_result_visitor& f, message& xs) override {
    auto x = first->invoke(f, xs);
    return x == match_result::no_match ? second->invoke(f, xs) : x;
  }

  void handle_timeout() override {
    // the second behavior overrides the timeout handling of
    // first behavior
    return second->handle_timeout();
  }

  combinator(pointer p0, const pointer& p1)
    : behavior_impl(p1->timeout()), first(std::move(p0)), second(p1) {
    // nop
  }

private:
  pointer first;
  pointer second;
};

class maybe_message_visitor : public detail::invoke_result_visitor {
public:
  optional<message> value;

  void operator()(error& x) override {
    value = make_message(std::move(x));
  }

  void operator()(message& x) override {
    value = std::move(x);
  }
};

} // namespace

behavior_impl::~behavior_impl() {
  // nop
}

behavior_impl::behavior_impl() : timeout_(infinite) {
  // nop
}

behavior_impl::behavior_impl(timespan tout) : timeout_(tout) {
  // nop
}

match_result behavior_impl::invoke_empty(detail::invoke_result_visitor& f) {
  message xs;
  return invoke(f, xs);
}

optional<message> behavior_impl::invoke(message& xs) {
  maybe_message_visitor f;
  // the following const-cast is safe, because invoke() is aware of
  // copy-on-write and does not modify x if it's shared
  if (!xs.empty())
    invoke(f, xs);
  else
    invoke_empty(f);
  return std::move(f.value);
}

match_result behavior_impl::invoke(detail::invoke_result_visitor& f,
                                   message& xs) {
  if (!xs.empty())
    return invoke(f, xs);
  return invoke_empty(f);
}

void behavior_impl::handle_timeout() {
  // nop
}

behavior_impl::pointer behavior_impl::or_else(const pointer& other) {
  CAF_ASSERT(other != nullptr);
  return make_counted<combinator>(this, other);
}

} // namespace caf::detail
