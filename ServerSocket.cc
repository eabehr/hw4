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

void GetDNSname(struct sockaddr *addr, size_t addrlen, std::string *dnsname);

void GetInfo(int fd, struct sockaddr *addr, size_t addrlen,
      std::string *set_addr, uint16_t *port);


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
  int retval;
  struct addrinfo hints, *result;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = ai_family;      // allow IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM;  // stream
  hints.ai_flags = AI_PASSIVE;      // use wildcard "INADDR_ANY"
  hints.ai_protocol = IPPROTO_TCP;  // tcp protocol
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  // use getaddrinfo
  // convert port to a char*
  std::string port_str = std::to_string(port_);
  const char* port_c_str = port_str.c_str();

  retval = getaddrinfo(NULL, port_c_str, &hints, &result);
  if (retval != 0) {
    std::cerr << "getaddrinfo() failed: ";
    std::cerr << gai_strerror(retval) << std::endl;
    return false;
  }

  *listen_fd = -1;
  for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) {
    *listen_fd = socket(rp->ai_family,
                       rp->ai_socktype,
                       rp->ai_protocol);
    if (*listen_fd == -1) {
      // Creating this socket failed.  So, loop to the next returned
      // result and try again.
      std::cerr << "socket() failed: " << strerror(errno) << std::endl;
      *listen_fd = -1;
      continue;
    }

    // Configure the socket; we're setting a socket "option."  In
    // particular, we set "SO_REUSEADDR", which tells the TCP stack
    // so make the port we bind to available again as soon as we
    // exit, rather than waiting for a few tens of seconds to recycle it.
    int optval = 1;
    retval = setsockopt(*listen_fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));
    Verify333(retval == 0);

    // Try binding the socket to the address and port number returned
    // by getaddrinfo().
    if (bind(*listen_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      break;
    }

    // The bind failed.  Close the socket, then loop back around and
    // try the next address/port returned by getaddrinfo().
    close(*listen_fd);
    *listen_fd = -1;
  }

  // Free the structure returned by getaddrinfo().
  freeaddrinfo(result);

  // If we failed to bind, return failure
  if (*listen_fd == -1) {
    return false;
  }

  // Succes. Tell the OS that we want this to be a listening socket.
  if (listen(*listen_fd, SOMAXCONN) != 0) {
    std::cerr << "Failed to mark socket as listening: ";
    std::cerr << strerror(errno) << std::endl;
    close(*listen_fd);
    return false;
  }

  listen_sock_fd_ = *listen_fd;
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
      if ((errno == EAGAIN) || (errno == EINTR)) {
        continue;
      } else {
        return false;
      }
    }
    
    // get accepted_fd value
    *accepted_fd = client_fd;

    // get client addr and port
    GetInfo(client_fd, reinterpret_cast<struct sockaddr *>(&caddr),
                caddr_len, client_addr, client_port);
    
    // get client dnsname
    GetDNSname(reinterpret_cast<struct sockaddr *>(&caddr), caddr_len,
                client_dnsname);

    // get server addr
    struct sockaddr_storage saddr;
    socklen_t saddr_len = sizeof(saddr);
    int retval = getsockname(client_fd,
              reinterpret_cast<struct sockaddr *>(&saddr), &saddr_len);
    Verify333(retval == 0);
    GetInfo(client_fd, reinterpret_cast<struct sockaddr *>(&saddr),
          saddr_len, server_addr, client_port);

    // get server name
    GetDNSname(reinterpret_cast<struct sockaddr *>(&saddr), saddr_len,
                  server_dnsname);
 
   return true;
  }
}

void GetDNSname(struct sockaddr *addr, size_t addrlen,
                  std::string *dnsname) {
  char hostname[1024];
  int retval = getnameinfo(addr, addrlen, hostname, 1024, NULL, 0, 0);
  Verify333(retval == 0);
  *dnsname = hostname;
}

void GetInfo(int fd, struct sockaddr *addr, size_t addrlen,
        std::string *set_addr, uint16_t *port) {
  if (addr->sa_family == AF_INET) {
    // set the IPV4 address and port
    char astring[INET_ADDRSTRLEN];
    struct sockaddr_in *in4 = reinterpret_cast<struct sockaddr_in *>(addr);
    inet_ntop(AF_INET, &(in4->sin_addr), astring, INET_ADDRSTRLEN);
    *set_addr = astring;
    *port = htons(in4->sin_port);
  } else if (addr->sa_family == AF_INET6) {
    // set the IPV4 address and port
    char astring[INET6_ADDRSTRLEN];
    struct sockaddr_in6 *in6 = reinterpret_cast<struct sockaddr_in6 *>(addr);
    inet_ntop(AF_INET6, &(in6->sin6_addr), astring, INET6_ADDRSTRLEN);
    *set_addr = astring;
    *port = htons(in6->sin6_port);
  } else {
    std::cout << "error? idk how to handle this" << std::endl;
  }
}

}  // namespace hw4
