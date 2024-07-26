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

struct UnirecIdStorage {
	ur_field_id_t srcIpId;
	ur_field_id_t dstIpId;
	ur_field_id_t srcPortId;
	ur_field_id_t dstPortId;
	ur_field_id_t linkBitFieldId;
	ur_field_id_t protocolId;
	ur_field_id_t timeLastId;
};

} // namespace Deduplicator
