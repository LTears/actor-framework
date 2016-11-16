/******************************************************************************
 *                       ____    _    _____                                   *
 *                      / ___|  / \  |  ___|    C++                           *
 *                     | |     / _ \ | |_       Actor                         *
 *                     | |___ / ___ \|  _|      Framework                     *
 *                      \____/_/   \_|_|                                      *
 *                                                                            *
 * Copyright (C) 2011 - 2016                                                  *
 * Dominik Charousset <dominik.charousset (at) haw-hamburg.de>                *
 *                                                                            *
 * Distributed under the terms and conditions of the BSD 3-Clause License or  *
 * (at your option) under the terms and conditions of the Boost Software      *
 * License 1.0. See accompanying files LICENSE and LICENSE_ALTERNATIVE.       *
 *                                                                            *
 * If you did not receive a copy of the license files, see                    *
 * http://opensource.org/licenses/BSD-3-Clause and                            *
 * http://www.boost.org/LICENSE_1_0.txt.                                      *
 ******************************************************************************/

#ifndef CAF_IO_NETWORK_DGRAM_ACCEPTOR_MANGER_HPP
#define CAF_IO_NETWORK_DGRAM_ACCEPTOR_MANGER_HPP

#include "caf/io/network/manager.hpp"

namespace caf {
namespace io {
namespace network {

/// A datagram source manager provides callbacks for incoming
/// datagrams as well as for error handling.
class dgram_acceptor_manager : public manager {
public:
  dgram_acceptor_manager(abstract_broker* ptr);

  ~dgram_acceptor_manager();

  virtual bool new_endpoint() = 0;

};

} // namespace network
} // namespace io
} // namespace caf

#endif // CAF_IO_NETWORK_DGRAM_ACCEPTOR_MANGER_HPP