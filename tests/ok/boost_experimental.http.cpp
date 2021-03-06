/*
local_settings:
    build:
        cxx_flags: -std=c++11
dependencies:
  pvt.cppan.demo.boost_experimental.http: master
  pvt.cppan.demo.boost.asio: 1.61
  pvt.cppan.demo.boost.coroutine: 1.61
*/

#include <iostream>
#include <algorithm>

#include <boost/utility/string_ref.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/http/buffered_socket.hpp>
#include <boost/http/algorithm.hpp>

using namespace std;
using namespace boost;

class connection: public std::enable_shared_from_this<connection>
{
public:
    void operator()(asio::yield_context yield)
    {
        auto self = shared_from_this();
        try {
            while (self->socket.is_open()) {
                cout << "--\n[" << self->counter
                     << "] About to receive a new message" << endl;
                self->socket.async_read_request(self->method, self->path,
                                                self->message, yield);
                //self->message.body().clear(); // freeing not used resources

                if (http::request_continue_required(self->message)) {
                    cout << '[' << self->counter
                         << "] Continue required. About to send"
                        " \"100-continue\""
                         << std::endl;
                    self->socket.async_write_response_continue(yield);
                }

                while (self->socket.read_state() != http::read_state::empty) {
                    cout << '[' << self->counter
                         << "] Message not fully received" << endl;
                    switch (self->socket.read_state()) {
                    case http::read_state::message_ready:
                        cout << '[' << self->counter
                             << "] About to receive some body" << endl;
                        self->socket.async_read_some(self->message, yield);
                        break;
                    case http::read_state::body_ready:
                        cout << '[' << self->counter
                             << "] About to receive trailers" << endl;
                        self->socket.async_read_trailers(self->message, yield);
                        break;
                    default:;
                    }
                }

                //cout << "BODY:==";
                //for (const auto &e: self->message.body()) {
                //    cout << char(e);
                //}
                //cout << "==" << endl;

                cout << '[' << self->counter << "] Message received. State = "
                     << int(self->socket.read_state()) << endl;
                cout << '[' << self->counter << "] Method: " << self->method
                     << endl;
                cout << '[' << self->counter << "] Path: " << self->path
                     << endl;
                {
                    auto host = self->message.headers().find("host");
                    if (host != self->message.headers().end())
                        cout << '[' << self->counter << "] Host header: "
                             << host->second << endl;
                }

                std::cout << '[' << self->counter << "] Write state = "
                          << int(self->socket.write_state()) << std::endl;

                cout << '[' << self->counter << "] About to send a reply"
                     << endl;

                http::message reply;
                //reply.headers().emplace("connection", "close");
                const char body[] = "Hello World\n";
                std::copy(body, body + sizeof(body) - 1,
                          std::back_inserter(reply.body()));

                self->socket.async_write_response(200, string_ref("OK"), reply,
                                                  yield);
            }
        } catch (system::system_error &e) {
            if (e.code() != system::error_code{asio::error::eof}) {
                cerr << '[' << self->counter << "] Aborting on exception: "
                     << e.what() << endl;
                std::exit(1);
            }

            cout << '[' << self->counter << "] Error: " << e.what() << endl;
        } catch (std::exception &e) {
            cerr << '[' << self->counter << "] Aborting on exception: "
                 << e.what() << endl;
            std::exit(1);
        }
    }

    asio::ip::tcp::socket &tcp_layer()
    {
        return socket.next_layer();
    }

    static std::shared_ptr<connection> make_connection(asio::io_service &ios,
                                                       int counter)
    {
        return std::shared_ptr<connection>{new connection{ios, counter}};
    }

private:
    connection(asio::io_service &ios, int counter)
        : socket(ios)
        , counter(counter)
    {}

    http::buffered_socket socket;
    int counter;

    std::string method;
    std::string path;
    http::message message;
};

int main()
{
    asio::io_service ios;
    asio::ip::tcp::acceptor acceptor(ios,
                                     asio::ip::tcp
                                     ::endpoint(asio::ip::tcp::v6(), 8080));

    auto work = [&acceptor](asio::yield_context yield) {
        int counter = 0;
        for ( ; true ; ++counter ) {
            try {
                auto connection
                    = connection::make_connection(acceptor.get_io_service(),
                                                  counter);
                cout << "About to accept a new connection" << endl;
                acceptor.async_accept(connection->tcp_layer(), yield);

                auto handle_connection
                    = [connection](asio::yield_context yield) mutable {
                    (*connection)(yield);
                };
                spawn(acceptor.get_io_service(), handle_connection);
            } catch (std::exception &e) {
                cerr << "Aborting on exception: " << e.what() << endl;
                std::exit(1);
            }
        }
    };

    cout << "About to spawn" << endl;
    spawn(ios, work);

    cout << "About to run" << endl;
    ios.run();

    return 0;
}