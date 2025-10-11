#include "asn.hpp"
#include "geolite.hpp"
#include "ipClassifier.hpp"
#include "sniClassifier.hpp"
#include <vector>

namespace NFieldClassifier {

// ALL PLUGINS VECTOR
// ADD NEW PLUGINS HERE:
//  ###################

static inline std::vector<class Plugin*> g_PLUGINS
	= {new Geolite(), new ASN(), new IPClassifier(), new SNIClassifier()};

// ###################

} // namespace NFieldClassifier
