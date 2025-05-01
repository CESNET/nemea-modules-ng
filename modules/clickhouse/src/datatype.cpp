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
struct DataTypeTraits<ColumnType::UInt8> {
	using ColumnType = clickhouse::ColumnUInt8;
	static constexpr std::string_view ClickhouseTypeName = "UInt8";
	static constexpr auto Getter = &getters::get_value<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::UInt16> {
	using ColumnType = clickhouse::ColumnUInt16;
	static constexpr std::string_view ClickhouseTypeName = "UInt16";
	static constexpr auto Getter = &getters::get_value<uint16_t>;
};

template <>
struct DataTypeTraits<ColumnType::UInt32> {
	using ColumnType = clickhouse::ColumnUInt32;
	static constexpr std::string_view ClickhouseTypeName = "UInt32";
	static constexpr auto Getter = &getters::get_value<uint32_t>;
};

template <>
struct DataTypeTraits<ColumnType::UInt64> {
	using ColumnType = clickhouse::ColumnUInt64;
	static constexpr std::string_view ClickhouseTypeName = "UInt64";
	static constexpr auto Getter = &getters::get_value<uint64_t>;
};

template <>
struct DataTypeTraits<ColumnType::Int8> {
	using ColumnType = clickhouse::ColumnInt8;
	static constexpr std::string_view ClickhouseTypeName = "Int8";
	static constexpr auto Getter = &getters::get_value<int8_t>;
};

template <>
struct DataTypeTraits<ColumnType::Int16> {
	using ColumnType = clickhouse::ColumnInt16;
	static constexpr std::string_view ClickhouseTypeName = "Int16";
	static constexpr auto Getter = &getters::get_value<int16_t>;
};

template <>
struct DataTypeTraits<ColumnType::Int32> {
	using ColumnType = clickhouse::ColumnInt32;
	static constexpr std::string_view ClickhouseTypeName = "Int32";
	static constexpr auto Getter = &getters::get_value<int32_t>;
};

template <>
struct DataTypeTraits<ColumnType::Int64> {
	using ColumnType = clickhouse::ColumnInt64;
	static constexpr std::string_view ClickhouseTypeName = "Int64";
	static constexpr auto Getter = &getters::get_value<int64_t>;
};

template <>
struct DataTypeTraits<ColumnType::UInt8Arr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt8)";
	static constexpr auto Getter = &getters::get_value_arr<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::UInt16Arr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt16>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt16)";
	static constexpr auto Getter = &getters::get_value_arr<uint16_t>;
};

template <>
struct DataTypeTraits<ColumnType::UInt32Arr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt32>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt32)";
	static constexpr auto Getter = &getters::get_value_arr<uint32_t>;
};

template <>
struct DataTypeTraits<ColumnType::UInt64Arr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt64>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt64)";
	static constexpr auto Getter = &getters::get_value_arr<uint64_t>;
};

template <>
struct DataTypeTraits<ColumnType::Int8Arr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt8>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Int8)";
	static constexpr auto Getter = &getters::get_value_arr<int8_t>;
};

template <>
struct DataTypeTraits<ColumnType::Int16Arr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt16>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Int16)";
	static constexpr auto Getter = &getters::get_value_arr<int16_t>;
};

template <>
struct DataTypeTraits<ColumnType::Int32Arr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt32>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Int32)";
	static constexpr auto Getter = &getters::get_value_arr<int32_t>;
};

template <>
struct DataTypeTraits<ColumnType::Int64Arr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt64>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Int64)";
	static constexpr auto Getter = &getters::get_value_arr<int64_t>;
};

template <>
struct DataTypeTraits<ColumnType::Char> {
	using ColumnType = clickhouse::ColumnUInt8;
	static constexpr std::string_view ClickhouseTypeName = "Uint8";
	static constexpr auto Getter = &getters::get_value<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::CharArr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Uint8)";
	static constexpr auto Getter = &getters::get_value_arr<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::Float> {
	using ColumnType = clickhouse::ColumnFloat32;
	static constexpr std::string_view ClickhouseTypeName = "Float32";
	static constexpr auto Getter = &getters::get_value<float>;
};

template <>
struct DataTypeTraits<ColumnType::FloatArr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnFloat32>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Float32)";
	static constexpr auto Getter = &getters::get_value_arr<float>;
};

template <>
struct DataTypeTraits<ColumnType::Double> {
	using ColumnType = clickhouse::ColumnFloat64;
	static constexpr std::string_view ClickhouseTypeName = "Float64";
	static constexpr auto Getter = &getters::get_value<double>;
};

template <>
struct DataTypeTraits<ColumnType::DoubleArr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnFloat64>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Float64)";
	static constexpr auto Getter = &getters::get_value_arr<double>;
};

