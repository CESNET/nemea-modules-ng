/**
 * @file inserter.cpp
 * @author Daniel Pelanek <xpeland00@vutbr.cz>
 * @brief Defines functions for creating column lambdas for creating, writing
 *        and parsing unirec data into them.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "datatype.hpp"

#include <iostream>

template <unsigned Precision>
class ColumnDateTime64 : public clickhouse::ColumnDateTime64 {
public:
	ColumnDateTime64()
		: clickhouse::ColumnDateTime64(Precision)
	{
	}
};

namespace getters {

template <typename Value>
static Value get_value(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Value value = record.getFieldAsType<Value>(fieldID);
	return static_cast<Value>(value);
}

template <typename Value>
static std::vector<Value> get_value_arr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UnirecArray<Value> arr = record.getFieldAsUnirecArray<Value>(fieldID);
	std::vector<Value> result;
	result.reserve(arr.size());
	for (const auto& value : arr) {
		result.push_back(value);
	}
	return result;
}

static std::vector<uint8_t> get_bytes(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UnirecArray<std::byte> arr = record.getFieldAsUnirecArray<std::byte>(fieldID);
	std::vector<uint8_t> result;
	result.reserve(arr.size());
	for (const auto& value : arr) {
		result.push_back((char) value);
	}
	return result;
}

static in6_addr get_ip(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::IpAddress addr = record.getFieldAsType<Nemea::IpAddress>(fieldID);
	return *((in6_addr*) &addr.ip);
}

static std::vector<in6_addr> get_ip_arr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UnirecArray<Nemea::IpAddress> addr_arr
		= record.getFieldAsUnirecArray<Nemea::IpAddress>(fieldID);
	std::vector<in6_addr> result;
	result.reserve(addr_arr.size());
	for (const auto& value : addr_arr) {
		result.push_back(*((in6_addr*) &value.ip));
	}
	return result;
}

static std::vector<uint8_t> get_mac(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::MacAddress mac = record.getFieldAsType<Nemea::MacAddress>(fieldID);
	std::vector<uint8_t> result;
	result.reserve(6);
	for (int i = 0; i < 6; i++) {
		result.push_back(mac.mac.bytes[i]);
	}
	return result;
}

static std::vector<std::vector<uint8_t>>
get_mac_arr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UnirecArray<Nemea::MacAddress> mac_arr
		= record.getFieldAsUnirecArray<Nemea::MacAddress>(fieldID);
	std::vector<std::vector<uint8_t>> result;
	result.reserve(mac_arr.size());
	for (const auto& value : mac_arr) {
		result.push_back(std::vector<uint8_t>());
		for (int i = 0; i < 6; i++) {
			result.back().push_back(value.mac.bytes[i]);
		}
	}
	return result;
}

static uint64_t get_time(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UrTime time = record.getFieldAsType<Nemea::UrTime>(fieldID);
	return (
		(static_cast<uint64_t>(ur_time_get_sec(time.time)) * 1000 * 1000 * 1000)
		+ static_cast<uint64_t>(ur_time_get_nsec(time.time)));
}

static std::vector<uint64_t> get_time_arr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UnirecArray<Nemea::UrTime> time_arr
		= record.getFieldAsUnirecArray<Nemea::UrTime>(fieldID);
	std::vector<uint64_t> result;
	result.reserve(time_arr.size());
	for (const auto& value : time_arr) {
		result.push_back(
			(static_cast<uint64_t>(ur_time_get_sec(value.time)) * 1000 * 1000 * 1000)
			+ static_cast<uint64_t>(ur_time_get_nsec(value.time)));
	}
	return result;
}

static std::string get_string(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	std::string str = record.getFieldAsType<std::string>(fieldID);
	return str;
}
} // namespace getters

template <ColumnType>
struct DataTypeTraits {};

template <>
struct DataTypeTraits<ColumnType::UINT8> {
	using ColumnType = clickhouse::ColumnUInt8;
	static constexpr std::string_view ClickhouseTypeName = "UInt8";
	static constexpr auto Getter = &getters::get_value<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT16> {
	using ColumnType = clickhouse::ColumnUInt16;
	static constexpr std::string_view ClickhouseTypeName = "UInt16";
	static constexpr auto Getter = &getters::get_value<uint16_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT32> {
	using ColumnType = clickhouse::ColumnUInt32;
	static constexpr std::string_view ClickhouseTypeName = "UInt32";
	static constexpr auto Getter = &getters::get_value<uint32_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT64> {
	using ColumnType = clickhouse::ColumnUInt64;
	static constexpr std::string_view ClickhouseTypeName = "UInt64";
	static constexpr auto Getter = &getters::get_value<uint64_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT8> {
	using ColumnType = clickhouse::ColumnInt8;
	static constexpr std::string_view ClickhouseTypeName = "Int8";
	static constexpr auto Getter = &getters::get_value<int8_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT16> {
	using ColumnType = clickhouse::ColumnInt16;
	static constexpr std::string_view ClickhouseTypeName = "Int16";
	static constexpr auto Getter = &getters::get_value<int16_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT32> {
	using ColumnType = clickhouse::ColumnInt32;
	static constexpr std::string_view ClickhouseTypeName = "Int32";
	static constexpr auto Getter = &getters::get_value<int32_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT64> {
	using ColumnType = clickhouse::ColumnInt64;
	static constexpr std::string_view ClickhouseTypeName = "Int64";
	static constexpr auto Getter = &getters::get_value<int64_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT8_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt8)";
	static constexpr auto Getter = &getters::get_value_arr<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT16_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt16>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt16)";
	static constexpr auto Getter = &getters::get_value_arr<uint16_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT32_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt32>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt32)";
	static constexpr auto Getter = &getters::get_value_arr<uint32_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT64_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt64>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt64)";
	static constexpr auto Getter = &getters::get_value_arr<uint64_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT8_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt8>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Int8)";
	static constexpr auto Getter = &getters::get_value_arr<int8_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT16_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt16>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Int16)";
	static constexpr auto Getter = &getters::get_value_arr<int16_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT32_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt32>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Int32)";
	static constexpr auto Getter = &getters::get_value_arr<int32_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT64_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt64>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Int64)";
	static constexpr auto Getter = &getters::get_value_arr<int64_t>;
};

template <>
struct DataTypeTraits<ColumnType::CHAR> {
	using ColumnType = clickhouse::ColumnUInt8;
	static constexpr std::string_view ClickhouseTypeName = "Uint8";
	static constexpr auto Getter = &getters::get_value<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::CHAR_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Uint8)";
	static constexpr auto Getter = &getters::get_value_arr<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::FLOAT> {
	using ColumnType = clickhouse::ColumnFloat32;
	static constexpr std::string_view ClickhouseTypeName = "Float32";
	static constexpr auto Getter = &getters::get_value<float>;
};

template <>
struct DataTypeTraits<ColumnType::FLOAT_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnFloat32>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Float32)";
	static constexpr auto Getter = &getters::get_value_arr<float>;
};

template <>
struct DataTypeTraits<ColumnType::DOUBLE> {
	using ColumnType = clickhouse::ColumnFloat64;
	static constexpr std::string_view ClickhouseTypeName = "Float64";
	static constexpr auto Getter = &getters::get_value<double>;
};

template <>
struct DataTypeTraits<ColumnType::DOUBLE_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnFloat64>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Float64)";
	static constexpr auto Getter = &getters::get_value_arr<double>;
};

template <>
struct DataTypeTraits<ColumnType::IPADDR> {
	using ColumnType = clickhouse::ColumnIPv6;
	static constexpr std::string_view ClickhouseTypeName = "IPv6";
	static constexpr auto Getter = &getters::get_ip;
};

template <>
struct DataTypeTraits<ColumnType::IPADDR_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnIPv6>;
	static constexpr std::string_view ClickhouseTypeName = "Array(IPv6)";
	static constexpr auto Getter = &getters::get_ip_arr;
};

template <>
struct DataTypeTraits<ColumnType::MACADDR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt8)";
	static constexpr auto Getter = &getters::get_mac;
};

template <>
struct DataTypeTraits<ColumnType::MACADDR_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Array(UInt8))";
	static constexpr auto Getter = &getters::get_mac_arr;
};

template <>
struct DataTypeTraits<ColumnType::TIME> {
	using ColumnType = ColumnDateTime64<9>;
	static constexpr std::string_view ClickhouseTypeName = "DateTime64(9)";
	static constexpr auto Getter = &getters::get_time;
};

template <>
struct DataTypeTraits<ColumnType::TIME_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<ColumnDateTime64<9>>;
	static constexpr std::string_view ClickhouseTypeName = "Array(DateTime64(9))";
	static constexpr auto Getter = &getters::get_time_arr;
};

template <>
struct DataTypeTraits<ColumnType::STRING> {
	using ColumnType = clickhouse::ColumnString;
	static constexpr std::string_view ClickhouseTypeName = "String";
	static constexpr auto Getter = &getters::get_string;
};

template <>
struct DataTypeTraits<ColumnType::BYTES> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt8)";
	static constexpr auto Getter = &getters::get_bytes;
};

template <typename Func>
static void visit(ColumnType type, Func func)
{
	switch (type) {
	case ColumnType::INT8:
		func(DataTypeTraits<ColumnType::INT8> {});
		break;
	case ColumnType::INT16:
		func(DataTypeTraits<ColumnType::INT16> {});
		break;
	case ColumnType::INT32:
		func(DataTypeTraits<ColumnType::INT32> {});
		break;
	case ColumnType::INT64:
		func(DataTypeTraits<ColumnType::INT64> {});
		break;
	case ColumnType::UINT8:
		func(DataTypeTraits<ColumnType::UINT8> {});
		break;
	case ColumnType::UINT16:
		func(DataTypeTraits<ColumnType::UINT16> {});
		break;
	case ColumnType::UINT32:
		func(DataTypeTraits<ColumnType::UINT32> {});
		break;
	case ColumnType::UINT64:
		func(DataTypeTraits<ColumnType::UINT64> {});
		break;
	case ColumnType::CHAR:
		func(DataTypeTraits<ColumnType::CHAR> {});
		break;
	case ColumnType::FLOAT:
		func(DataTypeTraits<ColumnType::FLOAT> {});
		break;
	case ColumnType::DOUBLE:
		func(DataTypeTraits<ColumnType::DOUBLE> {});
		break;
	case ColumnType::IPADDR:
		func(DataTypeTraits<ColumnType::IPADDR> {});
		break;
	case ColumnType::TIME:
		func(DataTypeTraits<ColumnType::TIME> {});
		break;
	case ColumnType::STRING:
		func(DataTypeTraits<ColumnType::STRING> {});
		break;
	case ColumnType::INT8_ARR:
		func(DataTypeTraits<ColumnType::INT8_ARR> {});
		break;
	case ColumnType::INT16_ARR:
		func(DataTypeTraits<ColumnType::INT16_ARR> {});
		break;
	case ColumnType::INT32_ARR:
		func(DataTypeTraits<ColumnType::INT32_ARR> {});
		break;
	case ColumnType::INT64_ARR:
		func(DataTypeTraits<ColumnType::INT64_ARR> {});
		break;
	case ColumnType::UINT8_ARR:
		func(DataTypeTraits<ColumnType::UINT8_ARR> {});
		break;
	case ColumnType::UINT16_ARR:
		func(DataTypeTraits<ColumnType::UINT16_ARR> {});
		break;
	case ColumnType::UINT32_ARR:
		func(DataTypeTraits<ColumnType::UINT32_ARR> {});
		break;
	case ColumnType::UINT64_ARR:
		func(DataTypeTraits<ColumnType::UINT64_ARR> {});
		break;
	case ColumnType::CHAR_ARR:
		func(DataTypeTraits<ColumnType::CHAR_ARR> {});
		break;
	case ColumnType::FLOAT_ARR:
		func(DataTypeTraits<ColumnType::FLOAT_ARR> {});
		break;
	case ColumnType::DOUBLE_ARR:
		func(DataTypeTraits<ColumnType::DOUBLE_ARR> {});
		break;
	case ColumnType::IPADDR_ARR:
		func(DataTypeTraits<ColumnType::IPADDR_ARR> {});
		break;
	case ColumnType::MACADDR:
		func(DataTypeTraits<ColumnType::MACADDR> {});
		break;
	case ColumnType::MACADDR_ARR:
		func(DataTypeTraits<ColumnType::MACADDR_ARR> {});
		break;
	case ColumnType::TIME_ARR:
		func(DataTypeTraits<ColumnType::TIME_ARR> {});
		break;
	case ColumnType::BYTES:
		func(DataTypeTraits<ColumnType::BYTES> {});
		break;
	default:
		throw std::runtime_error("invalid data type");
	}
}

template <typename Func>
static void visit_non_arr(ColumnType type, Func func)
{
	switch (type) {
	case ColumnType::INT8:
		func(DataTypeTraits<ColumnType::INT8> {});
		break;
	case ColumnType::INT16:
		func(DataTypeTraits<ColumnType::INT16> {});
		break;
	case ColumnType::INT32:
		func(DataTypeTraits<ColumnType::INT32> {});
		break;
	case ColumnType::INT64:
		func(DataTypeTraits<ColumnType::INT64> {});
		break;
	case ColumnType::UINT8:
		func(DataTypeTraits<ColumnType::UINT8> {});
		break;
	case ColumnType::UINT16:
		func(DataTypeTraits<ColumnType::UINT16> {});
		break;
	case ColumnType::UINT32:
		func(DataTypeTraits<ColumnType::UINT32> {});
		break;
	case ColumnType::UINT64:
		func(DataTypeTraits<ColumnType::UINT64> {});
		break;
	case ColumnType::CHAR:
		func(DataTypeTraits<ColumnType::CHAR> {});
		break;
	case ColumnType::FLOAT:
		func(DataTypeTraits<ColumnType::FLOAT> {});
		break;
	case ColumnType::DOUBLE:
		func(DataTypeTraits<ColumnType::DOUBLE> {});
		break;
	case ColumnType::IPADDR:
		func(DataTypeTraits<ColumnType::IPADDR> {});
		break;
	case ColumnType::TIME:
		func(DataTypeTraits<ColumnType::TIME> {});
		break;
	case ColumnType::STRING:
		func(DataTypeTraits<ColumnType::STRING> {});
		break;
	default:
		throw std::runtime_error("invalid data type");
	}
}

template <typename Func>
static void visit_arr(ColumnType type, Func func)
{
	switch (type) {
	case ColumnType::INT8_ARR:
		func(DataTypeTraits<ColumnType::INT8_ARR> {});
		break;
	case ColumnType::INT16_ARR:
		func(DataTypeTraits<ColumnType::INT16_ARR> {});
		break;
	case ColumnType::INT32_ARR:
		func(DataTypeTraits<ColumnType::INT32_ARR> {});
		break;
	case ColumnType::INT64_ARR:
		func(DataTypeTraits<ColumnType::INT64_ARR> {});
		break;
	case ColumnType::UINT8_ARR:
		func(DataTypeTraits<ColumnType::UINT8_ARR> {});
		break;
	case ColumnType::UINT16_ARR:
		func(DataTypeTraits<ColumnType::UINT16_ARR> {});
		break;
	case ColumnType::UINT32_ARR:
		func(DataTypeTraits<ColumnType::UINT32_ARR> {});
		break;
	case ColumnType::UINT64_ARR:
		func(DataTypeTraits<ColumnType::UINT64_ARR> {});
		break;
	case ColumnType::CHAR_ARR:
		func(DataTypeTraits<ColumnType::CHAR_ARR> {});
		break;
	case ColumnType::FLOAT_ARR:
		func(DataTypeTraits<ColumnType::FLOAT_ARR> {});
		break;
	case ColumnType::DOUBLE_ARR:
		func(DataTypeTraits<ColumnType::DOUBLE_ARR> {});
		break;
	case ColumnType::IPADDR_ARR:
		func(DataTypeTraits<ColumnType::IPADDR_ARR> {});
		break;
	case ColumnType::MACADDR:
		func(DataTypeTraits<ColumnType::MACADDR> {});
		break;
	case ColumnType::MACADDR_ARR:
		func(DataTypeTraits<ColumnType::MACADDR_ARR> {});
		break;
	case ColumnType::TIME_ARR:
		func(DataTypeTraits<ColumnType::TIME_ARR> {});
		break;
	case ColumnType::BYTES:
		func(DataTypeTraits<ColumnType::BYTES> {});
		break;
	default:
		throw std::runtime_error("invalid data type");
	}
}

inline bool is_arr(ColumnType type)
{
	return type < ColumnType::INT8;
}

std::shared_ptr<clickhouse::Column> make_column(ColumnType type)
{
	std::shared_ptr<clickhouse::Column> column;
	if (is_arr(type)) {
		visit(type, [&](auto traits) {
			using ColType = typename decltype(traits)::ColumnType;
			column = std::make_shared<ColType>();
		});
	} else {
		visit(type, [&](auto traits) {
			using ColType = clickhouse::ColumnNullableT<typename decltype(traits)::ColumnType>;
			column = std::make_shared<ColType>();
		});
	}
	return column;
}

GetterFn make_getter(ColumnType type)
{
	GetterFn getter;
	visit(type, [&](auto traits) {
		getter = [](Nemea::UnirecRecordView& record, ur_field_id_t fieldID, ValueVariant& value) {
			value = decltype(traits)::Getter(record, fieldID);
		};
	});
	return getter;
}

ColumnWriterFn make_columnwriter(ColumnType type)
{
	ColumnWriterFn columnwriter;

	if (is_arr(type)) {
		visit_arr(type, [&](auto traits) {
			columnwriter = [](ValueVariant* value, clickhouse::Column& column) {
				using ColumnType = typename decltype(traits)::ColumnType;
				using ValueType = std::invoke_result_t<
					decltype(decltype(traits)::Getter),
					Nemea::UnirecRecordView&,
					ur_field_type_t>;
				auto* col = dynamic_cast<ColumnType*>(&column);
				if (value) {
					col->Append(std::get<ValueType>(*value));
				}
			};
		});
	} else {
		visit_non_arr(type, [&](auto traits) {
			columnwriter = [](ValueVariant* value, clickhouse::Column& column) {
				using ColumnType
					= clickhouse::ColumnNullableT<typename decltype(traits)::ColumnType>;
				using ValueType = std::invoke_result_t<
					decltype(decltype(traits)::Getter),
					Nemea::UnirecRecordView&,
					ur_field_type_t>;
				auto* col = dynamic_cast<ColumnType*>(&column);
				if (!value) {
					col->Append(std::nullopt);
				} else {
					col->Append(std::get<ValueType>(*value));
				}
			};
		});
	}

	return columnwriter;
}

std::string type_to_clickhouse(ColumnType type)
{
	std::string result;
	visit(type, [&](auto traits) { result = traits.ClickhouseTypeName; });
	return result;
}
