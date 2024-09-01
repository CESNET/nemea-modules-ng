/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Implementation of the StaticFieldsHasher class for creating hash values of static fields
 * of Unirec records or rules
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "staticFieldsHasher.hpp"

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
	default:
		throw std::invalid_argument(
			std::string("Given field id ") + std::to_string(fieldId) + " is not a static field");
	}
	return {nullptr, 0};
}

uint64_t StaticFieldsHasher::calculateStaticHash(const Nemea::UnirecRecordView& unirecRecordView)
{
	auto writePos = 0U;

	for (const auto& fieldId : m_staticFieldIds) {
		auto [ptr, length] = getStaticFieldAsCharPtr(unirecRecordView, fieldId);
		std::memcpy(m_buffer.data() + writePos, ptr, length);
		writePos += length;
	}
	return XXH64(m_buffer.data(), writePos, 0);
}

StaticFieldsHasher::StaticFieldsHasher(const std::vector<ur_field_id_t>& ruleFields) noexcept
{
	std::copy_if(
		ruleFields.begin(),
		ruleFields.end(),
		std::inserter(m_staticFieldIds, m_staticFieldIds.begin()),
		[](const auto& fieldId) { return Rule::isStaticType(fieldId); });
	m_buffer.resize(m_staticFieldIds.size() * sizeof(size_t));
}

void StaticFieldsHasher::addRule(const std::vector<RuleField>& ruleFields) noexcept
{
	m_rulesStaticHashIndexes.insert(std::make_pair(calculateStaticHash(ruleFields), m_ruleIndex++));
}

std::vector<bool>
StaticFieldsHasher::getMatchingStaticHashMask(const Nemea::UnirecRecordView& unirecRecordView)
{
	std::vector<bool> res(m_ruleIndex);

	const auto hashValue = calculateStaticHash(unirecRecordView);

	for (auto [it, rangeEnd] = m_rulesStaticHashIndexes.equal_range(hashValue); it != rangeEnd;
		 it++) {
		res[it->second] = true;
	}
	return res;
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
		}
	}

private:
	std::byte* m_buffer;
	uint32_t& m_writePos;
};

uint64_t StaticFieldsHasher::calculateStaticHash(const std::vector<RuleField>& ruleFields)
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

	return XXH64(m_buffer.data(), writePos, 0);
}

} // namespace ListDetector
