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

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>

#include "./FileReader.h"
#include "./HttpConnection.h"
#include "./HttpRequest.h"
#include "./HttpUtils.h"
#include "./HttpServer.h"
#include "./libhw3/QueryProcessor.h"

using std::cerr;
using std::cout;
using std::endl;

namespace hw4 {

// This is the function that threads are dispatched into
// in order to process new client connections.
void HttpServer_ThrFn(ThreadPool::Task *t);

// Given a request, produce a response.
HttpResponse ProcessRequest(const HttpRequest &req,
                            const std::string &basedir,
                            const std::list<std::string> *indices);

// Process a file request.
HttpResponse ProcessFileRequest(const std::string &uri,
                                const std::string &basedir);

// Process a query request.
HttpResponse ProcessQueryRequest(const std::string &uri,
                                 const std::list<std::string> *indices);

bool HttpServer::Run(void) {
  // Create the server listening socket.
  int listen_fd;
  cout << "  creating and binding the listening socket..." << endl;
  if (!ss_.BindAndListen(AF_UNSPEC, &listen_fd)) {
    cerr << endl << "Couldn't bind to the listening socket." << endl;
    return false;
  }

  // Spin, accepting connections and dispatching them.  Use a
  // threadpool to dispatch connections into their own thread.
  cout << "  accepting connections..." << endl << endl;
  ThreadPool tp(kNumThreads);
  while (1) {
    HttpServerTask *hst = new HttpServerTask(HttpServer_ThrFn);
    hst->basedir = staticfile_dirpath_;
    hst->indices = &indices_;
    if (!ss_.Accept(&hst->client_fd,
                    &hst->caddr,
                    &hst->cport,
                    &hst->cdns,
                    &hst->saddr,
                    &hst->sdns)) {
      // The accept failed for some reason, so quit out of the server.
      break;
    }
    // The accept succeeded; dispatch it.
    tp.Dispatch(hst);
  }
  return true;
}

void HttpServer_ThrFn(ThreadPool::Task *t) {
  // Cast back our HttpServerTask structure with all of our new
  // client's information in it.
  std::unique_ptr<HttpServerTask> hst(static_cast<HttpServerTask *>(t));
  cout << "  client " << hst->cdns << ":" << hst->cport << " "
       << "(IP address " << hst->caddr << ")" << " connected." << endl;

  bool done = false;
  while (!done) {
    // Use the HttpConnection class to read in the next request from
    // this client, process it by invoking ProcessRequest(), and then
    // use the HttpConnection class to write the response.  If the
    // client sent a "Connection: close\r\n" header, then shut down
    // the connection.
    
    // MISSING:
    HttpConnection h_conn(hst->client_fd);  
    HttpRequest req;
    bool read = h_conn.GetNextRequest(&req);
    if (!read) {
      done = true;
      break;
    }

    // see if client sent close header
    if ((req.headers.find("connection") != req.headers.end())
        && (req.headers["connection"] == "close")) {
      done = true;
    }

    HttpResponse resp = ProcessRequest(req, hst->basedir, hst->indices);
    bool write = h_conn.WriteResponse(resp);
    if (!write) {
      done = true;
      break;
    }
  }
}

HttpResponse ProcessRequest(const HttpRequest &req,
                            const std::string &basedir,
                            const std::list<std::string> *indices) {
  // Is the user asking for a static file?
  if (req.URI.substr(0, 8) == "/static/") {
    return ProcessFileRequest(req.URI, basedir);
  }

  // The user must be asking for a query.
  return ProcessQueryRequest(req.URI, indices);
}


HttpResponse ProcessFileRequest(const std::string &uri,
                                const std::string &basedir) {
  // The response we'll build up.
  HttpResponse ret;

  // Steps to follow:
  //  - use the URLParser class to figure out what filename
  //    the user is asking for.
  //
  //  - use the FileReader class to read the file into memory
  //
  //  - copy the file content into the ret.body
  //
  //  - depending on the file name suffix, set the response
  //    Content-type header as appropriate, e.g.,:
  //      --> for ".html" or ".htm", set to "text/html"
  //      --> for ".jpeg" or ".jpg", set to "image/jpeg"
  //      --> for ".png", set to "image/png"
  //      etc.
  //
  // be sure to set the response code, protocol, and message
  // in the HttpResponse as well.
  std::string fname = "";

  // MISSING:

  URLParser url_p;
  // check if URI starts with static - if so, remove that keyword
  std::string check_static = uri.substr(0,8);
  if (check_static == "/static/") { 
    url_p.Parse(uri.substr(8));
  } else {
    url_p.Parse(uri);
  }
  fname = url_p.get_path();

  FileReader fr(basedir, fname);
  std::string str;
  bool read = fr.ReadFile(&str);
  if (read) {
    ret.protocol = "HTTP/1.1";
    ret.response_code = 200;
    ret.message = "success presumably";
    
    std::string type = "";
    size_t loc_of_dot = fname.find(".");
    std::string fn_suffix = fname.substr(loc_of_dot, fname.length());
    if (fn_suffix == ".html" || fn_suffix == ".htm") {
      type = "text/html";
    } else if (fn_suffix == ".jpeg" || fn_suffix == ".jpg") {
      type = "image/jpeg";
    } else if (fn_suffix == ".png") {
      type = "image/png";
    }
  
    ret.headers["Content-type"] = type;
    ret.body = str;
  } else {
    // If you couldn't find the file, return an HTTP 404 error.
    ret.protocol = "HTTP/1.1";
    ret.response_code = 404;
    ret.message = "Not Found";
    ret.body = "<html><body>Couldn't find file \"";
    ret.body +=  EscapeHTML(fname);
    ret.body += "\"</body></html>";
  }

  return ret;
}

HttpResponse ProcessQueryRequest(const std::string &uri,
                                 const std::list<std::string> *indices) {
  // The response we're building up.
  HttpResponse ret;

  // Your job here is to figure out how to present the user with
  // the same query interface as our solution_binaries/http333d server.
  // A couple of notes:
  //
  //  - no matter what, you need to present the 333gle logo and the
  //    search box/button
  //
  //  - if the user had previously typed in a search query, you also
  //    need to display the search results.
  //
  //  - you'll want to use the URLParser to parse the uri and extract
  //    search terms from a typed-in search query.  convert them
  //    to lower case.
  //
  //  - you'll want to create and use a hw3::QueryProcessor to process
  //    the query against the search indices
  //
  //  - in your generated search results, see if you can figure out
  //    how to hyperlink results to the file contents, like we did
  //    in our solution_binaries/http333d.

  // MISSING:

  std::string presentation = "<html><head><title>333gle</title></head>";
  presentation += "<body><center style=\"font-size:500%;\">";
  presentation += "<span style=\"position:relative;bottom:-0.33em;";
  presentation += "color:orange;\">3</span><span style=\"color:red;\">3</span>";
  presentation += "<span style=\"color:gold;\">3</span>";
  presentation += "<span style=\"color:blue;\">g</span>";
  presentation += "<span style=\"color:green;\">l</span>";
  presentation += "<span style=\"color:red;\">e</span>";
  presentation += "</center><p><div style=\"height:20px;\"></div><center>";
  presentation += "<form action=\"/query\" method=\"get\">";
  presentation += "<input type=\"text\" size=30 name=\"terms\" />";
  presentation += "<input type=\"submit\" value=\"Search\" /></form>";
  presentation += "</center><p></body></html>";

  URLParser url_p;
// ???  std::string uri_lower = uri.to_lower();
  url_p.Parse(uri);
  std::map<std::string, std::string> args = url_p.get_args();

  hw3::QueryProcessor qp(*indices);
  ret.body = presentation;
  return ret;
}

}  // namespace hw4
