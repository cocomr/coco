#pragma once
#include <memory>
#include <string>

namespace coco
{

class WebServer
{
public:
	static bool start(unsigned port, const std::string& graph_svg,
			const std::string& web_server_root);
	static void stop();
    static bool isRunning();
    static void sendStringWebSocket(const std::string &msg);

private:
    WebServer();
    static WebServer& instance();

    class WebServerImpl;
    std::unique_ptr<WebServerImpl> impl_ptr_;
};

}
