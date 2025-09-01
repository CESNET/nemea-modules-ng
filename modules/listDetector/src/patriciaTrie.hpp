#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace ListDetector {

struct PatriciaNode {
	uint32_t prefix;
	int prefix_len;
	std::vector<int> ids;

	std::unique_ptr<PatriciaNode> left;
	std::unique_ptr<PatriciaNode> right;

	PatriciaNode(uint32_t p, int len)
		: prefix(p)
		, prefix_len(len)
	{
	}
};

class PatriciaTrie {
public:
	PatriciaTrie() = default;

	void insert(uint32_t prefix, int prefix_len, int id)
	{
		if (!root) {
			root = std::make_unique<PatriciaNode>(prefix, prefix_len);
			root->ids.push_back(id);
			return;
		}
		insertRecursive(root, prefix, prefix_len, id);
	}

	std::vector<int> search(uint32_t ip) const
	{
		std::vector<int> result;
		searchRecursive(root.get(), ip, result);
		return result;
	}

private:
	std::unique_ptr<PatriciaNode> root;

	void
	insertRecursive(std::unique_ptr<PatriciaNode>& node, uint32_t prefix, int prefix_len, int id)
	{
		if (!node) {
			node = std::make_unique<PatriciaNode>(prefix, prefix_len);
			node->ids.push_back(id);
			return;
		}

		// Najdeme první bit, kde se prefixy liší
		int common_len
			= commonPrefixLength(node->prefix, prefix, std::min(node->prefix_len, prefix_len));

		if (common_len < node->prefix_len) {
			// Split
			auto new_node = std::make_unique<PatriciaNode>(node->prefix, common_len);
			if (getBit(node->prefix, common_len))
				new_node->right = std::move(node);
			else
				new_node->left = std::move(node);
			node = std::move(new_node);
		}

		if (prefix_len == common_len) {
			node->ids.push_back(id);
			return;
		}

		bool bit = getBit(prefix, common_len);
		if (bit)
			insertRecursive(node->right, prefix, prefix_len, id);
		else
			insertRecursive(node->left, prefix, prefix_len, id);
	}

	void searchRecursive(const PatriciaNode* node, uint32_t ip, std::vector<int>& result) const
	{
		if (!node)
			return;

		if (matchPrefix(ip, node->prefix, node->prefix_len)) {
			result.insert(result.end(), node->ids.begin(), node->ids.end());
		}

		bool bit = getBit(ip, node->prefix_len);
		if (bit)
			searchRecursive(node->right.get(), ip, result);
		else
			searchRecursive(node->left.get(), ip, result);
	}

	static bool getBit(uint32_t value, int pos) { return (value >> (31 - pos)) & 1; }

	static bool matchPrefix(uint32_t ip, uint32_t prefix, int len)
	{
		if (len == 0)
			return true;
		uint32_t mask = len == 32 ? 0xFFFFFFFFu : (~0u << (32 - len));
		return (ip & mask) == (prefix & mask);
	}

	static int commonPrefixLength(uint32_t a, uint32_t b, int max_len)
	{
		for (int i = 0; i < max_len; ++i) {
			if (getBit(a, i) != getBit(b, i))
				return i;
		}
		return max_len;
	}
};

} // namespace ListDetector
