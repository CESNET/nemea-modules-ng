/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Declaration of the Sampler class.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <cstdint>

namespace Sampler {

/**
 * @brief Structure to hold sampling statistics.
 */
struct SamplerStats {
	uint64_t sampledRecords = 0; ///< Number of sampled records.
	uint64_t totalRecords = 0;  ///< Total number of records.
};

/**
 * @brief A class for sampling records at a given 1:r rate.
 */
class Sampler {
public:
	/**
	 * @brief Constructs a Sampler object with the given sampling rate.
	 * @param samplingRate The 1:r rate at which records should be sampled.
	 */
	explicit Sampler(std::size_t samplingRate);

	/**
	 * @brief Determines whether the current record should be sampled.
	 *
	 * This function increments the total records counter and checks if the current record
	 * should be sampled based on the sampling rate.
	 *
	 * Every -rth record will be sampled.
	 *
	 * @return True if the current record should be sampled, false otherwise.
	 */
	bool shouldBeSampled() noexcept;

	/**
	 * @brief Returns the current sampling statistics.
	 * @return The current sampling statistics.
	 */
	SamplerStats getStats() const noexcept;

private:
	const std::size_t M_SAMPLING_RATE;
	uint64_t m_totalRecords = 0;
	uint64_t m_sampledRecords = 0;
};

} // namespace Sampler
