#pragma once
#include "commandLineParams.hpp"
#include "geolite.hpp"
#include <optional>
#include <unirec++/unirec.hpp>
#include <unirec++/unirecRecord.hpp>

namespace NFieldProcessor {

struct UnirecIDFields {
	ur_field_id_t srcCityID;
	ur_field_id_t srcCountryID;
	ur_field_id_t srcLatitudeID;
	ur_field_id_t srcLongitudeID;
	ur_field_id_t srcPostalCodeID;
	ur_field_id_t dstCityID;
	ur_field_id_t dstCountryID;
	ur_field_id_t dstLatitudeID;
	ur_field_id_t dstLongitudeID;
	ur_field_id_t dstPostalCodeID;
};

struct Data {
	std::string srcCityName;
	std::string srcCountryName;
	double srcLatitude;
	double srcLongitude;
	std::string srcPostalCode;
	std::string dstCityName;
	std::string dstCountryName;
	double dstLatitude;
	double dstLongitude;
	std::string dstPostalCode;
};

class FieldProcessor {
public:
	void init();
	void exit();

	/**
	 * @brief Save Unirec geolocation field IDs to structure.
	 */
	void getUnirecRecordFieldIDs();

	/**
	 * @brief Read data from MaxMind data and saves them to structure.
	 */
	void getDataForUnirecRecord();

	/**
	 * @brief Populates Unirec geolocation fields with data from MaxMind DB
	 */
	void setDataToUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;

	/**
	 * @brief Gets IP address from Unirec record and saves it to Geolite object.
	 *
	 * @param inputUnirecView - Unirec record received from trap interface
	 */
	void getIp(std::optional<Nemea::UnirecRecordView>& inputUnirecView);

	/**
	 * @brief saves commandline parameters to Geolite object
	 *
	 * @param params CommandLineParameters object containing command line parameters
	 */
	void setParameters(const NFieldProcessor::CommandLineParameters& params);
	// Set direction of communication

	/**
	 * @brief Returns set communication direction (bidirectional by default)
	 *
	 */
	Direction getDirection();

	// FOR TESTING ################################

	/**
	 * @brief Prints content of geolocation fields on std output
	 *
	 * @param unirecRecord record to use for printing
	 */
	void printUnirecRecord(Nemea::UnirecRecord& unirecRecord) const;

private:
	UnirecIDFields m_ids; // Unirec field IDs
	Data m_data; // Data to be saved to Unirec record

	Geolite::Geolite m_geolite;
	CommandLineParameters m_params;

	Nemea::IpAddress m_ipAddrSrc; // Source IP address from Unirec record
	Nemea::IpAddress m_ipAddrDst; // Destination IP address from Unirec record

	void saveIpAddress(
		const std::string& ipField,
		std::optional<Nemea::UnirecRecordView>& inputUnirecView,
		Nemea::IpAddress& ipAddr);

	/**
	 * @brief Methods to retreve IP address from Geolite object
	 *
	 * @return IpAddress object containing the IP address.
	 */

	// Helper function for returninig ip as string
	char* getIpString(Nemea::IpAddress) const;

	ur_field_id_t getUnirecFieldID(const char* name); //

	template <typename T>
	void
	saveDataToUnirecField(Nemea::UnirecRecord& unirecRecord, const T& data, ur_field_id_t idField)
		const
	{
		if (idField != UR_E_INVALID_NAME) {
			unirecRecord.setFieldFromType(data, idField);
		}
	}

	// TESTING
	void readFieldDouble(Nemea::UnirecRecord& unirecRecord, const char* name) const;
	void readFieldString(Nemea::UnirecRecord& unirecRecord, const char* name) const;
};
} // namespace NFieldProcessor
