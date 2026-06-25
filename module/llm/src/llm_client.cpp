#include "llm_client.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/json.hpp>

#include <limits>
#include <sstream>
#include <atomic>

namespace asio  = boost::asio;
namespace beast = boost::beast;
namespace http  = beast::http;
namespace ssl   = asio::ssl;
using tcp = asio::ip::tcp;

struct LlmClient::Impl : public std::enable_shared_from_this<LlmClient::Impl>
{
    asio::io_context& io;
    tcp::resolver resolver_;
    std::unique_ptr<ssl::stream<beast::tcp_stream>> stream_;
    asio::steady_timer timer_;
    beast::flat_buffer buf_;
    http::response_parser<http::string_body> parser_;
    http::request<http::string_body> req_;

    std::string host_, port_, body_, auth_;
    std::string full_response_, full_reasoning_;
    std::string leftover_;
    size_t prev_body_size_ = 0;

    LlmEventFn on_event_;
    LlmDoneFn on_done_;
    std::atomic<bool> cancelled_{false};

    static ssl::context& ssl_ctx()
    {
        static ssl::context ctx(ssl::context::tlsv12_client);
        return ctx;
    }

    Impl(asio::io_context& io_)
        : io(io_), resolver_(io_)
        , stream_(std::make_unique<ssl::stream<beast::tcp_stream>>(io_, ssl_ctx()))
        , timer_(io_)
    {}

    void start(const std::string& host, const std::string& port,
               const std::string& body, const std::string& auth,
               LlmEventFn on_event, LlmDoneFn on_done,
               std::chrono::seconds timeout)
    {
        on_event_ = std::move(on_event);
        on_done_ = std::move(on_done);
        body_ = body;
        auth_ = auth;
        host_ = host;
        port_ = port;

        auto self = shared_from_this();
        auto timeout_sec = timeout.count();
        timer_.expires_after(timeout);
        timer_.async_wait([self, timeout_sec](beast::error_code ec) {
            if (ec == asio::error::operation_aborted || self->cancelled_) return;
            self->finish_error("request timeout after " +
                std::to_string(timeout_sec) + "s");
        });

        do_resolve();
    }

    void cancel()
    {
        cancelled_ = true;
        timer_.cancel();
        resolver_.cancel();
        try {
            if (stream_) stream_->lowest_layer().cancel();
        } catch (...) {}
    }

private:
    void do_resolve()
    {
        auto self = shared_from_this();
        resolver_.async_resolve(host_, port_,
            [self](beast::error_code ec, tcp::resolver::results_type results) {
                if (ec || self->cancelled_) { if (ec) self->finish_error(ec.message()); return; }
                self->do_connect(results);
            });
    }

    void do_connect(tcp::resolver::results_type results)
    {
        auto self = shared_from_this();
        beast::get_lowest_layer(*self->stream_).expires_after(std::chrono::seconds(30));
        beast::get_lowest_layer(*self->stream_).async_connect(results,
            [self](beast::error_code ec, auto) {
                if (ec || self->cancelled_) { if (ec) self->finish_error(ec.message()); return; }
                self->do_handshake();
            });
    }

    void do_handshake()
    {
        auto self = shared_from_this();
        self->stream_->async_handshake(ssl::stream_base::client,
            [self](beast::error_code ec) {
                if (ec || self->cancelled_) { if (ec) self->finish_error(ec.message()); return; }
                self->do_write_request();
            });
    }

    void do_write_request()
    {
        auto self = shared_from_this();
        http::request<http::string_body> req;
        req.method(http::verb::post);
        req.target("/chat/completions");
        req.version(11);
        req.set(http::field::host, self->host_);
        req.set(http::field::content_type, "application/json");
        req.set(http::field::connection, "close");
        if (!self->auth_.empty()) req.set(http::field::authorization, self->auth_);
        req.body() = self->body_;
        req.prepare_payload();
        self->req_ = std::move(req);

        http::async_write(*self->stream_, self->req_,
            [self](beast::error_code ec, std::size_t) {
                if (ec || self->cancelled_) { if (ec) self->finish_error(ec.message()); return; }
                self->do_read_header();
            });
    }

