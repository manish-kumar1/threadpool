/*
 * managed_stop_source.cpp
 *
 *  Created on: Feb 6, 2021
 *      Author: xps33
 */

#include "include/managed_stop_source.hpp"
#include "include/managed_stop_token.hpp"

namespace thp {

[[nodiscard]] managed_stop_token managed_stop_source::get_managed_token()  noexcept {
	return managed_stop_token(this->shared_from_this());
}

} // namespace thp
