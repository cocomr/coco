#include "util/logging.h"
#include "util/timing.h"

#include "web_server.h"


namespace coco
{

void sendString(struct mg_connection *conn, const std::string &msg)
{
    mg_printf(conn, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nAccess-Control-Allow-Origin: *\r\n\r\n");

    mg_printf_http_chunk(conn, msg.c_str());
    mg_send_http_chunk(conn, "", 0);
}


void eventHandler(struct mg_connection * nc, int ev, void * ev_data)
{
    WebServer* web_server = (WebServer *)nc->mgr->user_data;

    switch(ev)
    {
        case MG_EV_HTTP_REQUEST:
        {
            struct http_message * hm = (struct http_message *)ev_data;
            std::string method(hm->method.p, hm->method.len);

            if (method != "POST")
                return;

            std::string uri(hm->uri.p, hm->uri.len);

            // Iterator to every word separate by /
            auto splitter = stringutil::splitter(uri, '/');
            auto uri_it = splitter.begin();



            //if bad
            //mg_printf(nc, "%s", "HTTP/1.1 400 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
            //mg_send_http_chunk(nc, "", 0);

        }
        default:
            break;
    }
}

WebServer &WebServer::instance()
{
    static WebServer instance;
    return instance;
}

bool WebServer::start(unsigned port, std::shared_ptr<GraphLoader> loader)
{
    return instance().startImpl(port, loader);
}

bool WebServer::startImpl(unsigned port, std::shared_ptr<GraphLoader> loader)
{
    port_ = port;
    graph_loader = loader;
    stop_server_ = false;
    server_thread_ = std::thread(&WebServer::run, this);
}

void WebServer::run()
{
    mg_mgr_init(&mgr_, this);
    mg_connection_ = mg_bind(&mgr_, std::to_string(port_).c_str(), eventHandler);
    if (mg_connection_ == NULL)
    {
        COCO_FATAL() << "Failed to initialize server on port: " << port_ << std::endl;
        return;
    }
    mg_set_protocol_http_websocket(mg_connection_);

    while(!stop_server_)
        mg_mgr_poll(&mgr_, 100);

    mg_mgr_free(&mgr_);
}

void WebServer::stop()
{
    instance().stopImpl();
}
void WebServer::stopImpl()
{
    stop_server_ = true;
}

}
