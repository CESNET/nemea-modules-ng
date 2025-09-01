/**
 * @file
 * @author Pavel Siska <siska@cesnet.cz>
 * @brief Core rule-matching structures and base matcher interface.
 *
 * This file defines the fundamental data structures for representing rule sets,
 * matching states, and the abstract interface @ref ListDetector::RuleFieldMatcher
 * for implementing field-specific matchers.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <boost/dynamic_bitset.hpp>
#include <cstdint>
#include <unirec++/unirec.hpp>

namespace ListDetector {

/**
 * @brief Identifier type for rules.
 *
 * Represents the index of a rule in the rule set.
 */
using RuleID = std::size_t;

/**
 * @brief Bitset type for tracking matched rules.
 *
 * Each bit corresponds to a rule. A set bit means that the rule
 * is currently matched, while an unset bit means it is not.
 */
using RuleBitset = boost::dynamic_bitset<>;

/**
 * @brief Represents the set of rules matched during evaluation.
 *
 * This class manages the state of matched rules across multiple field matchers.
 * It supports incremental merging of results, ensuring that rules must match
 * all participating fields in order to remain matched.
 *
 * @note The first merge initializes the internal bitset, subsequent merges
 *       intersect with it (logical AND).
 */
/**
 * @brief Represents the set of rules matched during evaluation.
 *
 * This class manages the state of matched rules across multiple field matchers.
 * It supports incremental merging of results, ensuring that rules must match
 * all participating fields in order to remain matched.
 *
 * @note The first merge initializes the internal bitset, subsequent merges
 *       intersect with it (logical AND).
 */
class RuleMatchSet {
public:
	/**
	 * @brief Resizes the underlying bitset to support a new number of rules.
	 *
	 * @param newSize Number of rules.
	 */
	void resize(std::size_t newSize)
	{
		m_matchedRules.resize(newSize);
		m_isInitialized = false;
	}

	/**
	 * @brief Checks whether no rules are matched.
	 *
	 * @return @c true if no rules are matched, otherwise @c false.
	 */
	bool none() const { return m_matchedRules.none(); }

	/**
	 * @brief Tests whether a given rule ID is matched.
	 *
	 * @param ruleID Rule identifier.
	 * @return @c true if the rule is matched, otherwise @c false.
	 */
	bool test(std::size_t ruleID) const { return m_matchedRules.test(ruleID); }

	/**
	 * @brief Resets the matched rule set to empty.
	 */
	void reset()
	{
		m_isInitialized = false;
		m_matchedRules.reset();
	}

	/**
	 * @brief Returns the number of matched rules.
	 *
	 * @return Number of matched rules.
	 */
	std::size_t count() const { return m_matchedRules.count(); }

	/**
	 * @brief Merges a new set of rule matches into the current set.
	 *
	 * If this is the first merge, the internal bitset is initialized
	 * with @p ruleBitset. Otherwise, the intersection (AND) of the
	 * current set and @p ruleBitset is computed.
	 *
	 * @param ruleBitset Bitset of rules to merge.
	 */
	void merge(const RuleBitset& ruleBitset)
	{
		if (!m_isInitialized) {
			m_matchedRules = ruleBitset;
			m_isInitialized = true;
		} else {
			m_matchedRules &= ruleBitset;
		}
	}

private:
	/** Indicates whether the bitset has been initialized by the first merge. */
	bool m_isInitialized = false;

	/** Current set of matched rules. */
	RuleBitset m_matchedRules;
};

/**
 * @brief Abstract base class for field-specific matchers.
 *
 * This class defines the interface for all matchers that evaluate rules
 * based on a specific UniRec field. Derived classes must implement
 * @ref matchRules to perform the actual evaluation.
 */
class RuleFieldMatcher {
public:
	/**
	 * @brief Constructs a matcher for a specific UniRec field.
	 *
	 * @param urFieldID ID of the UniRec field being matched.
	 * @param ruleCount Total number of rules in the rule set.
	 */
	RuleFieldMatcher(ur_field_id_t urFieldID, std::size_t ruleCount)
		: m_wildcardRules(ruleCount)
		, m_matchedRules(ruleCount)
		, m_urFieldID(urFieldID)
	{
	}

	/**
	 * @brief Matches rules against a UniRec record.
	 *
	 * Derived classes must implement this function to evaluate the field value
	 * of @p unirecRecordView and update @p ruleBitset accordingly.
	 *
	 * @param unirecRecordView A view of the UniRec record.
	 * @param ruleBitset Output rule set to be updated with matched rules.
	 */
	virtual void
	matchRules(const Nemea::UnirecRecordView& unirecRecordView, RuleMatchSet& ruleBitset)
		= 0;

protected:
	/** Rules that are matched unconditionally (wildcards). */
	RuleBitset m_wildcardRules;

	/** Temporary storage for rules matched during evaluation. */
	RuleBitset m_matchedRules;

	/** UniRec field ID that this matcher operates on. */
	ur_field_id_t m_urFieldID;
};

} // namespace ListDetector
