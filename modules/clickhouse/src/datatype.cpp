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

namespace Getters {

template <typename Value>
static Value getValue(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Value value = record.getFieldAsType<Value>(fieldID);
	return value;
}

template <typename Value>
static std::vector<Value> getValueArr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UnirecArray<Value> const arr = record.getFieldAsUnirecArray<Value>(fieldID);
	std::vector<Value> result;
	result.reserve(arr.size());
	for (const auto& value : arr) {
		result.push_back(value);
	}
	return result;
}

static std::vector<uint8_t> getBytes(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UnirecArray<std::byte> const arr = record.getFieldAsUnirecArray<std::byte>(fieldID);
	std::vector<uint8_t> result;
	result.reserve(arr.size());
	for (const auto& value : arr) {
		result.push_back((char) value);
	}
	return result;
}

static in6_addr getIp(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::IpAddress addr = record.getFieldAsType<Nemea::IpAddress>(fieldID);
	return *((in6_addr*) &addr.ip);
}

static std::vector<in6_addr> getIpArr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UnirecArray<Nemea::IpAddress> const addrArr
		= record.getFieldAsUnirecArray<Nemea::IpAddress>(fieldID);
	std::vector<in6_addr> result;
	result.reserve(addrArr.size());
	for (const auto& value : addrArr) {
		result.push_back(*((in6_addr*) &value.ip));
	}
	return result;
}

static std::vector<uint8_t> getMac(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::MacAddress const mac = record.getFieldAsType<Nemea::MacAddress>(fieldID);
	std::vector<uint8_t> result;
	const int nMacBytes = 6;
	result.reserve(nMacBytes);
	for (const unsigned char byte : mac.mac.bytes) {
		result.push_back(byte);
	}
	return result;
}

static std::vector<std::vector<uint8_t>>
getMacArr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UnirecArray<Nemea::MacAddress> const macArr
		= record.getFieldAsUnirecArray<Nemea::MacAddress>(fieldID);
	std::vector<std::vector<uint8_t>> result;
	result.reserve(macArr.size());
	for (const auto& value : macArr) {
		result.emplace_back();
		for (const unsigned char byte : value.mac.bytes) {
			result.back().push_back(byte);
		}
	}
	return result;
}

static uint64_t getTime(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UrTime const time = record.getFieldAsType<Nemea::UrTime>(fieldID);
	const uint64_t nsecInSec = 1000000000;
	return (
		(static_cast<uint64_t>(ur_time_get_sec(time.time)) * nsecInSec)
		+ static_cast<uint64_t>(ur_time_get_nsec(time.time)));
}

static std::vector<uint64_t> getTimeArr(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	Nemea::UnirecArray<Nemea::UrTime> const timeArr
		= record.getFieldAsUnirecArray<Nemea::UrTime>(fieldID);
	std::vector<uint64_t> result;
	result.reserve(timeArr.size());
	const uint64_t nsecInSec = 1000000000;
	for (const auto& value : timeArr) {
		result.push_back(
			(static_cast<uint64_t>(ur_time_get_sec(value.time)) * nsecInSec)
			+ static_cast<uint64_t>(ur_time_get_nsec(value.time)));
	}
	return result;
}

static std::string getString(Nemea::UnirecRecordView& record, ur_field_id_t fieldID)
{
	std::string str = record.getFieldAsType<std::string>(fieldID);
	return str;
}
} // namespace Getters

template <ColumnType>
struct DataTypeTraits {};

template <>
struct DataTypeTraits<ColumnType::UINT8> {
	using ColumnType = clickhouse::ColumnUInt8;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "UInt8";
	static constexpr auto GETTER = &Getters::getValue<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT16> {
	using ColumnType = clickhouse::ColumnUInt16;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "UInt16";
	static constexpr auto GETTER = &Getters::getValue<uint16_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT32> {
	using ColumnType = clickhouse::ColumnUInt32;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "UInt32";
	static constexpr auto GETTER = &Getters::getValue<uint32_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT64> {
	using ColumnType = clickhouse::ColumnUInt64;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "UInt64";
	static constexpr auto GETTER = &Getters::getValue<uint64_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT8> {
	using ColumnType = clickhouse::ColumnInt8;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Int8";
	static constexpr auto GETTER = &Getters::getValue<int8_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT16> {
	using ColumnType = clickhouse::ColumnInt16;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Int16";
	static constexpr auto GETTER = &Getters::getValue<int16_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT32> {
	using ColumnType = clickhouse::ColumnInt32;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Int32";
	static constexpr auto GETTER = &Getters::getValue<int32_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT64> {
	using ColumnType = clickhouse::ColumnInt64;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Int64";
	static constexpr auto GETTER = &Getters::getValue<int64_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT8_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(UInt8)";
	static constexpr auto GETTER = &Getters::getValueArr<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT16_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt16>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(UInt16)";
	static constexpr auto GETTER = &Getters::getValueArr<uint16_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT32_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt32>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(UInt32)";
	static constexpr auto GETTER = &Getters::getValueArr<uint32_t>;
};

template <>
struct DataTypeTraits<ColumnType::UINT64_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt64>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(UInt64)";
	static constexpr auto GETTER = &Getters::getValueArr<uint64_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT8_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt8>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(Int8)";
	static constexpr auto GETTER = &Getters::getValueArr<int8_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT16_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt16>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(Int16)";
	static constexpr auto GETTER = &Getters::getValueArr<int16_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT32_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt32>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(Int32)";
	static constexpr auto GETTER = &Getters::getValueArr<int32_t>;
};

template <>
struct DataTypeTraits<ColumnType::INT64_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnInt64>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(Int64)";
	static constexpr auto GETTER = &Getters::getValueArr<int64_t>;
};

template <>
struct DataTypeTraits<ColumnType::CHAR> {
	using ColumnType = clickhouse::ColumnUInt8;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "UInt8";
	static constexpr auto GETTER = &Getters::getValue<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::CHAR_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(UInt8)";
	static constexpr auto GETTER = &Getters::getValueArr<uint8_t>;
};

template <>
struct DataTypeTraits<ColumnType::FLOAT> {
	using ColumnType = clickhouse::ColumnFloat32;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Float32";
	static constexpr auto GETTER = &Getters::getValue<float>;
};

template <>
struct DataTypeTraits<ColumnType::FLOAT_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnFloat32>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(Float32)";
	static constexpr auto GETTER = &Getters::getValueArr<float>;
};

template <>
struct DataTypeTraits<ColumnType::DOUBLE> {
	using ColumnType = clickhouse::ColumnFloat64;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Float64";
	static constexpr auto GETTER = &Getters::getValue<double>;
};

template <>
struct DataTypeTraits<ColumnType::DOUBLE_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnFloat64>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(Float64)";
	static constexpr auto GETTER = &Getters::getValueArr<double>;
};

template <>
struct DataTypeTraits<ColumnType::IPADDR> {
	using ColumnType = clickhouse::ColumnIPv6;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "IPv6";
	static constexpr auto GETTER = &Getters::getIp;
};

template <>
struct DataTypeTraits<ColumnType::IPADDR_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnIPv6>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(IPv6)";
	static constexpr auto GETTER = &Getters::getIpArr;
};

template <>
struct DataTypeTraits<ColumnType::MACADDR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(UInt8)";
	static constexpr auto GETTER = &Getters::getMac;
};

template <>
struct DataTypeTraits<ColumnType::MACADDR_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(Array(UInt8))";
	static constexpr auto GETTER = &Getters::getMacArr;
};

