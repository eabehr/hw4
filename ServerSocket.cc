/*
 * Copyright 2012 Steven Gribble
 *
 *  This file is part of the UW CSE 333 course project sequence
 *  (333proj).
 *
 *  333proj is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  333proj is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with 333proj.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>       // for snprintf()
#include <unistd.h>      // for close(), fcntl()
#include <sys/types.h>   // for socket(), getaddrinfo(), etc.
#include <sys/socket.h>  // for socket(), getaddrinfo(), etc.
#include <arpa/inet.h>   // for inet_ntop()
#include <netdb.h>       // for getaddrinfo()
#include <errno.h>       // for errno, used by strerror()
#include <string.h>      // for memset, strerror()
#include <iostream>      // for std::cerr, etc.

#include "./ServerSocket.h"

extern "C" {
  #include "libhw1/CSE333.h"
}

namespace hw4 {

ServerSocket::ServerSocket(uint16_t port) {
  port_ = port;
  listen_sock_fd_ = -1;
}

ServerSocket::~ServerSocket() {
  // Close the listening socket if it's not zero.  The rest of this
  // class will make sure to zero out the socket if it is closed
  // elsewhere.
  if (listen_sock_fd_ != -1)
    close(listen_sock_fd_);
  listen_sock_fd_ = -1;
}

bool ServerSocket::BindAndListen(int ai_family, int *listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd".

  // MISSING:
  int retval, socket_fd;
  struct addrinfo hints, *results, *r;

  // zero out hints data structure
  memset(&hints, 0, sizeof(hints));

  // use getaddrinfo
  retval = getaddrinfo(NULL, NULL, &hints, &results); // 1st,2nd parameter?
  //Verify333(retval == 0);
  if (retval != 0) {
    return false;
  }

  for (r = results; r != NULL; r = r->ai_next) {
    // IPv4
    r->ai_family = ai_family;
    if (r->ai_family == AF_INET) {
      char ipstring[INET_ADDRSTRLEN];
      struct sockaddr_in *v4addr = (struct sockaddr_in *) r->ai_addr;
      inet_ntop(r->ai_family, &(v4addr->sin_addr), ipstring,
            INET_ADDRSTRLEN);
    } else if (r->ai_family == AF_INET6) { //IPv6
      char ipstring[INET6_ADDRSTRLEN];
      struct sockaddr_in6 *v6addr = (struct sockaddr_in6 *) r->ai_addr;
      inet_ntop(r->ai_family, &(v6addr->sin6_addr), ipstring,
            INET6_ADDRSTRLEN);
    } else if (r->ai_family == AF_UNSPEC) { //try 4, then 6?
      // idkkk
    } else {
      // idk
    }
  
    // use socket
        // should i have if else for aifamily?
        // should this all be in for loop?
    socket_fd = socket(ai_family, SOCK_STREAM, 0);
    if (socket_fd == -1) {
      return false; // return false or Verify333?
    }

    // use bind
    if (bind(socket_fd, r->ai_addr, r->ai_addrlen) == 0) {
      break;
    }
  }
  // use listen - create listening socket on port port_
  retval = listen(socket_fd, SOMAXCONN);
  //Verify333(retval == 0);
  if (retval != 0) {
    return false;
  }

  // return listening socket through output param listen_fd
  *listen_fd = socket_fd;  
  return true;
}

bool ServerSocket::Accept(int *accepted_fd,
                          std::string *client_addr,
                          uint16_t *client_port,
                          std::string *client_dnsname,
                          std::string *server_addr,
                          std::string *server_dnsname) {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // MISSING:

  while (1) {
    struct sockaddr_storage caddr;
    socklen_t caddr_len = sizeof(caddr);
    int client_fd = accept(listen_sock_fd_,
                      reinterpret_cast<struct sockaddr *>(&caddr),
                      &caddr_len);
    if (client_fd < 0) {
      // used wrappedread or something?
      if ((errno == EAGAIN) || (errno == EINTR)) {
        continue;
      } else {
        return false;
      }
    }
    

  }

  return true;
}

}  // namespace hw4
