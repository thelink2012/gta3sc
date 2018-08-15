#pragma once
#include <gta3sc/adt/detail/span.hpp>

namespace gta3sc::adt
{
/// A span is a mutable view into a contiguous sequence of objects.
/// Please see https://en.cppreference.com/w/cpp/container/span
using nonstd::span;
using nonstd::operator==;
using nonstd::operator!=;
using nonstd::operator<;
using nonstd::operator<=;
using nonstd::operator>;
using nonstd::operator>=;
}
