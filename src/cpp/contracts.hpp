/// Contracts, Unreachable and Assertion
///
/// https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-expects
/// https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-ensures
/// 
#pragma once
#include <stdexcept>

// Assert
#include <cassert>

// http://stackoverflow.com/a/19343239/2679626
#ifndef STRINGIFY
#   define STRINGIFY_DETAIL(x) #x
#   define STRINGIFY(x) STRINGIFY_DETAIL(x)
#endif

#ifndef NDEBUG

struct broken_contract : std::runtime_error
{
    broken_contract(const char* msg)
        : std::runtime_error(msg)
    {}
};

struct unreachable_exception : std::runtime_error
{
    unreachable_exception(const char* msg)
        : std::runtime_error(msg)
    {}
};

/// I.6: Prefer Expects() for expressing preconditions.
/// See https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-expects
#define Expects(cond) \
    ((cond)? void(0) : throw broken_contract("Precondition failure at  " __FILE__ "(" STRINGIFY(__LINE__) "): " #cond "."))

/// I.8: Prefer Ensures() for expressing postconditions.
/// See https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Ri-ensures
#define Ensures(cond) \
    ((cond)? void(0) : throw broken_contract("Postcondition failure at  " __FILE__ "(" STRINGIFY(__LINE__) "): " #cond "."))

/// Unreachable code must be marked with this.
#define Unreachable() \
    (throw unreachable_exception("Unreachable code reached at " __FILE__ "(" STRINGIFY(__LINE__) ")."))

#else

#define Expects(cond)     void(0)
#define Ensures(cond)     void(0)
#define Unreachable()     \
    (throw std::runtime_error("Unreachable code reached"))

#endif
