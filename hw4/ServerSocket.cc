/*
 * Copyright ©2022 Hal Perkins.  All rights reserved.  Permission is
 * hereby granted to students registered for University of Washington
 * CSE 333 for use solely during Fall Quarter 2022 for purposes of
 * the course.  No other use, copying, distribution, or modification
 * is permitted without prior written consent. Copyrights for
 * third-party components of this work must be honored.  Instructors
 * interested in reusing these course materials should contact the
 * author.
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

#define HOST_NAME_MAX_LEN 1024

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

bool ServerSocket::BindAndListen(int ai_family, int* const listen_fd) {
  // Use "getaddrinfo," "socket," "bind," and "listen" to
  // create a listening socket on port port_.  Return the
  // listening socket through the output parameter "listen_fd"
  // and set the ServerSocket data member "listen_sock_fd_"

  // STEP 1:

  // Populate the "hints" addrinfo structure for getaddrinfo()
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_flags |= AI_V4MAPPED;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_canonname = nullptr;
  hints.ai_addr = nullptr;
  hints.ai_next = nullptr;

  // Get the address information
  struct addrinfo *result;
  std::string port = std::to_string(port_);
  int retval;
  if ((retval = getaddrinfo(nullptr, port.c_str(), &hints, &result)) != 0) {
    std::cerr << "getaddrinfo failed: ";
    std::cerr << gai_strerror(retval) << std::endl;
    return false;
  }

  // Loop through the returned address structures until we are able to create a
  // socket and bind to one
  int listen_sock_fd = -1;
  for (struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) {
    listen_sock_fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (listen_sock_fd == -1) {
      std::cerr << "socket() failed: " << strerror(errno) << std::endl;
      listen_sock_fd = -1;
      continue;
    }

    // Configure the socket
    int optval = 1;
    setsockopt(listen_sock_fd, SOL_SOCKET, SO_REUSEADDR,
               &optval, sizeof(optval));

    // Try binding the socket to the address and port number returned by
    // getaddrinfo()
    if (bind(listen_sock_fd, rp->ai_addr, rp->ai_addrlen) == 0) {
      // Bind worked!
      sock_family_ = rp->ai_family;
      break;
    }

    // Bind failed. Close the socket and then try the next address/port returned
    // by getaddrinfo()
    close(listen_sock_fd);
    listen_sock_fd = -1;
  }

  freeaddrinfo(result);
  // Failed to create a socket and bind. Return false
  if (listen_sock_fd == -1) {
    return false;
  }
  // Tell the OS we want this to be a listening socket
  if (listen(listen_sock_fd, SOMAXCONN) != 0) {
    std::cerr << "Failed to mark socket as listening: " << strerror(errno);
    std::cerr << std::endl;
    close(listen_sock_fd);
    return false;
  }
  // Set output parameter and private field
  *listen_fd = listen_sock_fd;
  listen_sock_fd_ = listen_sock_fd;

  return true;
}

bool ServerSocket::Accept(int* const accepted_fd,
                          std::string* const client_addr,
                          uint16_t* const client_port,
                          std::string* const client_dns_name,
                          std::string* const server_addr,
                          std::string* const server_dns_name) const {
  // Accept a new connection on the listening socket listen_sock_fd_.
  // (Block until a new connection arrives.)  Return the newly accepted
  // socket, as well as information about both ends of the new connection,
  // through the various output parameters.

  // STEP 2:
  struct sockaddr_storage caddr;
  socklen_t caddr_len = sizeof(caddr);
  struct sockaddr* addr = reinterpret_cast<struct sockaddr*>(&caddr);
  int client_fd = -1;
  while (1) {
    // Accept the client's connection
    client_fd = accept(listen_sock_fd_, addr, &caddr_len);
    if (client_fd < 0) {
      // Error occurs
      if ((errno == EINTR) || (errno == EAGAIN) || (errno == EWOULDBLOCK)) {
        continue;
      }
      std::cerr << "Failure on accept: " << strerror(errno) << std::endl;
      return false;
    }
    break;
  }
  if (client_fd < 0) {
    return false;
  }
  *accepted_fd = client_fd;
  // Write client's address and port to output parameters
  if (addr->sa_family == AF_INET) {
    // Handle IPv4 address
    struct sockaddr_in *v4addr = reinterpret_cast<struct sockaddr_in*>(addr);
    char astring[INET_ADDRSTRLEN];
    // Converts the client's address to string representation
    inet_ntop(AF_INET, &(v4addr->sin_addr), astring,
              INET_ADDRSTRLEN);
    *client_addr = std::string(astring);
    *client_port = v4addr->sin_port;
  } else if (addr->sa_family == AF_INET6) {
    // Handle IPv6 address
    struct sockaddr_in6 *v6addr = reinterpret_cast<struct sockaddr_in6*>(addr);
    char astring[INET6_ADDRSTRLEN];
    // Converts the client's address to string representation
    inet_ntop(AF_INET6, &(v6addr->sin6_addr), astring,
              INET6_ADDRSTRLEN);
    *client_addr = std::string(astring);
    *client_port = v6addr->sin6_port;
  }

  // Do a reverse DNS lookup on the client to get its DNS name and write to
  // the output parameter
  char host[HOST_NAME_MAX_LEN];
  int res;
  // Look up the client's DNS name
  res = getnameinfo(addr, caddr_len, host, HOST_NAME_MAX_LEN, NULL, 0, 0);
  if (res != 0) {
    std::cerr << "getnameinfo failed: ";
    std::cerr << gai_strerror(res) << std::endl;
    return false;
  }
  *client_dns_name = std::string(host);

  // Get the server's address and DNS name and write them to the output
  // parameters
  char server_host[HOST_NAME_MAX_LEN];
  server_host[0] = '\0';
  if (sock_family_ == AF_INET) {
    // Handle IPv4 address
    struct sockaddr_in sock_addr;
    socklen_t sock_addr_len = sizeof(sock_addr);
    // Look up the server's address information
    res = getsockname(client_fd,
                      reinterpret_cast<struct sockaddr*>(&sock_addr),
                      &sock_addr_len);
    if (res != 0) {
      std::cerr << "getsockname failed: " << strerror(errno) << std::endl;
      return false;
    }
    char astring[INET_ADDRSTRLEN];
    // Converts the server's address to string representation
    inet_ntop(AF_INET, &sock_addr.sin_addr, astring, INET_ADDRSTRLEN);
    // Look up the server's DNS name
    res = getnameinfo(reinterpret_cast<struct sockaddr*>(&sock_addr),
                sock_addr_len, server_host, HOST_NAME_MAX_LEN, NULL, 0, 0);
    if (res != 0) {
      std::cerr << "getnameinfo failed: ";
      std::cerr << gai_strerror(res) << std::endl;
      return false;
    }
    *server_addr = std::string(astring);
    *server_dns_name = std::string(server_host);
  } else if (sock_family_ == AF_INET6) {
    // Handle IPv6 address
    struct sockaddr_in6 sock_addr;
    socklen_t sock_addr_len = sizeof(sock_addr);
    // Look up the server's address information
    res = getsockname(client_fd,
                      reinterpret_cast<struct sockaddr*>(&sock_addr),
                      &sock_addr_len);
    if (res != 0) {
      std::cerr << "getsockname failed: " << strerror(errno) << std::endl;
      return false;
    }
    char astring[INET_ADDRSTRLEN];
    // Converts the server's address to string representation
    inet_ntop(AF_INET6, &sock_addr.sin6_addr, astring, INET6_ADDRSTRLEN);
    // Look up the server's DNS name
    res = getnameinfo(reinterpret_cast<struct sockaddr*>(&sock_addr),
                sock_addr_len, server_host, HOST_NAME_MAX_LEN, NULL, 0, 0);
    if (res != 0) {
      std::cerr << "getnameinfo failed: ";
      std::cerr << gai_strerror(res) << std::endl;
      return false;
    }
    *server_addr = std::string(astring);
    *server_dns_name = std::string(server_host);
  }

  return true;
}

}  // namespace hw4
