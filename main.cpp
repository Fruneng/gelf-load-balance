#include <iostream>
#include <ctime>
#include <boost/array.hpp>  
#include <boost/asio.hpp>  
#include <boost/bind.hpp>
#include <boost/property_tree/ptree.hpp>  
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>

#include "protocol.hpp"
#include "upstream.hpp"

using namespace boost::asio::ip;
using namespace boost::property_tree; 

namespace UdpAsyService  
{  
    class udp_server  
    {  
    public:  
        udp_server(
            boost::asio::io_service& io,
            udp::endpoint endpoint,
            ptree& upstreams)  
            :socket_(io, endpoint),
             protocol_(boost::bind(&udp_server::hand_chunk, this, _1)),
             upstream_(upstreams)
        {
            start_recive();  
        }
  
        void start_recive()  
        {  
            socket_.async_receive_from(boost::asio::buffer(rec_buf_), remot_endpoint_,  
                boost::bind(&udp_server::hand_receive, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));  
        }

        void hand_chunk(BufferStack buffer_stack) 
        {
            // TODO 
            std::cout << "hand_chunk: " << buffer_stack.size() << std::endl;
        }
  
    private:  
        void hand_receive(const boost::system::error_code& error, std::size_t size)  
        {  
            if (!error || error != boost::asio::error::message_size)  
            {
                SharedBuffer copy = SharedBuffer(new Buffer());
                boost::swap(rec_buf_, *copy);
                if ((*copy)[0] == 0x78 || (*copy)[0] == 0x1f) {
                    udp::endpoint &up_endpoint = upstream_.get();
                    socket_.async_send_to(boost::asio::buffer(*copy, size), up_endpoint,  
                        boost::bind(&udp_server::hand_send, this, copy, size, boost::asio::placeholders::error,  
                            boost::asio::placeholders::bytes_transferred));  
                } else if ((*copy)[0] == 0x1e) {
                    protocol_.enStack(copy);
                }
                start_recive();//next client;  
            }  
        }

        void hand_send(SharedBuffer buff, size_t buff_len, const boost::system::system_error& error, std::size_t size)  
        {  
            std::cout << "send byte: " << buff_len << std::endl;
        }  
  
    private:  
        udp::socket socket_;  
        udp::endpoint remot_endpoint_;
        Buffer rec_buf_;
        Protocol protocol_;
        Upstream upstream_;
    };  
  
    void udp_asy_server(
            udp::endpoint endpoint,
            ptree& upstreams)  
    {  
        try  
        {  
            boost::asio::io_service io;  
            udp_server server(io, endpoint, upstreams);  
  
            io.run();  
        }  
        catch (std::exception& e)  
        {  
            std::cerr << e.what() << std::endl;  
        }  
    }  
}


int main(int argc, char* argv[])
{
    std::string filename = "/etc/lss.conf.d/gelf-load-balance.json";
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-f") == 0) {
            filename = argv[i+1];
            break;
        }
    }
    std::cout << filename << std::endl;
    ptree pt;
    read_json(filename, pt);

    std::vector<udp::endpoint> endpoints;

    ptree upstream = pt.get_child("upstream");

    std::string host = pt.get_child("host").get_value<std::string>();
    int port = pt.get_child("port").get_value<int>();

    udp::endpoint endpoint(address_v4::from_string(host), port);

    UdpAsyService::udp_asy_server(endpoint, upstream);
    return 0;
}