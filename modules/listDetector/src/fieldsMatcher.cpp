/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Implementation of the FieldsMatcher class for creating hash values of static fields of
 * Unirec records or rules and matching their dynamic fields
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fieldsMatcher.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <xxhash.h>

namespace ListDetector {

static std::pair<const char*, size_t>
getStaticFieldAsCharPtr(const Nemea::UnirecRecordView& unirecRecordView, ur_field_id_t fieldId)
{
	switch (ur_get_type(fieldId)) {
	case UR_TYPE_CHAR:
		return {unirecRecordView.getFieldAsType<const char*>(fieldId), sizeof(char)};
	case UR_TYPE_UINT8:
		return {
			reinterpret_cast<const char*>(unirecRecordView.getFieldAsType<const uint8_t*>(fieldId)),
			sizeof(uint8_t)};
	case UR_TYPE_INT8:
		return {
			reinterpret_cast<const char*>(unirecRecordView.getFieldAsType<const int8_t*>(fieldId)),
			sizeof(int8_t)};
	case UR_TYPE_UINT16:
		return {
			reinterpret_cast<const char*>(
				unirecRecordView.getFieldAsType<const uint16_t*>(fieldId)),
			sizeof(uint16_t)};
	case UR_TYPE_INT16:
		return {
			reinterpret_cast<const char*>(unirecRecordView.getFieldAsType<const int16_t*>(fieldId)),
			sizeof(int16_t)};
	case UR_TYPE_UINT32:
		return {
			reinterpret_cast<const char*>(
				unirecRecordView.getFieldAsType<const uint32_t*>(fieldId)),
			sizeof(uint32_t)};
	case UR_TYPE_INT32:
		return {
			reinterpret_cast<const char*>(unirecRecordView.getFieldAsType<const int32_t*>(fieldId)),
			sizeof(int32_t)};
	case UR_TYPE_UINT64:
		return {
			reinterpret_cast<const char*>(
				unirecRecordView.getFieldAsType<const uint64_t*>(fieldId)),
			sizeof(uint64_t)};
	case UR_TYPE_INT64:
		return {
			reinterpret_cast<const char*>(unirecRecordView.getFieldAsType<const int64_t*>(fieldId)),
			sizeof(int64_t)};
	case UR_TYPE_STRING: {
		auto str = unirecRecordView.getFieldAsType<std::string_view>(fieldId);
		return {str.data(), str.length()};
	}
	default:
		throw std::invalid_argument(
			std::string("Given field id ") + std::to_string(fieldId) + " is not a static field");
	}
	return {nullptr, 0};
}

static void writeFieldToBuffer(
	std::vector<std::byte>& buffer,
	size_t& writePos,
	const Nemea::UnirecRecordView& unirecRecordView,
	ur_field_id_t fieldId)
{
	auto [ptr, length] = getStaticFieldAsCharPtr(unirecRecordView, fieldId);
	if (writePos + length >= buffer.capacity()) {
		buffer.resize(writePos + 4 * length);
	}
	std::memcpy(buffer.data() + writePos, ptr, length);
	writePos += length;
}

uint64_t FieldsMatcher::calculateStaticHash(
	const Nemea::UnirecRecordView& unirecRecordView,
	size_t stringMask)
{
	size_t writePos = 0;

	for (const auto& fieldId : m_staticFieldIds) {
		writeFieldToBuffer(m_buffer, writePos, unirecRecordView, fieldId);
	}

	for (auto i = 0U; i < m_stringFieldIds.size(); i++) {
		if (((stringMask >> i) & 0b1) == 0U) {
			continue;
		}
		writeFieldToBuffer(m_buffer, writePos, unirecRecordView, m_stringFieldIds[i]);
	}
	return XXH64(m_buffer.data(), writePos, 0);
}

FieldsMatcher::FieldsMatcher(std::vector<Rule>& rules) noexcept
	: m_rules(rules)
{
	if (rules.empty()) {
		return;
	}

	const auto& ruleFields = rules[0].getRuleFields();
	for (const auto& ruleField : ruleFields) {
		if (Rule::isStaticType(ruleField.first)) {
			m_staticFieldIds.push_back(ruleField.first);
		}
	}
	for (const auto& ruleField : ruleFields) {
		if (Rule::isStringType(ruleField.first)) {
			m_stringFieldIds.push_back(ruleField.first);
		}
	}

	for (const auto& rule : rules) {
		addRule(rule.getRuleFields());
	}
}

void FieldsMatcher::addRule(const std::vector<RuleField>& ruleFields) noexcept
{
	auto totalBufferLength = std::accumulate(
		ruleFields.begin(),
		ruleFields.end(),
		0L,
		[](uint32_t length, const auto& ruleField) {
			if (Rule::isStaticType(ruleField.first)) {
				return length + sizeof(size_t);
			}
			if (Rule::isStringType(ruleField.first)
				&& std::holds_alternative<std::string>(ruleField.second.value())) {
				return length + std::get<std::string>(ruleField.second.value()).length();
			}
			return (size_t) length;
		});
	m_buffer.resize(totalBufferLength);
	m_rulesStaticHashIndexes.insert(std::make_pair(calculateStaticHash(ruleFields), m_ruleIndex++));
}

bool FieldsMatcher::anyOfRulesMatch(
	const Nemea::UnirecRecordView& unirecRecordView,
	const std::vector<bool>& previouslyMatchedRulesMask)
{
	bool match = false;
	const size_t stringPermutationCount = 1 << m_stringFieldIds.size();

	for (size_t stringMask = 0; !match && stringMask < stringPermutationCount; stringMask++) {
		const auto hashValue = calculateStaticHash(unirecRecordView, stringMask);

		for (auto [it, rangeEnd] = m_rulesStaticHashIndexes.equal_range(hashValue);
			 !match && it != rangeEnd;
			 it++) {
			match = previouslyMatchedRulesMask[it->second]
				&& m_rules[it->second].dynamicFieldsMatch(unirecRecordView);
		}
	}
	return match;
}

struct StaticFieldsHashVisitor {
	StaticFieldsHashVisitor(std::byte* buffer, uint32_t& writePos)
		: m_buffer(buffer)
		, m_writePos(writePos)
	{
	}

