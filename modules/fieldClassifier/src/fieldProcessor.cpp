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
	if (TemplateCreator::s_activeModules.geolite || TemplateCreator::s_activeModules.asn) {
		try {
			m_geolite.init(m_params);
		} catch (const std::exception& ex) {
			throw std::runtime_error(std::string("Error while initializing Geolite: ") + ex.what());
		}
	}
	if (TemplateCreator::s_activeModules.ipClas) {
		try {
			m_ipClassifier.init(m_params);
		} catch (const std::exception& ex) {
			throw std::runtime_error(std::string("Error while initializing SNI: ") + ex.what());
		}
	}
	if (TemplateCreator::s_activeModules.sniClas) {
		try {
			m_sniClassifier.init(m_params);
		} catch (const std::exception& ex) {
			throw std::runtime_error(std::string("Error while initializing TLS SNI: ") + ex.what());
		}
	}
}

void FieldProcessor::exit()
{
	if (TemplateCreator::s_activeModules.geolite || TemplateCreator::s_activeModules.asn) {
		m_geolite.exit();
	}
	if (TemplateCreator::s_activeModules.ipClas) {
		m_ipClassifier.exit();
	}
	if (TemplateCreator::s_activeModules.sniClas) {
		m_sniClassifier.exit();
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

void FieldProcessor::getDataForOneDirection(Data& data, Nemea::IpAddress ipAddr)
{
	std::string ipStr = getIpString(ipAddr);

	if (TemplateCreator::s_activeModules.geolite) {
		m_geolite.getGeoData(data, ipStr.c_str());
	}
	if (TemplateCreator::s_activeModules.asn) {
		m_geolite.getASNData(data, ipStr.c_str());
	}
	if (TemplateCreator::s_activeModules.ipClas) {
		m_ipClassifier.checkForMatch(data, ipStr.c_str(), ipAddr.isIpv4());
	}
	if (TemplateCreator::s_activeModules.sniClas) {
		m_sniClassifier.checkForMatch(data, m_sni);
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
		if (!NLRUCache::LRUCache::get(ipStr, m_data_src)) {
			// if not get data from plugins and save to cache
			getDataForOneDirection(m_data_src, m_ipAddrSrc);
			NLRUCache::LRUCache::put(ipStr, m_data_src);
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

		if (!NLRUCache::LRUCache::get(ipStr, m_data_dst)) {
			getDataForOneDirection(m_data_dst, m_ipAddrDst);
			NLRUCache::LRUCache::put(ipStr, m_data_dst);
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
	// GEOLITE
	saveDataToUnirecField(unirecRecord, m_data_src.cityName, TemplateCreator::s_idsSrc.cityID);
	saveDataToUnirecField(
		unirecRecord,
		m_data_src.countryName,
		TemplateCreator::s_idsSrc.countryID);
	saveDataToUnirecField(unirecRecord, m_data_src.latitude, TemplateCreator::s_idsSrc.latitudeID);
	saveDataToUnirecField(
		unirecRecord,
		m_data_src.longitude,
		TemplateCreator::s_idsSrc.longitudeID);
	saveDataToUnirecField(
		unirecRecord,
		m_data_src.postalCode,
		TemplateCreator::s_idsSrc.postalCodeID);
	saveDataToUnirecField(
		unirecRecord,
		m_data_src.continentName,
		TemplateCreator::s_idsSrc.continentID);
	saveDataToUnirecField(unirecRecord, m_data_src.isoCode, TemplateCreator::s_idsSrc.isoCodeID);
	saveDataToUnirecField(unirecRecord, m_data_src.accuracy, TemplateCreator::s_idsSrc.accuracyID);

	saveDataToUnirecField(unirecRecord, m_data_dst.cityName, TemplateCreator::s_idsDst.cityID);
	saveDataToUnirecField(
		unirecRecord,
		m_data_dst.countryName,
		TemplateCreator::s_idsDst.countryID);
	saveDataToUnirecField(unirecRecord, m_data_dst.latitude, TemplateCreator::s_idsDst.latitudeID);
	saveDataToUnirecField(
		unirecRecord,
		m_data_dst.longitude,
		TemplateCreator::s_idsDst.longitudeID);
	saveDataToUnirecField(
		unirecRecord,
		m_data_dst.postalCode,
		TemplateCreator::s_idsDst.postalCodeID);
	saveDataToUnirecField(
		unirecRecord,
		m_data_dst.continentName,
		TemplateCreator::s_idsDst.continentID);
	saveDataToUnirecField(unirecRecord, m_data_dst.isoCode, TemplateCreator::s_idsDst.isoCodeID);
	saveDataToUnirecField(unirecRecord, m_data_dst.accuracy, TemplateCreator::s_idsDst.accuracyID);

	// ASN
	saveDataToUnirecField(unirecRecord, m_data_src.asn, TemplateCreator::s_idsSrc.asnID);
	saveDataToUnirecField(unirecRecord, m_data_src.asnOrg, TemplateCreator::s_idsSrc.asnOrgID);

	saveDataToUnirecField(unirecRecord, m_data_dst.asn, TemplateCreator::s_idsDst.asnID);
	saveDataToUnirecField(unirecRecord, m_data_dst.asnOrg, TemplateCreator::s_idsDst.asnOrgID);

	// IP_Classifier
	saveDataToUnirecField(unirecRecord, m_data_src.ipFlags, TemplateCreator::s_idsSrc.ipFlagsID);
	saveDataToUnirecField(unirecRecord, m_data_dst.ipFlags, TemplateCreator::s_idsDst.ipFlagsID);

	// SNI_Classifier
	saveDataToUnirecField(unirecRecord, m_data_src.company, TemplateCreator::s_idsSrc.companyID);
	saveDataToUnirecField(unirecRecord, m_data_src.sniFlags, TemplateCreator::s_idsSrc.sniFlagsID);
	saveDataToUnirecField(unirecRecord, m_data_dst.company, TemplateCreator::s_idsDst.companyID);
	saveDataToUnirecField(unirecRecord, m_data_dst.sniFlags, TemplateCreator::s_idsDst.sniFlagsID);
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
		readFieldString(unirecRecord, "SRC_CITY_NAME", m_data_src.cityName.size());
		readFieldString(unirecRecord, "SRC_COUNTRY_NAME", m_data_src.countryName.size());
		readFieldDouble(unirecRecord, "SRC_LATITUDE");
		readFieldDouble(unirecRecord, "SRC_LONGITUDE");
		readFieldString(unirecRecord, "SRC_POSTAL_CODE", m_data_src.postalCode.size());
		readFieldString(unirecRecord, "SRC_CONTINENT_NAME", m_data_src.continentName.size());
		readFieldString(unirecRecord, "SRC_ISO_CODE", m_data_src.isoCode.size());
		readFieldString(unirecRecord, "SRC_ASO", m_data_src.asnOrg.size());
		readFieldInt(unirecRecord, "SRC_ASN");
		readFieldInt(unirecRecord, "SRC_ACCURACY");
		readFieldString(unirecRecord, "SRC_IP_FLAGS", m_data_src.ipFlags.size());
		readFieldString(unirecRecord, "SRC_COMPANY", m_data_src.company.size());
		readFieldString(unirecRecord, "SRC_SNI_FLAGS", m_data_src.sniFlags.size());
		std::cout << "##############################" << '\n';
	}
	if (m_params.traffic == Direction::BOTH || m_params.traffic == Direction::DESTINATION) {
		std::cout << "##############################" << '\n';
		std::cout << "Data for DST_IP: " << getIpString(m_ipAddrDst) << '\n';
		std::cout << "SNI content:" << m_sni << '\n';
		readFieldString(unirecRecord, "DST_CITY_NAME", m_data_dst.cityName.size());
		readFieldString(unirecRecord, "DST_COUNTRY_NAME", m_data_dst.countryName.size());
		readFieldDouble(unirecRecord, "DST_LATITUDE");
		readFieldDouble(unirecRecord, "DST_LONGITUDE");
		readFieldString(unirecRecord, "DST_POSTAL_CODE", m_data_dst.postalCode.size());
		readFieldString(unirecRecord, "DST_CONTINENT_NAME", m_data_dst.continentName.size());
		readFieldString(unirecRecord, "DST_ISO_CODE", m_data_dst.isoCode.size());
		readFieldString(unirecRecord, "DST_ASO", m_data_dst.asnOrg.size());
		readFieldInt(unirecRecord, "DST_ASN");
		readFieldInt(unirecRecord, "DST_ACCURACY");
		readFieldString(unirecRecord, "DST_IP_FLAGS", m_data_dst.ipFlags.size());
		readFieldString(unirecRecord, "DST_COMPANY", m_data_dst.company.size());
		readFieldString(unirecRecord, "DST_SNI_FLAGS", m_data_dst.sniFlags.size());
		std::cout << "##############################" << '\n';
	}
}

} // namespace NFieldProcessor
//
