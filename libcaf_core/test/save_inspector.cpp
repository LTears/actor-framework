/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright 2011-2020 Dominik Charousset                                     *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#define CAF_SUITE save_inspector

#include "caf/save_inspector.hpp"

#include "caf/test/dsl.hpp"

#include <cstdint>
#include <string>
#include <vector>

#include "caf/message.hpp"
#include "caf/serializer.hpp"

#include "nasty.hpp"

namespace {

struct point_3d;
struct line;
struct duration;
struct person;
class foobar;
struct dummy_message;
struct basics;

} // namespace

#define CAF_TYPE_NAME(type)                                                    \
  namespace caf {                                                              \
  template <>                                                                  \
  struct type_name<type> {                                                     \
    static constexpr string_view value = #type;                                \
  };                                                                           \
  }

CAF_TYPE_NAME(point_3d)
CAF_TYPE_NAME(line)
CAF_TYPE_NAME(duration)
CAF_TYPE_NAME(person)
CAF_TYPE_NAME(foobar)
CAF_TYPE_NAME(dummy_message)
CAF_TYPE_NAME(basics)
CAF_TYPE_NAME(nasty)

using namespace caf;

namespace {

using string_list = std::vector<std::string>;

struct point_3d {
  int32_t x;
  int32_t y;
  int32_t z;
};

template <class Inspector>
bool inspect(Inspector& f, point_3d& x) {
  return f.object(x).fields(f.field("x", x.x), f.field("y", x.y),
                            f.field("z", x.z));
}

struct line {
  point_3d p1;
  point_3d p2;
};

template <class Inspector>
bool inspect(Inspector& f, line& x) {
  return f.object(x).fields(f.field("p1", x.p1), f.field("p2", x.p2));
}

struct duration {
  std::string unit;
  double count;
};

bool valid_time_unit(const std::string& unit) {
  return unit == "seconds" || unit == "minutes";
}

template <class Inspector>
bool inspect(Inspector& f, duration& x) {
  return f.object(x).fields(
    f.field("unit", x.unit).fallback("seconds").invariant(valid_time_unit),
    f.field("count", x.count));
}

struct person {
  std::string name;
  optional<std::string> phone;
};

template <class Inspector>
bool inspect(Inspector& f, person& x) {
  return f.object(x).fields(f.field("name", x.name), f.field("phone", x.phone));
}

class foobar {
public:
  const std::string& foo() {
    return foo_;
  }

  void foo(std::string value) {
    foo_ = std::move(value);
  }

  const std::string& bar() {
    return bar_;
  }

  void bar(std::string value) {
    bar_ = std::move(value);
  }

private:
  std::string foo_;
  std::string bar_;
};

template <class Inspector>
bool inspect(Inspector& f, foobar& x) {
  auto get_foo = [&x]() -> decltype(auto) { return x.foo(); };
  auto set_foo = [&x](std::string value) {
    x.foo(std::move(value));
    return true;
  };
  auto get_bar = [&x]() -> decltype(auto) { return x.bar(); };
  auto set_bar = [&x](std::string value) {
    x.bar(std::move(value));
    return true;
  };
  return f.object(x).fields(f.field("foo", get_foo, set_foo),
                            f.field("bar", get_bar, set_bar));
}

struct dummy_message {
  variant<std::string, double> content;
};

template <class Inspector>
bool inspect(Inspector& f, dummy_message& x) {
  return f.object(x).fields(f.field("content", x.content));
}

struct basics {
  struct tag {};
  tag v1;
  int32_t v2;
  int32_t v3[4];
  dummy_message v4[2];
  std::array<int32_t, 2> v5;
  std::tuple<int32_t, dummy_message> v6;
  std::map<std::string, int32_t> v7;
  std::vector<std::list<std::pair<std::string, std::array<int32_t, 3>>>> v8;
};

template <class Inspector>
bool inspect(Inspector& f, basics& x) {
  return f.object(x).fields(f.field("v1", x.v1), f.field("v2", x.v2),
                            f.field("v3", x.v3), f.field("v4", x.v4),
                            f.field("v5", x.v5), f.field("v6", x.v6),
                            f.field("v7", x.v7), f.field("v8", x.v8));
}

struct testee : serializer {
  std::string log;

