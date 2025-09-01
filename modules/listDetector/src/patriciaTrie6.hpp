#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdint>
#include <memory>
#include <vector>

namespace ListDetector {

class PatriciaTrieV6 {
public:
	using Bits128 = std::bitset<128>;

	void insert(const Bits128& addr, int prefix_len, int id)
	{
		insertNode(root_, addr, prefix_len, id);
	}

	std::vector<int> search(const Bits128& ip) const
	{
		std::vector<int> out;
		searchNode(root_.get(), ip, out);
		return out;
	}

	static Bits128 fromBytes(const std::array<uint8_t, 16>& bytes)
	{
		Bits128 b;
		for (int i = 0; i < 128; ++i) {
			const int byte_index = i / 8;
			const int bit_in_byte = 7 - (i % 8);
			const bool bit = (bytes[byte_index] >> bit_in_byte) & 0x1;
			b.set(127 - i, bit);
		}
		return b;
	}

private:
	struct Node {
		Bits128 prefix;
		int prefix_len = 0;
		std::vector<int> ids;
		std::unique_ptr<Node> left;
		std::unique_ptr<Node> right;

		Node(Bits128 p, int len)
			: prefix(std::move(p))
			, prefix_len(len)
		{
		}
	};

	std::unique_ptr<Node> root_;

	static bool getBitMSB(const Bits128& v, int i) { return v.test(127 - i); }

	static Bits128 makeMask(int len)
	{
		Bits128 m;
		for (int i = 0; i < len; ++i)
			m.set(127 - i);
		return m;
	}

	static Bits128 truncatePrefix(const Bits128& v, int len) { return v & makeMask(len); }

	static bool equalPrefix(const Bits128& a, const Bits128& b, int len)
	{
		if (len == 0)
			return true;
		const Bits128 m = makeMask(len);
		return (a & m) == (b & m);
	}

	static int commonPrefixLen(const Bits128& a, const Bits128& b, int limit)
	{
		for (int i = 0; i < limit; ++i) {
			if (getBitMSB(a, i) != getBitMSB(b, i))
				return i;
		}
		return limit;
	}

	static void insertNode(std::unique_ptr<Node>& node, const Bits128& addr, int len, int id)
	{
		const Bits128 trimmed = truncatePrefix(addr, len);

		if (!node) {
			node = std::make_unique<Node>(trimmed, len);
			node->ids.push_back(id);
			return;
		}

		const int limit = std::min(len, node->prefix_len);
		const int i = commonPrefixLen(node->prefix, trimmed, limit);

		if (i == node->prefix_len && i == len) {
			node->ids.push_back(id);
			return;
		}

		if (i == node->prefix_len) {
			const bool bit = getBitMSB(trimmed, i);
			auto& child = bit ? node->right : node->left;
			insertNode(child, trimmed, len, id);
			return;
		}

		auto split = std::make_unique<Node>(truncatePrefix(node->prefix, i), i);
		const bool oldBit = getBitMSB(node->prefix, i);
		const bool newBit = getBitMSB(trimmed, i);

		if (oldBit)
			split->right = std::move(node);
		else
			split->left = std::move(node);

		auto newLeaf = std::make_unique<Node>(trimmed, len);
		newLeaf->ids.push_back(id);

		auto& target = newBit ? split->right : split->left;
		if (target) {
			insertNode(target, trimmed, len, id);
		} else {
			target = std::move(newLeaf);
		}

		node = std::move(split);
	}

	static void searchNode(const Node* node, const Bits128& ip, std::vector<int>& out)
	{
		if (!node)
			return;

		if (!node->ids.empty() && equalPrefix(ip, node->prefix, node->prefix_len)) {
			out.insert(out.end(), node->ids.begin(), node->ids.end());
		}

		if (node->prefix_len == 128)
			return;

		const bool bit = getBitMSB(ip, node->prefix_len);
		searchNode((bit ? node->right.get() : node->left.get()), ip, out);
	}
};

} // namespace ListDetector