template <>
struct DataTypeTraits<ColumnType::TIME> {
	using ColumnType = ColumnDateTime64<g_TIME_PRECISION>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "DateTime64(9)";
	static constexpr auto GETTER = &Getters::getTime;
};

template <>
struct DataTypeTraits<ColumnType::TIME_ARR> {
	using ColumnType = clickhouse::ColumnArrayT<ColumnDateTime64<g_TIME_PRECISION>>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(DateTime64(9))";
	static constexpr auto GETTER = &Getters::getTimeArr;
};

template <>
struct DataTypeTraits<ColumnType::STRING> {
	using ColumnType = clickhouse::ColumnString;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "String";
	static constexpr auto GETTER = &Getters::getString;
};

template <>
struct DataTypeTraits<ColumnType::BYTES> {
	using ColumnType = clickhouse::ColumnArrayT<clickhouse::ColumnUInt8>;
	static constexpr std::string_view CLICKHOUSE_TYPE_NAME = "Array(UInt8)";
	static constexpr auto GETTER = &Getters::getBytes;
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
static void visitNonArr(ColumnType type, Func func)
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
static void visitArr(ColumnType type, Func func)
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

inline bool isArr(ColumnType type)
{
	return type < ColumnType::INT8;
}

std::shared_ptr<clickhouse::Column> makeArrColumn(ColumnType type)
{
	std::shared_ptr<clickhouse::Column> column;
	visit(type, [&](auto traits) {
		using ColType = typename decltype(traits)::ColumnType;
		column = std::make_shared<ColType>();
	});

	return column;
}

std::shared_ptr<clickhouse::Column> makeNonArrColumn(ColumnType type)
{
	std::shared_ptr<clickhouse::Column> column;
	visit(type, [&](auto traits) {
		using ColType = clickhouse::ColumnNullableT<typename decltype(traits)::ColumnType>;
		column = std::make_shared<ColType>();
	});

	return column;
}

std::shared_ptr<clickhouse::Column> makeColumn(ColumnType type)
{
	if (isArr(type)) {
		return makeArrColumn(type);
	}
	return makeNonArrColumn(type);
}

GetterFn makeGetter(ColumnType type)
{
	GetterFn getter;
	visit(type, [&](auto traits) {
		getter = [](Nemea::UnirecRecordView& record, ur_field_id_t fieldID, ValueVariant& value) {
			value = decltype(traits)::GETTER(record, fieldID);
		};
	});
	return getter;
}

ColumnWriterFn makeArrColumnwriter(ColumnType type)
{
	ColumnWriterFn columnwriter;

	visitArr(type, [&](auto traits) {
		columnwriter = [](ValueVariant* value, clickhouse::Column& column) {
			using ColumnType = typename decltype(traits)::ColumnType;
			using ValueType = std::invoke_result_t<
				decltype(decltype(traits)::GETTER),
				Nemea::UnirecRecordView&,
				ur_field_type_t>;
			auto* col = dynamic_cast<ColumnType*>(&column);
			if (value) {
				col->Append(std::get<ValueType>(*value));
			}
		};
	});

	return columnwriter;
}

ColumnWriterFn makeNonArrColumnwriter(ColumnType type)
{
	ColumnWriterFn columnwriter;

	visitNonArr(type, [&](auto traits) {
		columnwriter = [](ValueVariant* value, clickhouse::Column& column) {
			using ColumnType = clickhouse::ColumnNullableT<typename decltype(traits)::ColumnType>;
			using ValueType = std::invoke_result_t<
				decltype(decltype(traits)::GETTER),
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

	return columnwriter;
}

ColumnWriterFn makeColumnwriter(ColumnType type)
{
	if (isArr(type)) {
		return makeArrColumnwriter(type);
	}
	return makeNonArrColumnwriter(type);
}

std::string typeToClickhouse(ColumnType type)
{
	std::string result;
	visit(type, [&](auto traits) { result = traits.CLICKHOUSE_TYPE_NAME; });
	return result;
}