  size_t indent = 0;

  void set_has_human_readable_format(bool new_value) {
    has_human_readable_format_ = new_value;
  }

  void new_line() {
    log += '\n';
    log.insert(log.end(), indent, ' ');
  }

  using serializer::object;

  template <class T>
  auto object(optional<T>&) {
    return object_t<testee>{"optional", this};
  }

  template <class... Ts>
  auto object(variant<Ts...>&) {
    return object_t<testee>{"variant", this};
  }

  bool inject_next_object_type(type_id_t type) override {
    new_line();
    log += "next object type: ";
    auto tn = detail::global_meta_object(type)->type_name;
    log.insert(log.end(), tn.begin(), tn.end());
    return ok;
  }

  bool begin_object(string_view object_name) override {
    new_line();
    indent += 2;
    log += "begin object ";
    log.insert(log.end(), object_name.begin(), object_name.end());
    return ok;
  }

  bool end_object() override {
    if (indent < 2)
      CAF_FAIL("begin/end mismatch");
    indent -= 2;
    new_line();
    log += "end object";
    return ok;
  }

  bool begin_field(string_view name) override {
    new_line();
    indent += 2;
    log += "begin field ";
    log.insert(log.end(), name.begin(), name.end());
    return ok;
  }

  bool begin_field(string_view name, bool) override {
    new_line();
    indent += 2;
    log += "begin optional field ";
    log.insert(log.end(), name.begin(), name.end());
    return ok;
  }

  bool begin_field(string_view name, span<const type_id_t>, size_t) override {
    new_line();
    indent += 2;
    log += "begin variant field ";
    log.insert(log.end(), name.begin(), name.end());
    return ok;
  }

  bool begin_field(string_view name, bool, span<const type_id_t>,
                   size_t) override {
    new_line();
    indent += 2;
    log += "begin optional variant field ";
    log.insert(log.end(), name.begin(), name.end());
    return ok;
  }

  bool end_field() override {
    if (indent < 2)
      CAF_FAIL("begin/end mismatch");
    indent -= 2;
    new_line();
    log += "end field";
    return ok;
  }

  bool begin_tuple(size_t size) override {
    new_line();
    indent += 2;
    log += "begin tuple of size ";
    log += std::to_string(size);
    return ok;
  }

  bool end_tuple() override {
    if (indent < 2)
      CAF_FAIL("begin/end mismatch");
    indent -= 2;
    new_line();
    log += "end tuple";
    return ok;
  }

  bool begin_sequence(size_t size) override {
    new_line();
    indent += 2;
    log += "begin sequence of size ";
    log += std::to_string(size);
    return ok;
  }

  bool end_sequence() override {
    if (indent < 2)
      CAF_FAIL("begin/end mismatch");
    indent -= 2;
    new_line();
    log += "end sequence";
    return ok;
  }

  bool value(bool) override {
    new_line();
    log += "bool value";
    return ok;
  }

  bool value(int8_t) override {
    new_line();
    log += "int8_t value";
    return ok;
  }

  bool value(uint8_t) override {
    new_line();
    log += "uint8_t value";
    return ok;
  }

  bool value(int16_t) override {
    new_line();
    log += "int16_t value";
    return ok;
  }

  bool value(uint16_t) override {
    new_line();
    log += "uint16_t value";
    return ok;
  }

  bool value(int32_t) override {
    new_line();
    log += "int32_t value";
    return ok;
  }

  bool value(uint32_t) override {
    new_line();
    log += "uint32_t value";
    return ok;
  }

  bool value(int64_t) override {
    new_line();
    log += "int64_t value";
    return ok;
  }

  bool value(uint64_t) override {
    new_line();
    log += "uint64_t value";
    return ok;
  }

