#pragma once
#include <string>

#include "ai/BehaviorTrees/behaviors.hpp"
#include "ai/FiniteStateMachines/finite_state_machine.hpp"
#include "magic_enum/magic_enum.hpp"

namespace bee::ai
{
template <typename T>
    struct StructTests
    {
        template <typename U>
        static std::true_type TestToString(decltype(std::to_string(std::declval<U>())));

        template <typename>
        static std::false_type TestToString(...);

        template <typename U>
        static std::true_type TestIterators(typename U::iterator*);

        template <typename>
        static std::false_type TestIterators(...);

        template <typename U>
        static auto TestPushBack(U* u) -> decltype(u->push_back({}), std::true_type{});

        template <typename>
        static std::false_type TestPushBack(...);

        template <typename U>
        static auto TestInsert(U* u) -> decltype(u->insert({}), std::true_type{});

        template <typename>
        static std::false_type TestInsert(...);

        static constexpr bool toString = decltype(TestToString<T>(nullptr))::value;
        static constexpr bool iterators = decltype(TestIterators<T>(nullptr))::value;
        static constexpr bool pushBack = decltype(TestPushBack<T>(nullptr))::value;
        static constexpr bool insert = decltype(TestInsert<T>(nullptr))::value;
    };

    class EditorVariable
    {
    public:
        virtual ~EditorVariable() = default;
        virtual std::string ToString() const = 0;
        virtual std::type_index GetTypeInfo() const = 0;
        virtual void Deserialize(const std::string& serializedValue) = 0;
        virtual std::vector<std::string> GetEnumNames() = 0;
        virtual std::type_index GetUnderlyingType() const = 0;
    };

    template <typename T>
    class SerializedField : public EditorVariable
    {
    public:
        SerializedField(T& variable) : value(variable){};
        SerializedField(ai::BehaviorTreeAction& action, const std::string& name, T& variable) : value(variable)
        {
            action.RegisterEditorVariable(name, this);
        }

        SerializedField(ai::State& state, const std::string& name, T& variable) : value(variable)
        {
            state.RegisterEditorVariable(name, this);
        }

        T GetValue() { return value; }

        std::string ValueToString(T& val) const
        {
            if constexpr (std::is_enum_v<T>)
            {
                return std::string(magic_enum::enum_name(val));
            }

            if constexpr (std::is_same<T, std::string>::value)
            {
                return val;
            }

            
            if constexpr (std::is_same<T, std::filesystem::path>::value)
            {
                return val.string();
            }


            if constexpr (StructTests<T>::toString)
            {
                return std::to_string(val);
            }

            if constexpr (visit_struct::traits::is_visitable<T>::value)
            {
                std::stringstream ss;
                visit_struct::for_each(val, [&ss](const char* name, const auto& value) { ss << name << ":" << value << ' '; });
                return ss.str();
            }

            if constexpr (StructTests<T>::iterators)
            {
                std::stringstream ss;
                for (auto element : val)
                {
                    SerializedField<decltype(element)> f(element);
                    ss << "{" << f.ToString() << "}";
                }
                return ss.str();
            }

            return "";
        }

        std::string ToString() const override { return ValueToString(value); }

        T GetDeserializedValue(const std::string& string)
        {
            T toReturn;
            std::istringstream stringStream(string);

            if constexpr (visit_struct::traits::is_visitable<T>::value)
            {
                visit_struct::for_each(toReturn,[&stringStream](const char* name, auto& value)
                   {
                       std::string tempValue;
                       stringStream >> tempValue;
                       size_t colonPos = tempValue.find(':');

                       std::string stringName = tempValue.substr(0, colonPos);
                       std::stringstream val = std::stringstream(tempValue.substr(colonPos + 1));

                       val >> value;
                   });
                return toReturn;
            }

            if constexpr (std::is_same<T, std::filesystem::path>::value)
            {
                stringStream >> toReturn;
                return toReturn;
            }

            if constexpr (std::is_same<T, std::string>::value)
            {
                stringStream >> toReturn;
                return toReturn;
            }

            if constexpr (std::is_enum_v<T>)
            {
                auto temp = magic_enum::enum_cast<T>(stringStream.str());
                if (temp.has_value())
                {
                    toReturn = temp.value();
                }
                return toReturn;
            }

            if constexpr (StructTests<T>::toString)
            {
                stringStream >> toReturn;
                return toReturn;
            }

            if constexpr (StructTests<T>::iterators && !std::is_same<T, std::string>::value)
            {
                std::string element;
                while (std::getline(stringStream, element, '}'))
                {
                    // Extract the content inside curly braces
                    std::string content = element.substr(1, element.size() - 1);

                    using ElementType = std::remove_const_t<std::remove_reference_t<decltype(*toReturn.begin())>>;
                    ElementType d;
                    SerializedField<ElementType> temp(d);
                    temp.Deserialize(content);

                    if constexpr (StructTests<T>::pushBack)
                    {
                        toReturn.push_back(temp.GetValue());
                    }
                    else if constexpr (StructTests<T>::insert && !StructTests<T>::pushBack)
                    {
                        toReturn.insert(temp.GetValue());
                    }
                }
                return toReturn;
            }

            return toReturn;
        }

        void Deserialize(const std::string& serializedValue) override { value = GetDeserializedValue(serializedValue); }

        std::vector<std::string> ExtractElements(const std::string& input)
        {
            std::vector<std::string> elements;
            std::string element;

            bool insideBraces = false;
            for (char c : input)
            {
                if (c == '{')
                {
                    insideBraces = true;
                }
                else if (c == '}')
                {
                    insideBraces = false;
                    if (!element.empty())
                    {
                        elements.push_back(element);
                        element.clear();
                    }
                }
                else if (insideBraces)
                {
                    element += c;
                }
            }

            return elements;
        }

        std::type_index GetTypeInfo() const override { return typeid(T); }

        std::vector<std::string> GetEnumNames() override
        {
            assert(std::is_enum_v<T>);
            std::vector<std::string> toReturn;
            if constexpr (std::is_enum_v<T>)
            {
                constexpr auto names = magic_enum::enum_names<T>();
                for (auto name : names)
                {
                    toReturn.emplace_back(name);
                }
                return toReturn;
            }

            return {};
        }

        std::type_index GetUnderlyingType() const override
        {
            if constexpr (StructTests<T>::iterators)
            {
                return typeid(*value.begin());
            }
            return typeid(T);
        }

    private:
        T& value;
    };

}  // namespace bee::ai

#define SERIALIZE_FIELD(type, name) \
        type name;                      \
        bee::ai::SerializedField<type> name##Helper = bee::ai::SerializedField<type>(*this, #name, name);

