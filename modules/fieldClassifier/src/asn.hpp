#include "plugin.hpp"
#include <maxminddb.h>
#include <unordered_map>

namespace NFieldProcessor {

/**
 * @brief Geolite class add Unirec geolocation fields
 */
class ASNClassifier : public MaxMindPlugin {
public:
	/**
	 * @brief Initializes the MaxMind database. Throws DatabaseException on failure.
	 * @param path Path to the MaxMind database file.
	 */
	void init(const CommandLineParameters& params) override;

	/**
	 * @brief Closes the MaxMind database.
	 */
	void exit() override;

	/**
	 * @brief Methods to retrieve specific geolocation data.
	 *
	 * @return Return geolocation data retreved from database of default empty value if error or
	 * data not found. (EMPTY_STRING, EMPTY_DOUBLE)
	 */
	bool getData(std::unordered_map<std::string, Field>& fields, PluginData& pluginData) override;
};

} // namespace NFieldProcessor