  bool value(float) override {
    new_line();
    log += "float value";
    return ok;
  }

  bool value(double) override {
    new_line();
    log += "double value";
    return ok;
  }

  bool value(long double) override {
    new_line();
    log += "long double value";
    return ok;
  }

  bool value(string_view) override {
    new_line();
    log += "std::string value";
    return ok;
  }

  bool value(const std::u16string&) override {
    new_line();
    log += "std::u16string value";
    return ok;
  }

  bool value(const std::u32string&) override {
    new_line();
    log += "std::u32string value";
    return ok;
  }

  bool value(span<const byte>) override {
    new_line();
    log += "byte_span value";
    return ok;
  }
};

struct fixture {
  testee f;
};

} // namespace

CAF_TEST_FIXTURE_SCOPE(load_inspector_tests, fixture)

CAF_TEST(save inspectors can visit C arrays) {
  int32_t xs[] = {1, 2, 3};
  CAF_CHECK_EQUAL(detail::save_value(f, xs), true);
  CAF_CHECK_EQUAL(f.log, R"_(
begin tuple of size 3
  int32_t value
  int32_t value
  int32_t value
end tuple)_");
}

CAF_TEST(save inspectors can visit simple POD types) {
  point_3d p{1, 1, 1};
  CAF_CHECK_EQUAL(inspect(f, p), true);
  CAF_CHECK_EQUAL(p.x, 1);
  CAF_CHECK_EQUAL(p.y, 1);
  CAF_CHECK_EQUAL(p.z, 1);
  CAF_CHECK_EQUAL(f.log, R"_(
begin object point_3d
  begin field x
    int32_t value
  end field
  begin field y
    int32_t value
  end field
  begin field z
    int32_t value
  end field
end object)_");
}

CAF_TEST(save inspectors can visit node IDs) {
  auto tmp = make_node_id(42, "0102030405060708090A0B0C0D0E0F1011121314");
  auto hash_based_id = unbox(tmp);
  CAF_CHECK_EQUAL(inspect(f, hash_based_id), true);
  CAF_CHECK_EQUAL(f.log, R"_(
begin object caf::node_id
  begin optional variant field data
    begin object caf::hashed_node_id
      begin field process_id
        uint32_t value
      end field
      begin field host
        begin tuple of size 20
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
          uint8_t value
        end tuple
      end field
    end object
  end field
end object)_");
}

CAF_TEST(save inspectors recurse into members) {
  line l{point_3d{1, 1, 1}, point_3d{1, 1, 1}};
  CAF_CHECK_EQUAL(inspect(f, l), true);
  CAF_CHECK_EQUAL(l.p1.x, 1);
  CAF_CHECK_EQUAL(l.p1.y, 1);
  CAF_CHECK_EQUAL(l.p1.z, 1);
  CAF_CHECK_EQUAL(l.p2.x, 1);
  CAF_CHECK_EQUAL(l.p2.y, 1);
  CAF_CHECK_EQUAL(l.p2.z, 1);
  CAF_CHECK_EQUAL(f.log, R"_(
begin object line
  begin field p1
    begin object point_3d
      begin field x
        int32_t value
      end field
      begin field y
        int32_t value
      end field
      begin field z
        int32_t value
      end field
    end object
  end field
  begin field p2
    begin object point_3d
      begin field x
        int32_t value
      end field
      begin field y
        int32_t value
      end field
      begin field z
        int32_t value
      end field
    end object
  end field
end object)_");
}

