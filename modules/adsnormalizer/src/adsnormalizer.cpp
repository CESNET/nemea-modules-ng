/**
 * @file
 * @author Jaroslav Pesek <jaroslav.pesek@fit.cvut.cz>
 * @brief Implementation of the AdsNormalizer class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "adsnormalizer.hpp"
#include <unordered_map>

namespace {

using namespace Nemea;
using SwapFn = void(*)(UnirecRecord&, ur_field_id_t, ur_field_id_t);

static ur_field_id_t getUnirecIdByName(const char* str)
{
	auto unirecId = ur_get_id_by_name(str);
	if (unirecId == UR_E_INVALID_NAME) {
		throw std::runtime_error(std::string("Invalid Unirec name:") + str);
	}
	return static_cast<ur_field_id_t>(unirecId);
}

template<typename T>
void swapValues(UnirecRecord& record, ur_field_id_t field1, ur_field_id_t field2)
{
    auto value1 = record.getFieldAsType<T>(field1);
    auto value2 = record.getFieldAsType<T>(field2);
    record.setFieldFromType<T>(value2, field1);
    record.setFieldFromType<T>(value1, field2);
}


template<typename T>
void swap_fn(UnirecRecord& rec, ur_field_id_t f1, ur_field_id_t f2)
{
    swapValues<T>(rec, f1, f2);
}

static const std::unordered_map<int, SwapFn> swap_dispatch = {
    {UR_TYPE_IP, swap_fn<IpAddress>},
    {UR_TYPE_MAC, swap_fn<MacAddress>},
    {UR_TYPE_UINT8, swap_fn<uint8_t>},
    {UR_TYPE_UINT16, swap_fn<uint16_t>},
    {UR_TYPE_UINT32, swap_fn<uint32_t>},
    {UR_TYPE_UINT64, swap_fn<uint64_t>},
    {UR_TYPE_INT8, swap_fn<int8_t>},
    {UR_TYPE_INT16, swap_fn<int16_t>},
    {UR_TYPE_INT32, swap_fn<int32_t>},
    {UR_TYPE_INT64, swap_fn<int64_t>}
};

} // namespace

namespace AdsNorm {

AdsNormalizer::AdsNormalizer(const std::string& templateStr)
{
    std::vector<std::string> names;

    std::istringstream outer(templateStr);
    std::string token;

    while (std::getline(outer, token, ',')) {
        std::istringstream inner(token);
        std::string droppedType, name;

        if (!(inner >> droppedType >> name) || name.empty()) {
            throw std::invalid_argument("AdsNormalizer: malformed segment: \"" + token + '"');
        }
        names.push_back(std::move(name));
    }
    if (names.size() % 2 != 0) {
        throw std::invalid_argument(
            "AdsNormalizer: odd number of column names in template string");
    }

    m_str_fields.reserve(names.size() / 2);
    for (size_t i = 0; i < names.size(); i += 2) {
        m_str_fields.emplace_back(std::move(names[i]), std::move(names[i + 1]));
    }
}

UnirecRecord AdsNormalizer::swapColumnPairs(const UnirecRecordView& record)
{
    to_send_record.copyFieldsFrom(record);
    for (const auto& pair_id : m_id_fields) {
        auto field_type = ur_get_type(pair_id.first);
        auto it = swap_dispatch.find(field_type);
        if (it != swap_dispatch.end()) {
            it->second(to_send_record, pair_id.first, pair_id.second);
        } else {
            throw std::runtime_error("AdsNormalizer: unsupported field type for rotation");
        }
    }

    return to_send_record;
}

std::optional<UnirecRecordView> AdsNormalizer::sendOriginal(UnirecRecordView& record)
{
    m_stats.totalRecords++;

    auto prefix_tag = record.getFieldAsType<uint32_t>(m_prefix_tag_id);
    if (prefix_tag == 0) {
        return std::nullopt; // Do not send the record if PREFIX_TAG is 0.
    }
    m_stats.nonRotatedRecords++;
    return record;
}

std::optional<UnirecRecord> AdsNormalizer::sendRotated(UnirecRecordView& record)
{
    auto prefix_tag_dst = record.getFieldAsType<uint32_t>(m_prefix_tag_dst_id);
    if (prefix_tag_dst == 0) {
        return std::nullopt; // Do not send the record if PREFIX_TAG_DST is 0.
    }

    auto rotated_record = swapColumnPairs(record);
    m_stats.rotatedRecords++;
    return rotated_record;
}

void AdsNormalizer::updateUnirec(UnirecRecord& new_record)
{
    for(const auto& str_fields : m_str_fields) {
        ur_field_id_t field = getUnirecIdByName(str_fields.first.c_str());
        ur_field_id_t field_rev = getUnirecIdByName(str_fields.second.c_str());
        m_id_fields.emplace_back(std::make_pair(field, field_rev));
    }
    m_prefix_tag_id = getUnirecIdByName("PREFIX_TAG");
    m_prefix_tag_dst_id = getUnirecIdByName("PREFIX_TAG_DST");
    to_send_record = UnirecRecord(new_record);
}










} // namespace AdsNorm
