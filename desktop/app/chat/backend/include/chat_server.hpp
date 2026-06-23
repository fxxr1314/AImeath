#pragma once

#include <string>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

/// Run a chat session on an already-established WebSocket.
/// first_msg is the first JSON message already read from the wire.
void chatServe(boost::beast::websocket::stream<boost::beast::tcp_stream>& ws,
               const std::string& first_msg);