CAF_TEST(save inspectors support fields with fallbacks and invariants) {
  CAF_MESSAGE("save inspectors suppress fields with their default value");
  {
    duration d{"seconds", 12.0};
    CAF_CHECK_EQUAL(inspect(f, d), true);
    CAF_CHECK_EQUAL(d.unit, "seconds");
    CAF_CHECK_EQUAL(d.count, 12.0);
    CAF_CHECK_EQUAL(f.log, R"_(
begin object duration
  begin optional field unit
  end field
  begin field count
    double value
  end field
end object)_");
  }
  f.log.clear();
  CAF_MESSAGE("save inspectors include fields with non-default value");
  {
    duration d{"minutes", 42.0};
    CAF_CHECK_EQUAL(inspect(f, d), true);
    CAF_CHECK_EQUAL(d.unit, "minutes");
    CAF_CHECK_EQUAL(d.count, 42.0);
    CAF_CHECK_EQUAL(f.log, R"_(
begin object duration
  begin optional field unit
    std::string value
  end field
  begin field count
    double value
  end field
end object)_");
  }
}

CAF_TEST(save inspectors support optional) {
  optional<int32_t> x;
  CAF_CHECK_EQUAL(inspect_object(f, x), true);
  CAF_CHECK_EQUAL(f.log, R"_(
begin object optional
  begin optional field value
  end field
end object)_");
}

CAF_TEST(save inspectors support fields with optional values) {
  person p1{"Eduard Example", none};
  CAF_CHECK_EQUAL(inspect(f, p1), true);
  CAF_CHECK_EQUAL(f.log, R"_(
begin object person
  begin field name
    std::string value
  end field
  begin optional field phone
  end field
end object)_");
  f.log.clear();
  person p2{"Bruce Almighty", std::string{"776-2323"}};
  CAF_CHECK_EQUAL(inspect(f, p2), true);
  CAF_CHECK_EQUAL(f.log, R"_(
begin object person
  begin field name
    std::string value
  end field
  begin optional field phone
    std::string value
  end field
end object)_");
}

CAF_TEST(save inspectors support fields with getters and setters) {
  foobar fb;
  fb.foo("hello");
  fb.bar("world");
  CAF_CHECK(inspect(f, fb));
  CAF_CHECK_EQUAL(fb.foo(), "hello");
  CAF_CHECK_EQUAL(fb.bar(), "world");
  CAF_CHECK_EQUAL(f.log, R"_(
begin object foobar
  begin field foo
    std::string value
  end field
  begin field bar
    std::string value
  end field
end object)_");
}

CAF_TEST(save inspectors support nasty data structures) {
  nasty x;
  CAF_CHECK(inspect(f, x));
  CAF_CHECK_EQUAL(f.get_error(), error{});
  CAF_CHECK_EQUAL(f.log, R"_(
begin object nasty
  begin field field_01
    int32_t value
  end field
  begin optional field field_02
  end field
  begin field field_03
    int32_t value
  end field
  begin optional field field_04
  end field
  begin optional field field_05
  end field
  begin optional field field_07
  end field
  begin variant field field_09
    std::string value
  end field
  begin optional variant field field_10
    std::string value
  end field
  begin variant field field_11
    std::string value
  end field
  begin optional variant field field_12
    std::string value
  end field
  begin field field_13
    begin tuple of size 2
      std::string value
      int32_t value
    end tuple
  end field
  begin optional field field_14
  end field
  begin field field_15
    begin tuple of size 2
      std::string value
      int32_t value
    end tuple
  end field
  begin optional field field_16
  end field
  begin field field_17
    int32_t value
  end field
  begin optional field field_18
  end field
  begin field field_19
    int32_t value
  end field
  begin optional field field_20
  end field
  begin optional field field_21
  end field
  begin optional field field_23
  end field
  begin variant field field_25
    std::string value
  end field
  begin optional variant field field_26
    std::string value
  end field
  begin variant field field_27
    std::string value
  end field
  begin optional variant field field_28
    std::string value
  end field
  begin field field_29
    begin tuple of size 2
      std::string value
      int32_t value
    end tuple
  end field
  begin optional field field_30
  end field
  begin field field_31
    begin tuple of size 2
      std::string value
      int32_t value
    end tuple
  end field
  begin optional field field_32
  end field
  begin optional variant field field_33
  end field
  begin optional field field_34
  end field
  begin optional variant field field_35
  end field
  begin optional field field_36
  end field
end object)_");
}

