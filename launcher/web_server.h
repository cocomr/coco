#pragma once

#include "mongoose/mongoose.h"
#include "graph_loader.h"

namespace coco
{



class WebServer
{
public:
    static bool start(unsigned port, std::shared_ptr<coco::GraphLoader> loader);
    static void stop();

private:
    WebServer()
    {}

    static WebServer &instance();

    bool startImpl(unsigned port, std::shared_ptr<coco::GraphLoader> loader);
    void run();
    void stopImpl();

public:
    std::shared_ptr<coco::GraphLoader> graph_loader;

private:
    unsigned port_;
    struct mg_mgr mgr_;
    struct mg_connection * mg_connection_;
    std::thread server_thread_;
    std::atomic<bool> stop_server_;
};

}
