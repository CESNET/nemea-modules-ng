#include <cstdint>
#include <string>
#include <vector>

#pragma once

namespace NFieldProcessor {

enum Direction : uint8_t {
	SOURCE,
	DESTINATION,
	BOTH,
};

struct CommandLineParameters {
	Direction traffic;
	std::string source;
	std::string destination;
	std::string path;
	std::string fields;
	std::vector<std::string> validFields;
};

} // namespace NFieldProcessor