CAF_TEST(save inspectors support all basic STL types) {
  basics x;
  x.v7.emplace("one", 1);
  x.v7.emplace("two", 2);
  x.v7.emplace("three", 3);
  using v8_list = decltype(x.v8);
  using v8_nested_list = v8_list::value_type;
  using array_3i = std::array<int32_t, 3>;
  v8_nested_list v8_1;
  v8_1.emplace_back("hello", array_3i{{1, 2, 3}});
  v8_1.emplace_back("world", array_3i{{2, 3, 4}});
  v8_nested_list v8_2;
  v8_2.emplace_back("foo", array_3i{{0, 0, 0}});
  x.v8.emplace_back(std::move(v8_1));
  x.v8.emplace_back(std::move(v8_2));
  CAF_CHECK(inspect(f, x));
  CAF_CHECK_EQUAL(f.log, R"_(
begin object basics
  begin field v1
    begin object anonymous
    end object
  end field
  begin field v2
    int32_t value
  end field
  begin field v3
    begin tuple of size 4
      int32_t value
      int32_t value
      int32_t value
      int32_t value
    end tuple
  end field
  begin field v4
    begin tuple of size 2
      begin object dummy_message
        begin variant field content
          std::string value
        end field
      end object
      begin object dummy_message
        begin variant field content
          std::string value
        end field
      end object
    end tuple
  end field
  begin field v5
    begin tuple of size 2
      int32_t value
      int32_t value
    end tuple
  end field
  begin field v6
    begin tuple of size 2
      int32_t value
      begin object dummy_message
        begin variant field content
          std::string value
        end field
      end object
    end tuple
  end field
  begin field v7
    begin sequence of size 3
      begin tuple of size 2
        std::string value
        int32_t value
      end tuple
      begin tuple of size 2
        std::string value
        int32_t value
      end tuple
      begin tuple of size 2
        std::string value
        int32_t value
      end tuple
    end sequence
  end field
  begin field v8
    begin sequence of size 2
      begin sequence of size 2
        begin tuple of size 2
          std::string value
          begin tuple of size 3
            int32_t value
            int32_t value
            int32_t value
          end tuple
        end tuple
        begin tuple of size 2
          std::string value
          begin tuple of size 3
            int32_t value
            int32_t value
            int32_t value
          end tuple
        end tuple
      end sequence
      begin sequence of size 1
        begin tuple of size 2
          std::string value
          begin tuple of size 3
            int32_t value
            int32_t value
            int32_t value
          end tuple
        end tuple
      end sequence
    end sequence
  end field
end object)_");
}

CAF_TEST(save inspectors support messages) {
  auto x = make_message(1, "two", 3.0);
  CAF_MESSAGE("for machine-to-machine formats, messages prefix their types");
  CAF_CHECK(inspect(f, x));
  CAF_CHECK_EQUAL(f.log, R"_(
begin object message
  begin field types
    begin sequence of size 3
      uint16_t value
      uint16_t value
      uint16_t value
    end sequence
  end field
  begin field values
    begin tuple of size 3
      begin object int32_t
        begin field value
          int32_t value
        end field
      end object
      begin object std::string
        begin field value
          std::string value
        end field
      end object
      begin object double
        begin field value
          double value
        end field
      end object
    end tuple
  end field
end object)_");
  CAF_MESSAGE("for human-readable formats, messages inline type annotations");
  f.log.clear();
  f.set_has_human_readable_format(true);
  CAF_CHECK(inspect(f, x));
  CAF_CHECK_EQUAL(f.log, R"_(
begin object message
  begin field values
    begin sequence of size 3
      next object type: int32_t
      begin object int32_t
        begin field value
          int32_t value
        end field
      end object
      next object type: std::string
      begin object std::string
        begin field value
          std::string value
        end field
      end object
      next object type: double
      begin object double
        begin field value
          double value
        end field
      end object
    end sequence
  end field
end object)_");
}

CAF_TEST_FIXTURE_SCOPE_END()
