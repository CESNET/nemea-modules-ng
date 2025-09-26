/**
 * @file fieldProcessor.cpp
 * @author Tomáš Vrána <xvranat00@vutbr.cz>
 * @brief fieldProcessor class impolementation
 *
 * This class is responsible for processing Unirec records and adding new fields with data to them
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fieldProcessor.hpp"
#include "LRUCache.hpp"
#include "common.hpp"
#include "templateCreator.hpp"
#include <cstdint>
#include <iostream>
#include <unirec++/ipAddress.hpp>
#include <unirec/unirec.h>

namespace NFieldProcessor {

void FieldProcessor::init()
{
	createPlugins();

	for (auto& plugin : m_plugins) {
		try {
			plugin->init(m_params);
		} catch (const std::exception& ex) {
			throw std::runtime_error(std::string("Error while initializing plugin: ") + ex.what());
		}
	}
}

void FieldProcessor::exit()
{
	for (auto& plugin : m_plugins) {
		plugin->exit();
	}
}
void FieldProcessor::setParameters(const CommandLineParameters& params)
{
	m_params = params;
}

std::string FieldProcessor::getIpString(Nemea::IpAddress ipAddr) const
{
	if (ipAddr.isIpv4()) {
		static char str[16];
		snprintf(
			str,
			sizeof(str),
			"%u.%u.%u.%u",
			ipAddr.ip.bytes[8],
			ipAddr.ip.bytes[9],
			ipAddr.ip.bytes[10],
			ipAddr.ip.bytes[11]);
		return {str};
	}

	if (ipAddr.isIpv6()) {
		static char str[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &ipAddr.ip, str, sizeof(str));
		return {str};
	}
	throw std::runtime_error("Invalid IP address type");
}

void FieldProcessor::getDataForOneDirection(FieldsMap& data, Nemea::IpAddress ipAddr)
{
	PluginData pluginData;
	pluginData.ipAddr = getIpString(ipAddr);
	pluginData.field = m_sni;
	pluginData.isIpv4 = ipAddr.isIpv4();

	for (auto& plugin : m_plugins) {
		plugin->getData(data, pluginData);
	}
}

void FieldProcessor::getDataForUnirecRecord(std::optional<Nemea::UnirecRecordView>& inputUnirecView)
{
	if (TemplateCreator::s_activeModules.sniClas) {
		saveSNI(TemplateCreator::s_idsGen.sniID, inputUnirecView, m_sni);
	}

	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::SOURCE) {
		// get ip from Unirec record
		try {
			saveIpAddress(TemplateCreator::s_idsGen.srcIPID, inputUnirecView, m_ipAddrSrc);
		} catch (const std::exception& ex) {
			throw std::runtime_error(
				std::string("Error while getting source IP address: ") + ex.what());
			return;
		}

		// save it as a string
		std::string ipStr = getIpString(m_ipAddrSrc);

		debugPrint("------------------------------------------------", 2);
		debugPrint("Processing IP: " + ipStr, 2);

		// check if this ip is in cache
		if (!NLRUCache::LRUCache::get(ipStr, TemplateCreator::s_allFields)) {
			// if not get data from plugins and save to cache
			getDataForOneDirection(TemplateCreator::s_allFields[DirIndex::SRC], m_ipAddrSrc);
			NLRUCache::LRUCache::put(ipStr, TemplateCreator::s_allFields);
			debugPrint("Cache miss", 2);
		} else {
			debugPrint("Cache hit", 2);
		}
		debugPrint("------------------------------------------------", 2);
	}
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::DESTINATION) {
		try {
			saveIpAddress(TemplateCreator::s_idsGen.dstIPID, inputUnirecView, m_ipAddrDst);
		} catch (const std::exception& ex) {
			throw std::runtime_error(
				std::string("Error while getting destination IP address: ") + ex.what());
			return;
		}

		std::string ipStr = getIpString(m_ipAddrDst);

		debugPrint("------------------------------------------------", 2);
		debugPrint("Processing IP: " + ipStr, 2);

		if (!NLRUCache::LRUCache::get(ipStr, TemplateCreator::s_allFields)) {
			getDataForOneDirection(TemplateCreator::s_allFields[DirIndex::DST], m_ipAddrDst);
			NLRUCache::LRUCache::put(ipStr, TemplateCreator::s_allFields);
			debugPrint("Cache miss", 2);
		} else {
			debugPrint("Cache hit", 2);
		}
		debugPrint("------------------------------------------------", 2);
	}
	debugPrint("TSL_SNI content:" + m_sni, 2);
}

void FieldProcessor::setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const
{
	for (const auto& [fieldName, field] : TemplateCreator::s_allFields[DirIndex::SRC]) {
		try {
			saveDataToUnirecField(unirecRecord, field.data, field.id);
		} catch (const std::exception& ex) {
			throw std::runtime_error("Unexpected variant type in field: " + fieldName);
		}
	}
	for (const auto& [fieldName, field] : TemplateCreator::s_allFields[DirIndex::DST]) {
		try {
			saveDataToUnirecField(unirecRecord, field.data, field.id);
		} catch (const std::exception& ex) {
			throw std::runtime_error("Unexpected variant type in field: " + fieldName);
		}
	}
}

void FieldProcessor::saveIpAddress(
	const ur_field_id_t& ipID,
	std::optional<Nemea::UnirecRecordView>& inputUnirecView,
	Nemea::IpAddress& ipAddr)
{
	if (ipID == UR_E_INVALID_NAME) {
		throw std::runtime_error(
			std::string("Name/s for Unirec IP fields not found in Unirec communication"));
		return;
	}
	try {
		ipAddr = inputUnirecView->getFieldAsType<Nemea::IpAddress>(ipID);
	} catch (const std::exception& ex) {
		throw;
	}
}
void FieldProcessor::saveSNI(
	const ur_field_id_t& sniID,
	std::optional<Nemea::UnirecRecordView>& inputUnirecView,
	std::string& sni)
{
	if (sniID == UR_E_INVALID_NAME) {
		sni = "";
		debugPrint("SNI field not found in Unirec record", 2);
		return;
	}
	try {
		sni = inputUnirecView->getFieldAsType<std::string>(sniID);
	} catch (const std::exception& ex) {
		sni = "";
		debugPrint("Unable to get SNI field from Unirec record", 1);
	}
}

// TESTING ##############################

void FieldProcessor::readFieldDouble(Nemea::UnirecRecord& unirecRecord, const char* name) const
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		return;
	}
	auto data = unirecRecord.getFieldAsType<double>(idField);
	std::cout << name << ": " << data << '\n';
}

void FieldProcessor::readFieldString(
	Nemea::UnirecRecord& unirecRecord,
	const char* name,
	unsigned long size) const
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		return;
	}
	const auto* data = unirecRecord.getFieldAsType<const char*>(idField);

	std::cout << name << ": " << std::string(data, size) << '\n';
}
void FieldProcessor::readFieldInt(Nemea::UnirecRecord& unirecRecord, const char* name) const
{
	auto idField = static_cast<ur_field_id_t>(ur_get_id_by_name(name));
	if (idField == UR_E_INVALID_NAME) {
		return;
	}
	auto data = unirecRecord.getFieldAsType<uint16_t>(idField);
	std::cout << name << ": " << data << '\n';
}

void FieldProcessor::printUnirecRecord(Nemea::UnirecRecord& unirecRecord) const
{
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::SOURCE) {
		std::cout << "##############################" << '\n';
		std::cout << "Data for SRC_IP: " << getIpString(m_ipAddrSrc) << '\n';
		std::cout << "SNI content:" << m_sni << '\n';
		readFieldString(
			unirecRecord,
			"SRC_CITY_NAME",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::SRC].at("CITY_NAME").data)
				.size());
		readFieldString(
			unirecRecord,
			"SRC_COUNTRY_NAME",
			std::get<std::string>(
				TemplateCreator::s_allFields[DirIndex::SRC].at("COUNTRY_NAME").data)
				.size());
		readFieldDouble(unirecRecord, "SRC_LATITUDE");
		readFieldDouble(unirecRecord, "SRC_LONGITUDE");
		readFieldString(
			unirecRecord,
			"SRC_POSTAL_CODE",
			std::get<std::string>(
				TemplateCreator::s_allFields[DirIndex::SRC].at("POSTAL_CODE").data)
				.size());
		readFieldString(
			unirecRecord,
			"SRC_CONTINENT_NAME",
			std::get<std::string>(
				TemplateCreator::s_allFields[DirIndex::SRC].at("CONTINENT_NAME").data)
				.size());
		readFieldString(
			unirecRecord,
			"SRC_ISO_CODE",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::SRC].at("ISO_CODE").data)
				.size());
		readFieldString(
			unirecRecord,
			"SRC_ASO",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::SRC].at("ASO").data)
				.size());
		readFieldInt(unirecRecord, "SRC_ASN");
		readFieldInt(unirecRecord, "SRC_ACCURACY");
		readFieldString(
			unirecRecord,
			"SRC_IP_FLAGS",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::SRC].at("IP_FLAGS").data)
				.size());
		readFieldString(
			unirecRecord,
			"SRC_COMPANY",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::SRC].at("COMPANY").data)
				.size());
		readFieldString(
			unirecRecord,
			"SRC_SNI_FLAGS",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::SRC].at("SNI_FLAGS").data)
				.size());
		std::cout << "##############################" << '\n';
	}
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::DESTINATION) {
		std::cout << "##############################" << '\n';
		std::cout << "Data for DST_IP: " << getIpString(m_ipAddrDst) << '\n';
		std::cout << "SNI content:" << m_sni << '\n';
		readFieldString(
			unirecRecord,
			"DST_CITY_NAME",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::DST].at("CITY_NAME").data)
				.size());
		readFieldString(
			unirecRecord,
			"DST_COUNTRY_NAME",
			std::get<std::string>(
				TemplateCreator::s_allFields[DirIndex::DST].at("COUNTRY_NAME").data)
				.size());
		readFieldDouble(unirecRecord, "DST_LATITUDE");
		readFieldDouble(unirecRecord, "DST_LONGITUDE");
		readFieldString(
			unirecRecord,
			"DST_POSTAL_CODE",
			std::get<std::string>(
				TemplateCreator::s_allFields[DirIndex::DST].at("POSTAL_CODE").data)
				.size());
		readFieldString(
			unirecRecord,
			"DST_CONTINENT_NAME",
			std::get<std::string>(
				TemplateCreator::s_allFields[DirIndex::DST].at("CONTINENT_NAME").data)
				.size());
		readFieldString(
			unirecRecord,
			"DST_ISO_CODE",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::DST].at("ISO_CODE").data)
				.size());
		readFieldString(
			unirecRecord,
			"DST_ASO",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::DST].at("ASO").data)
				.size());
		readFieldInt(unirecRecord, "DST_ASN");
		readFieldInt(unirecRecord, "DST_ACCURACY");
		readFieldString(
			unirecRecord,
			"DST_IP_FLAGS",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::DST].at("IP_FLAGS").data)
				.size());
		readFieldString(
			unirecRecord,
			"DST_COMPANY",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::DST].at("COMPANY").data)
				.size());
		readFieldString(
			unirecRecord,
			"DST_SNI_FLAGS",
			std::get<std::string>(TemplateCreator::s_allFields[DirIndex::DST].at("SNI_FLAGS").data)
				.size());
		std::cout << "##############################" << '\n';
	}
}

} // namespace NFieldProcessor
//
