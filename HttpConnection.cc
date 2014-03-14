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

#include <stdint.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <map>
#include <string>
#include <vector>

#include <stdio.h>
#include <string.h>

#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpConnection.h"

using std::map;
using std::string;

namespace hw4 {

bool HttpConnection::GetNextRequest(HttpRequest *request) {
  // Use "WrappedRead" to read data into the buffer_
  // instance variable.  Keep reading data until either the
  // connection drops or you see a "\r\n\r\n" that demarcates
  // the end of the request header.
  //
  // Once you've seen the request header, use ParseRequest()
  // to parse the header into the *request argument.
  //
  // Very tricky part:  clients can send back-to-back requests
  // on the same socket.  So, you need to preserve everything
  // after the "\r\n\r\n" in buffer_ for the next time the
  // caller invokes GetNextRequest()!

  // MISSING:
  
  std::string exit_str = "\r\n\r\n";
  char *buf = (char *) malloc(sizeof(char) * 1024);
// how to check for connection dropped?
  size_t location_of_exit_str = 0;
  while (1) {
    int retval = WrappedRead(fd_, (unsigned char*) buf, 1024);
// check if retval < 1024???
    if (retval == -1 || retval == 0) {
      // close connection
      return false;
    }
    buffer_.append(buf, retval);
    
    location_of_exit_str = buffer_.find_first_of(exit_str);
    if (location_of_exit_str != string::npos) {
      // contains request header
   //   ParseRequest(location_of_exit_str); // or location - 1???
      break;
    }

  }
// ??????
if (location_of_exit_str != 0) {
  *request = ParseRequest(location_of_exit_str); // or location-1
}
  return true;
}

    //std::string buf_str(buf); // do i need this???                               
    /*char* contains = std::strstr(buf, "\r\n\r\n");                               
    if (contains != NULL) {                                                        
      // request header found                                                      
      // call parse request                                                        
      break;                                                                       
    }*/ 


bool HttpConnection::WriteResponse(const HttpResponse &response) {
  std::string str = response.GenerateResponseString();
  int res = WrappedWrite(fd_,
                         (unsigned char *) str.c_str(),
                         str.length());
  if (res != static_cast<int>(str.length()))
    return false;
  return true;
}

HttpRequest HttpConnection::ParseRequest(size_t end) {
  HttpRequest req;
  req.URI = "/";  // by default, get "/".

  // Get the header.
  std::string str = buffer_.substr(0, end);

  // Split the header into lines.  Extract the URI from the first line
  // and store it in req.URI.  For each additional line beyond the
  // first, extract out the header name and value and store them in
  // req.headers (i.e., req.headers[headername] = headervalue).
  // You should look at HttpResponse.h for details about the HTTP header
  // format that you need to parse.
  //
  // You'll probably want to look up boost functions for (a) splitting
  // a string into lines on a "\r\n" delimiter, (b) trimming
  // whitespace from the end of a string, and (c) converting a string
  // to lowercase.

  // MISSING:
  string delimiter = "\r\n";
  std::vector<string> lines;
  boost::split(lines, buffer_, boost::is_any_of(delimiter));
  
  std::vector<string> first_line;
  boost::split(first_line, lines[0], boost::is_any_of(" "));
  // get second token
  req.URI = first_line[1];

  for (size_t i = 1; i < lines.size(); i++) {
    // trim whitespace from end of string
    boost::trim(lines[i]);
    // convert string to lower case
    boost::to_lower(lines[i]);

    // get headername and headerval from line
    // line is of format [headername]: [headerval]
    std::vector<string> line;
    boost::split(line, lines[i], boost::is_any_of(": "));
    std::string headername = line[0];
    std::string headerval = line[1];

    req.headers.insert({headername, headerval});
  }
 
  

  return req;
}

}  // namespace hw4
