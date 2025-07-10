/**
 * @file normalizer.hpp
 * @author Jaroslav Pesek <jaroslav.pesek@fit.cvut.cz>
 * @brief Declaration of the ADS Normalizer class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unirec++/unirec.hpp>
#include <sstream>
#include <stdexcept>


namespace AdsNorm {

using namespace Nemea;

struct AdsNormalizerStats {
    uint64_t totalRecords = 0; ///< Total number of records processed.
    uint64_t rotatedRecords = 0; ///< Number of records that were rotated.
    uint64_t nonRotatedRecords = 0; ///< Number of records that were not rotated.
};


/**
 * @brief A class for normalizing ADS records.
 *
 * This class provides functionality to normalize ADS records by rotating them
 * based on PREFIX_TAG or PREFIX_TAG_DST.
 * If PREFIX_TAG == 0 and PREFIX_TAG_DST == 0, the unirec is dropped and info is logged as unexpected situation.
 * If PREFIX_TAG != 0 and PREFIX_TAG_DST == 0, the rotatable columns are NOT rotated and the unirec is send as is.
 * If PREFIX_TAG == 0 and PREFIX_TAG_DST != 0, the rotatable columns ARE rotated and the unirec is send with the rotated columns.
 * If PREFIX_TAG != 0 and PREFIX_TAG_DST != 0, the unirec is send 2 times: rotated and non-rotated.
 */
class AdsNormalizer {
public:
    /**
     * @brief Constructs an AdsNormalizer object with the given template string.
     * @param templateStr The template string for the normalization - strings containing the column names to be rotated.
     *                    The template string should contain pairs of column names separated by a comma.
     *                    For example: "ipaddr SRC_IP,ipaddr DST_IP,uint16 SRC_PORT,uint16 DST_PORT,uint32 BYTES,uint32 BYTES_REV".
     */
    explicit AdsNormalizer(const std::string& templateStr);

    /**
     * @brief Rotates the columns of the given record based on the template string.
     * @param record The UnirecRecordView object representing the record to be normalized.
     *
     * This function modifies the record in place by rotating the specified columns
     * according to the template string provided during construction.
     */
    UnirecRecord swapColumnPairs(const UnirecRecordView& record);

    std::optional<UnirecRecordView> sendOriginal(UnirecRecordView& record);

    std::optional<UnirecRecord> sendRotated(UnirecRecordView& record);

    void updateUnirec(UnirecRecord& new_record);

private:
    std::vector<std::pair<std::string, std::string>> m_str_fields; ///< Template string for the normalization.
    std::vector<std::pair<ur_field_id_t, ur_field_id_t>> m_id_fields; ///< Vector of pairs of column names to be rotated.
    AdsNormalizerStats m_stats; ///< Statistics of the normalization process.
    ur_field_id_t m_prefix_tag_id; ///< Unirec ID for PREFIX_TAG field.
    ur_field_id_t m_prefix_tag_dst_id; ///< Unirec ID for PREFIX_TAG_DST field.
    UnirecRecord to_send_record;
};






} // namespace AdsNorm
