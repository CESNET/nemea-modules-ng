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
		buffer.resize(writePos + (4 * length));
	}
	std::memcpy(buffer.data() + writePos, ptr, length);
	writePos += length;
}

uint64_t FieldsMatcher::calculateStaticHash(
	const Nemea::UnirecRecordView& unirecRecordView,
	const std::vector<bool>& presentedStaticFieldsMask)
{
	size_t writePos = 0;
	for (auto fieldIndex = 0U; fieldIndex < m_fieldIds.size(); fieldIndex++) {
		if (!presentedStaticFieldsMask[fieldIndex]) {
			continue;
		}
		writeFieldToBuffer(m_buffer, writePos, unirecRecordView, m_fieldIds[fieldIndex]);
	}
	return XXH64(m_buffer.data(), writePos, 0);
}

FieldsMatcher::FieldsMatcher(std::vector<Rule>& rules)
	: m_rules(rules)
{
	if (!rules.empty()) {
		std::transform(
			rules.begin()->getRuleFields().begin(),
			rules.begin()->getRuleFields().end(),
			std::back_inserter(m_fieldIds),
			[](const RuleField& ruleField) { return ruleField.first; });
	}

	for (const auto& rule : rules) {
		resizeHashBuffer(rule);
		m_rulesStaticHashIndexes.insert(std::make_pair(calculateStaticHash(rule), m_ruleIndex++));
		m_presentedStaticFieldsMasks.emplace(rule.getPresentedStaticFieldsMask());
	}
}

void FieldsMatcher::resizeHashBuffer(const Rule& rule)
{
	const size_t totalBufferLength = std::accumulate(
		rule.getRuleFields().begin(),
		rule.getRuleFields().end(),
		0UL,
		[](uint32_t length, const auto& ruleField) -> size_t {
			// Wildcard and regex rule fields are not included in the hash value
			if (Rule::isWildcardRuleField(ruleField) || Rule::isRegexRuleField(ruleField)
				|| Rule::isIPRuleField(ruleField)) {
				return length;
			}
			if (Rule::isStaticRuleField(ruleField)) {
				return length + sizeof(size_t);
			}
			if (Rule::isStringRuleField(ruleField) && !Rule::isRegexRuleField(ruleField)) {
				return length + std::get<std::string>(ruleField.second.value()).length();
			}
			throw std::runtime_error("Unexpected rule field type");
		});
	m_buffer.resize(totalBufferLength);
}

bool FieldsMatcher::anyOfRulesMatch(
	const Nemea::UnirecRecordView& unirecRecordView,
	const std::vector<bool>& previouslyMatchedRulesMask)
{
	bool match = false;

	for (const auto& presentedStaticFieldsMask : m_presentedStaticFieldsMasks) {
		const size_t hashValue = calculateStaticHash(unirecRecordView, presentedStaticFieldsMask);

		for (auto [it, rangeEnd] = m_rulesStaticHashIndexes.equal_range(hashValue);
			 !match && it != rangeEnd;
			 it++) {
			match = previouslyMatchedRulesMask[it->second]
				&& m_rules[it->second].dynamicFieldsMatch(unirecRecordView);
		}
		if (match) {
			break;
		}
	}
	return match;
}

struct StaticFieldsHashVisitor {
	StaticFieldsHashVisitor(std::byte* buffer, size_t& writePos)
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
	size_t& m_writePos;
};

uint64_t FieldsMatcher::calculateStaticHash(const Rule& rule)
{
	size_t writePos = 0;
	for (const auto& ruleField : rule.getRuleFields()) {
		if (Rule::isWildcardRuleField(ruleField) || Rule::isRegexRuleField(ruleField)
			|| Rule::isIPRuleField(ruleField)) {
			continue;
		}
		if (const std::optional<RuleFieldValue>& ruleFieldOpt = ruleField.second;
			ruleFieldOpt.has_value()) {
			std::visit(StaticFieldsHashVisitor {m_buffer.data(), writePos}, *ruleFieldOpt);
		}
	}
	return XXH64(m_buffer.data(), writePos, 0);
}

} // namespace ListDetector