template <>
struct DataTypeTraits<ColumnType::Ipaddr> {
	using ColumnType = clickhouse::ColumnIPv6;
	static constexpr std::string_view ClickhouseTypeName = "IPv6";
	static constexpr auto Getter = &getters::get_ip;
};

template <>
struct DataTypeTraits<ColumnType::IpaddrArr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnIPv6>;
	static constexpr std::string_view ClickhouseTypeName = "Array(IPv6)";
	static constexpr auto Getter = &getters::get_ip_arr;
};

template <>
struct DataTypeTraits<ColumnType::Macaddr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt8)";
	static constexpr auto Getter = &getters::get_mac;
};

template <>
struct DataTypeTraits<ColumnType::MacaddrArr> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>>;
	static constexpr std::string_view ClickhouseTypeName = "Array(Array(UInt8))";
	static constexpr auto Getter = &getters::get_mac_arr;
};

template <>
struct DataTypeTraits<ColumnType::Time> {
	using ColumnType = ColumnDateTime64<9>;
	static constexpr std::string_view ClickhouseTypeName = "DateTime64(9)";
	static constexpr auto Getter = &getters::get_time;
};

template <>
struct DataTypeTraits<ColumnType::TimeArr> {
	using ColumnType = clickhouse::ColumnArrayT<ColumnDateTime64<9>>;
	static constexpr std::string_view ClickhouseTypeName = "Array(DateTime64(9))";
	static constexpr auto Getter = &getters::get_time_arr;
};

template <>
struct DataTypeTraits<ColumnType::String> {
	using ColumnType = clickhouse::ColumnString;
	static constexpr std::string_view ClickhouseTypeName = "String";
	static constexpr auto Getter = &getters::get_string;
};

template <>
struct DataTypeTraits<ColumnType::Bytes> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view ClickhouseTypeName = "Array(UInt8)";
	static constexpr auto Getter = &getters::get_bytes;
};

template <typename Func>
static void visit(ColumnType type, Func func)
{
	switch (type) {
	case ColumnType::Int8:
		func(DataTypeTraits<ColumnType::Int8> {});
		break;
	case ColumnType::Int16:
		func(DataTypeTraits<ColumnType::Int16> {});
		break;
	case ColumnType::Int32:
		func(DataTypeTraits<ColumnType::Int32> {});
		break;
	case ColumnType::Int64:
		func(DataTypeTraits<ColumnType::Int64> {});
		break;
	case ColumnType::UInt8:
		func(DataTypeTraits<ColumnType::UInt8> {});
		break;
	case ColumnType::UInt16:
		func(DataTypeTraits<ColumnType::UInt16> {});
		break;
	case ColumnType::UInt32:
		func(DataTypeTraits<ColumnType::UInt32> {});
		break;
	case ColumnType::UInt64:
		func(DataTypeTraits<ColumnType::UInt64> {});
		break;
	case ColumnType::Char:
		func(DataTypeTraits<ColumnType::Char> {});
		break;
	case ColumnType::Float:
		func(DataTypeTraits<ColumnType::Float> {});
		break;
	case ColumnType::Double:
		func(DataTypeTraits<ColumnType::Double> {});
		break;
	case ColumnType::Ipaddr:
		func(DataTypeTraits<ColumnType::Ipaddr> {});
		break;
	case ColumnType::Time:
		func(DataTypeTraits<ColumnType::Time> {});
		break;
	case ColumnType::String:
		func(DataTypeTraits<ColumnType::String> {});
		break;
	case ColumnType::Int8Arr:
		func(DataTypeTraits<ColumnType::Int8Arr> {});
		break;
	case ColumnType::Int16Arr:
		func(DataTypeTraits<ColumnType::Int16Arr> {});
		break;
	case ColumnType::Int32Arr:
		func(DataTypeTraits<ColumnType::Int32Arr> {});
		break;
	case ColumnType::Int64Arr:
		func(DataTypeTraits<ColumnType::Int64Arr> {});
		break;
	case ColumnType::UInt8Arr:
		func(DataTypeTraits<ColumnType::UInt8Arr> {});
		break;
	case ColumnType::UInt16Arr:
		func(DataTypeTraits<ColumnType::UInt16Arr> {});
		break;
	case ColumnType::UInt32Arr:
		func(DataTypeTraits<ColumnType::UInt32Arr> {});
		break;
	case ColumnType::UInt64Arr:
		func(DataTypeTraits<ColumnType::UInt64Arr> {});
		break;
	case ColumnType::CharArr:
		func(DataTypeTraits<ColumnType::CharArr> {});
		break;
	case ColumnType::FloatArr:
		func(DataTypeTraits<ColumnType::FloatArr> {});
		break;
	case ColumnType::DoubleArr:
		func(DataTypeTraits<ColumnType::DoubleArr> {});
		break;
	case ColumnType::IpaddrArr:
		func(DataTypeTraits<ColumnType::IpaddrArr> {});
		break;
	case ColumnType::Macaddr:
		func(DataTypeTraits<ColumnType::Macaddr> {});
		break;
	case ColumnType::MacaddrArr:
		func(DataTypeTraits<ColumnType::MacaddrArr> {});
		break;
	case ColumnType::TimeArr:
		func(DataTypeTraits<ColumnType::TimeArr> {});
		break;
	case ColumnType::Bytes:
		func(DataTypeTraits<ColumnType::Bytes> {});
		break;
	default:
		throw std::runtime_error("invalid data type");
	}
}

