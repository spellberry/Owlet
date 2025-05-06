#include "core/engine.hpp"
#include "tools/log.hpp"
#include "tools/serialization.hpp"

using namespace bee;

bee::Serializer::~Serializer() { Log::Info("~Serializer"); }

bool bee::Serializer::HasItem(Serializable& s) { return m_mappings.find(&s) != m_mappings.end(); }

const std::string& bee::Serializer::GetMapping(Serializable& s)
{
    assert(HasItem(s));
    return m_mappings[&s];
}

void Serializer::Add(Serializable* s) { m_mappings[s] = ""; }

void bee::Serializer::Update(Serializable* s, const std::string& mapping) { m_mappings[s] = mapping; }

void Serializer::Remove(Serializable* s)
{
    if (m_mappings.find(s) != m_mappings.end())
    {
        m_mappings.erase(s);
    }
}