    void do_read_header()
    {
        auto self = shared_from_this();
        http::async_read_header(*self->stream_, self->buf_, self->parser_,
            [self](beast::error_code ec, std::size_t) {
                if (ec || self->cancelled_) { if (ec) self->finish_error(ec.message()); return; }
                int status = self->parser_.get().result_int();
                if (status != 200) {
                    http::async_read(*self->stream_, self->buf_, self->parser_,
                        [self](beast::error_code ec2, std::size_t) {
                            std::string msg = "HTTP " + std::to_string(
                                self->parser_.get().result_int());
                            if (!ec2) msg += ": " + self->parser_.get().body();
                            self->finish_error(msg);
                        });
                    return;
                }
                self->parser_.body_limit(std::numeric_limits<std::uint64_t>::max());
                self->prev_body_size_ = 0;
                self->do_read_body();
            });
    }

    void do_read_body()
    {
        auto self = shared_from_this();
        http::async_read_some(*self->stream_, self->buf_, self->parser_,
            [self](beast::error_code ec, std::size_t) {
                if (self->cancelled_) return;
                if (ec == http::error::end_of_stream) {
                    self->drain_remaining();
                    self->finish_success();
                    return;
                }
                if (ec) { self->finish_error(ec.message()); return; }
                self->drain_remaining();
                if (self->parser_.is_done()) {
                    self->finish_success();
                } else {
                    self->do_read_body();
                }
            });
    }

    void drain_remaining()
    {
        auto& body = parser_.get().body();
        size_t prev = prev_body_size_;
        prev_body_size_ = body.size();
        if (prev < body.size()) {
            std::string chunk = body.substr(prev);
            process_sse(chunk);
        }
    }

    void process_sse(const std::string& chunk)
    {
        leftover_ += chunk;
        size_t pos;
        while ((pos = leftover_.find("\n\n")) != std::string::npos) {
            std::string event = leftover_.substr(0, pos);
            leftover_.erase(0, pos + 2);
            std::istringstream ss(event);
            std::string line;
            while (std::getline(ss, line)) {
                if (line.rfind("data: ", 0) != 0) continue;
                std::string data = line.substr(6);
                if (data == "[DONE]") continue;
                try {
                    auto val = boost::json::parse(data);
                    auto& choices = val.as_object().at("choices").as_array();
                    if (choices.empty()) continue;
                    auto& delta = choices[0].as_object().at("delta").as_object();

                    if (delta.contains("reasoning_content") &&
                        delta.at("reasoning_content").is_string()) {
                        std::string r = delta.at("reasoning_content").as_string().c_str();
                        if (!r.empty()) {
                            full_reasoning_ += r;
                            LlmEvent ev;
                            ev.type = "reasoning";
                            ev.text = r;
                            if (on_event_) on_event_(std::move(ev));
                        }
                    }
                    if (delta.contains("content") &&
                        delta.at("content").is_string()) {
                        std::string c = delta.at("content").as_string().c_str();
                        if (!c.empty()) {
                            full_response_ += c;
                            LlmEvent ev;
                            ev.type = "delta";
                            ev.text = std::move(c);
                            if (on_event_) on_event_(std::move(ev));
                        }
                    }
                } catch (...) {}
            }
        }
    }

    void stop_timer()
    {
        timer_.cancel();
    }

    void finish_success()
    {
        stop_timer();
        if (cancelled_) return;
        LlmEvent ev;
        ev.type = "stream_end";
        if (on_event_) on_event_(std::move(ev));
        if (on_done_) on_done_(std::move(full_response_), std::move(full_reasoning_));
    }

    void finish_error(const std::string& msg)
    {
        stop_timer();
        if (cancelled_) return;
        LlmEvent ev;
        ev.type = "stream_end";
        if (!msg.empty()) ev.msg = msg;
        if (on_event_) on_event_(std::move(ev));
        if (on_done_) on_done_(std::move(full_response_), std::move(full_reasoning_));
    }
};

// ---- LlmClient public API ----

LlmClient::LlmClient(boost::asio::io_context& io)
    : impl_(std::make_shared<Impl>(io))
{}

LlmClient::~LlmClient()
{
    impl_->cancel();
}

void LlmClient::start(const std::string& host, const std::string& port,
                      const std::string& body, const std::string& auth,
                      LlmEventFn on_event, LlmDoneFn on_done,
                      std::chrono::seconds timeout)
{
    impl_->start(host, port, body, auth, std::move(on_event), std::move(on_done), timeout);
}

void LlmClient::cancel()
{
    impl_->cancel();
}
