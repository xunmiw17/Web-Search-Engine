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

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

#define BUF_SIZE 1024

using std::map;
using std::string;
using std::vector;

namespace hw4 {

static const char* kHeaderEnd = "\r\n\r\n";
static const int kHeaderEndLen = 4;

bool HttpConnection::GetNextRequest(HttpRequest* const request) {
  // Use WrappedRead from HttpUtils.cc to read bytes from the files into
  // private buffer_ variable. Keep reading until:
  // 1. The connection drops
  // 2. You see a "\r\n\r\n" indicating the end of the request header.
  //
  // Hint: Try and read in a large amount of bytes each time you call
  // WrappedRead.
  //
  // After reading complete request header, use ParseRequest() to parse into
  // an HttpRequest and save to the output parameter request.
  //
  // Important note: Clients may send back-to-back requests on the same socket.
  // This means WrappedRead may also end up reading more than one request.
  // Make sure to save anything you read after "\r\n\r\n" in buffer_ for the
  // next time the caller invokes GetNextRequest()!

  // STEP 1:
  // If buffer_ does not contain "\r\n\r\n", read the request header
  size_t pos = buffer_.find("\r\n\r\n");
  if (pos == string::npos) {
    int bytes_read;
    unsigned char buf[BUF_SIZE];
    while (1) {
      bytes_read = WrappedRead(fd_, buf, BUF_SIZE);
      if (bytes_read == -1) {
        return false;
      } else if (bytes_read == 0) {
        break;
      } else {
        // Append the read bytes to buffer_
        buffer_ += string(reinterpret_cast<char*>(buf), bytes_read);
        pos = buffer_.find("\r\n\r\n");
        if (pos != string::npos) {
          // There is occurrence of "\r\n\r\n"!
          break;
        }
      }
    }
  }

  // The request header does not contain "\r\n\r\n"
  if (pos == string::npos) {
    return false;
  }

  // Store the parsed request into the output parameter
  HttpRequest req = ParseRequest(buffer_.substr(0, pos + 4));
  *request = req;

  // Save anything after "\r\n\r\n" in buffer_
  buffer_ = buffer_.substr(0, pos + 4);

  return false;  // You may want to change this.
}

bool HttpConnection::WriteResponse(const HttpResponse& response) const {
  string str = response.GenerateResponseString();
  int res = WrappedWrite(fd_,
                         reinterpret_cast<const unsigned char*>(str.c_str()),
                         str.length());
  if (res != static_cast<int>(str.length()))
    return false;
  return true;
}

HttpRequest HttpConnection::ParseRequest(const string& request) const {
  HttpRequest req("/");  // by default, get "/".

  // Plan for STEP 2:
  // 1. Split the request into different lines (split on "\r\n").
  // 2. Extract the URI from the first line and store it in req.URI.
  // 3. For the rest of the lines in the request, track the header name and
  //    value and store them in req.headers_ (e.g. HttpRequest::AddHeader).
  //
  // Hint: Take a look at HttpRequest.h for details about the HTTP header
  // format that you need to parse.
  //
  // You'll probably want to look up boost functions for:
  // - Splitting a string into lines on a "\r\n" delimiter
  // - Trimming whitespace from the end of a string
  // - Converting a string to lowercase.
  //
  // Note: If a header is malformed, skip that line.

  // STEP 2:
  // Split the request into lines
  vector<string> lines;
  boost::split(lines, request, boost::is_any_of("\r\n"));

  // Trim whitespace from the end of a line, for all lines
  for (auto it = lines.begin(); it < lines.end(); it++) {
    boost::trim_right(*it);
  }

  // Get the first line, extract the URI, and store it in req.URI
  vector<string> toks;
  boost::split(toks, lines[0], boost::is_any_of(" "));
  req.set_uri(toks[1]);

  // For the rest of the lines, extract the header name and value and store them
  for (auto it = lines.begin(); it < lines.end(); it++) {
    if (it != lines.begin()) {

    }
  }

  return req;
}

}  // namespace hw4
