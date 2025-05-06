#pragma once
#include <sstream>
#include <string>
#include <type_traits>
#include "blackboard.hpp"

template <typename T>
struct OperatorTests
{
    template <typename U>
    static std::true_type testEqual(decltype(std::declval<U>() == std::declval<U>())*);

    template <typename>
    static std::false_type testEqual(...);

    template <typename U>
    static std::true_type testNotEqual(decltype(std::declval<U>() != std::declval<U>())*);

    template <typename>
    static std::false_type testNotEqual(...);

    template <typename U>
    static std::true_type testLess(decltype(std::declval<U>() < std::declval<U>())*);

    template <typename>
    static std::false_type testLess(...);

    template <typename U>
    static std::true_type testLessEqual(decltype(std::declval<U>() <= std::declval<U>())*);

    template <typename>
    static std::false_type testLessEqual(...);

    template <typename U>
    static std::true_type testGreater(decltype(std::declval<U>() > std::declval<U>())*);

    template <typename>
    static std::false_type testGreater(...);

    template <typename U>
    static std::true_type testGreaterEqual(decltype(std::declval<U>() >= std::declval<U>())*);

    template <typename>
    static std::false_type testGreaterEqual(...);

    static constexpr bool equality = decltype(testEqual<T>(nullptr))::value;
    static constexpr bool inequality = decltype(testNotEqual<T>(nullptr))::value;
    static constexpr bool less = decltype(testLess<T>(nullptr))::value;
    static constexpr bool lessEqual = decltype(testLessEqual<T>(nullptr))::value;
    static constexpr bool greater = decltype(testGreater<T>(nullptr))::value;
    static constexpr bool greaterEqual = decltype(testGreaterEqual<T>(nullptr))::value;
};

namespace bee::ai
{
class Blackboard;

enum class ComparisonType
{
    EQUAL = 0,
    NOT_EQUAL = 1,
    LESS = 2,
    LESS_EQUAL = 3,
    GREATER = 4,
    GREATER_EQUAL = 5
};

class IComparator
{
public:
    virtual bool Evaluate(const Blackboard& blackboard) const { return false; }
    virtual ~IComparator() = default;
    virtual std::string ToString() const { return {}; }
};

template <typename T>
class Comparator : public IComparator
{
public:
    Comparator(const std::string& comparison_key, const ComparisonType comparison_type, const T& value)
        : m_comparisonKey(comparison_key), m_comparisonType(comparison_type), m_value(value)
    {
    }

    std::string ToString() const override;

    bool Evaluate(const Blackboard& blackboard) const override;

    std::string GetComparisonKey() const { return m_comparisonKey; }
    ComparisonType GetComparisonType() const { return m_comparisonType; }
    T GetValue() const { return m_value; }
private:
    std::string m_comparisonKey; 
    ComparisonType m_comparisonType; 
    T m_value;
};

template <typename T>
std::string Comparator<T>::ToString() const
{
    std::stringstream toReturn;
    toReturn << m_comparisonKey << " ";

    if constexpr (std::is_same_v<T, std::string>)
    {
        toReturn << "string"
                 << " ";
    }
    else
    {
        toReturn << typeid(T).name() << " ";
    }

    toReturn << static_cast<int>(m_comparisonType) << " ";
    toReturn << m_value << " ";
    return toReturn.str();
}

template <typename T>
bool Comparator<T>::Evaluate(const Blackboard& blackboard) const
{
    switch (m_comparisonType)
    {
        case ComparisonType::EQUAL:
            if constexpr (OperatorTests<T>::equality)
            {
                    auto query = blackboard.TryGet<T>(m_comparisonKey);
                    if (query == nullptr) return false;
                    return *query == m_value;
            }
            else
            {
                return false;
            }
        case ComparisonType::NOT_EQUAL:
            if constexpr (OperatorTests<T>::inequality)
            {
                auto query = blackboard.TryGet<T>(m_comparisonKey);
                if (query == nullptr) return false;
                return *query != m_value;
            }
            else
            {
                return false;
            }
        case ComparisonType::LESS:
            if constexpr (OperatorTests<T>::less)
            {
                auto query = blackboard.TryGet<T>(m_comparisonKey);
                if (query == nullptr) return false;
                return *query < m_value;
            }
            else
            {
                return false;
            }
        case ComparisonType::LESS_EQUAL:
            if constexpr (OperatorTests<T>::lessEqual)
            {
                auto query = blackboard.TryGet<T>(m_comparisonKey);
                if (query == nullptr) return false;
                return *query <= m_value;
            }
            else
            {
                return false;
            }
        case ComparisonType::GREATER:
            if constexpr (OperatorTests<T>::greater)
            {
                auto query = blackboard.TryGet<T>(m_comparisonKey);
                if (query == nullptr) return false;
                return *query > m_value;
            }
            else
            {
                return false;
            }
        case ComparisonType::GREATER_EQUAL:
            if constexpr (OperatorTests<T>::greaterEqual)
            {
                auto query = blackboard.TryGet<T>(m_comparisonKey);
                if (query == nullptr) return false;
                return *query >= m_value;
            }
            else
            {
                return false;
            }
        default:
            return false;
    }
}
}  // namespace bee::ai