#include "plugin.hpp"

#include <fstream>

namespace NFieldClassifier {

#define SNI_PATH_DEFAULT "/tmp/sniTLS.csv"
#define SNI_FIELD_NAME_DEFAULT "TLS_SNI"

class SNIClassifier : public Plugin {
public:
	void handleParameters(argparse::ArgumentParser& parser) override;
	void storeParameters(argparse::ArgumentParser& parser) override;
	FieldDefinition defineFields() override;
	void init() override;
	void exit() override;
	bool getData(DataMap& dataMap, std::string& ipAddr) override;
	void getAdditionalDataFromUnirec(std::optional<Nemea::UnirecRecordView>& unirecView) override;

private:
	struct SNIRule {
		std::string sni; // SNI domain
		std::string company; // company associated with the domain (NULL if unknown)
		std::string flags; // flags associated with the domain
	};
	std::string m_pathToSNIFile = SNI_PATH_DEFAULT;
	std::ifstream m_filePtr;
	std::vector<SNIRule> m_tlsRules;
	std::string m_sniValue;
	ur_field_id_t m_sniFieldId;
	std::string m_SNIFieldName = SNI_FIELD_NAME_DEFAULT;
};
} // namespace NFieldClassifier