template <typename Func>
static void visit_non_arr(ColumnType type, Func func)
{
	switch (type) {
	case ColumnType::Int8:
		func(DataTypeTraits<ColumnType::Int8> {});
		break;
	case ColumnType::Int16:
		func(DataTypeTraits<ColumnType::Int16> {});
		break;
	case ColumnType::Int32:
		func(DataTypeTraits<ColumnType::Int32> {});
		break;
	case ColumnType::Int64:
		func(DataTypeTraits<ColumnType::Int64> {});
		break;
	case ColumnType::UInt8:
		func(DataTypeTraits<ColumnType::UInt8> {});
		break;
	case ColumnType::UInt16:
		func(DataTypeTraits<ColumnType::UInt16> {});
		break;
	case ColumnType::UInt32:
		func(DataTypeTraits<ColumnType::UInt32> {});
		break;
	case ColumnType::UInt64:
		func(DataTypeTraits<ColumnType::UInt64> {});
		break;
	case ColumnType::Char:
		func(DataTypeTraits<ColumnType::Char> {});
		break;
	case ColumnType::Float:
		func(DataTypeTraits<ColumnType::Float> {});
		break;
	case ColumnType::Double:
		func(DataTypeTraits<ColumnType::Double> {});
		break;
	case ColumnType::Ipaddr:
		func(DataTypeTraits<ColumnType::Ipaddr> {});
		break;
	case ColumnType::Time:
		func(DataTypeTraits<ColumnType::Time> {});
		break;
	case ColumnType::String:
		func(DataTypeTraits<ColumnType::String> {});
		break;
	default:
		throw std::runtime_error("invalid data type");
	}
}

template <typename Func>
static void visit_arr(ColumnType type, Func func)
{
	switch (type) {
	case ColumnType::Int8Arr:
		func(DataTypeTraits<ColumnType::Int8Arr> {});
		break;
	case ColumnType::Int16Arr:
		func(DataTypeTraits<ColumnType::Int16Arr> {});
		break;
	case ColumnType::Int32Arr:
		func(DataTypeTraits<ColumnType::Int32Arr> {});
		break;
	case ColumnType::Int64Arr:
		func(DataTypeTraits<ColumnType::Int64Arr> {});
		break;
	case ColumnType::UInt8Arr:
		func(DataTypeTraits<ColumnType::UInt8Arr> {});
		break;
	case ColumnType::UInt16Arr:
		func(DataTypeTraits<ColumnType::UInt16Arr> {});
		break;
	case ColumnType::UInt32Arr:
		func(DataTypeTraits<ColumnType::UInt32Arr> {});
		break;
	case ColumnType::UInt64Arr:
		func(DataTypeTraits<ColumnType::UInt64Arr> {});
		break;
	case ColumnType::CharArr:
		func(DataTypeTraits<ColumnType::CharArr> {});
		break;
	case ColumnType::FloatArr:
		func(DataTypeTraits<ColumnType::FloatArr> {});
		break;
	case ColumnType::DoubleArr:
		func(DataTypeTraits<ColumnType::DoubleArr> {});
		break;
	case ColumnType::IpaddrArr:
		func(DataTypeTraits<ColumnType::IpaddrArr> {});
		break;
	case ColumnType::Macaddr:
		func(DataTypeTraits<ColumnType::Macaddr> {});
		break;
	case ColumnType::MacaddrArr:
		func(DataTypeTraits<ColumnType::MacaddrArr> {});
		break;
	case ColumnType::TimeArr:
		func(DataTypeTraits<ColumnType::TimeArr> {});
		break;
	case ColumnType::Bytes:
		func(DataTypeTraits<ColumnType::Bytes> {});
		break;
	default:
		throw std::runtime_error("invalid data type");
	}
}

inline bool is_arr(ColumnType type)
{
	return type < ColumnType::Int8;
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
