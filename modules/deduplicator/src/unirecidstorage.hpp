/**
 * @file
 * @author Damir Zainullin <zaidamilda@gmail.com>
 * @brief Declaration of the UnirecIdStorage structure (Unirec field ids required by export process)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <unirec++/unirecRecordView.hpp>

namespace Deduplicator {

/**
 * @brief Structure keeps all Unirec ids required by deduplicator.
 */
struct UnirecIdStorage {
	ur_field_id_t srcIpId; ///< Unirec ID of source ip.
	ur_field_id_t dstIpId; ///< Unirec ID of destination ip.
	ur_field_id_t srcPortId; ///< Unirec ID of source port.
	ur_field_id_t dstPortId; ///< Unirec ID of destination port.
	ur_field_id_t linkBitFieldId; ///< Unirec ID of link bit field.
	ur_field_id_t protocolId; ///< Unirec ID of protocol field.
	ur_field_id_t timeLastId; ///< Unirec ID of last packet timestamp.
};

} // namespace Deduplicator
