#include "geolite.hpp"
#include <vector>

namespace NFieldClassifier {

// ALL PLUGINS VECTOR
// ADD NEW PLUGINS HERE:
//  ###################

static inline std::vector<class Plugin*> g_PLUGINS = {new Geolite()};

// ###################

} // namespace NFieldClassifier
