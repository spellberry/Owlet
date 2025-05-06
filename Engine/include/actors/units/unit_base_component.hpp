#pragma once
#include <unordered_map>
#include <utility>
#include <string>

//This file is currently not in use, but it might come in handy in the future

enum AttributesOperations
{
	Addition,
	Multiplication,
	Percentage
};

struct AttributeModifier
{
	//std::unordered_map<BaseAttributes, std::pair<float, AttributesOperations>> m_Modifiers {};
	std::string m_AttributeName = "";
};