	template <typename T>
	void operator()(const T& value)
	{
		if constexpr (std::is_trivially_copyable_v<T>) {
			std::memcpy(m_buffer + m_writePos, &value, sizeof(value));
			m_writePos += sizeof(value);
		} else if constexpr (std::is_same_v<T, std::string>) {
			std::memcpy(m_buffer + m_writePos, value.c_str(), value.length());
			m_writePos += value.length();
		}
	}

private:
	std::byte* m_buffer;
	uint32_t& m_writePos;
};

uint64_t FieldsMatcher::calculateStaticHash(const std::vector<RuleField>& ruleFields)
{
	auto writePos = 0U;

	for (const auto ruleFieldId : m_staticFieldIds) {
		auto ruleFieldIt = std::find_if(
			ruleFields.begin(),
			ruleFields.end(),
			[ruleFieldId](const auto& ruleField) { return ruleFieldId == ruleField.first; });
		if (const auto& ruleFieldOpt = ruleFieldIt->second; ruleFieldOpt.has_value()) {
			std::visit(StaticFieldsHashVisitor {m_buffer.data(), writePos}, *ruleFieldOpt);
		}
	}

	for (const auto ruleFieldId : m_stringFieldIds) {
		auto ruleFieldIt = std::find_if(
			ruleFields.begin(),
			ruleFields.end(),
			[ruleFieldId](const auto& ruleField) { return ruleFieldId == ruleField.first; });
		if (const auto& ruleFieldOpt = ruleFieldIt->second;
			ruleFieldOpt.has_value() && std::holds_alternative<std::string>(ruleFieldOpt.value())) {
			std::visit(StaticFieldsHashVisitor {m_buffer.data(), writePos}, *ruleFieldOpt);
		}
	}

	return XXH64(m_buffer.data(), writePos, 0);
}

} // namespace ListDetector
