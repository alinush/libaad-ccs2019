#include <aad/Configuration.h>

#include <aad/BitString.h>

#include <ostream>

namespace libaad {

std::ostream& operator<<(std::ostream& out, const libaad::BitString& bs) {
    out << bs.toString();
    return out;
}

}
