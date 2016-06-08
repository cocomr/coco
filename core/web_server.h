#pragma once

#include <thread>
#include <atomic>

#include "mongoose/mongoose.h"
//#include "graph_loader.h"

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
	WebServer()
	{

	}

	bool startImpl(unsigned port, const std::string& graph_svg,
			const std::string &web_server_root);
	void run();
	void stopImpl();
	bool isRunningImpl() const;
	void sendStringHttp(struct mg_connection *conn, const std::string& type,
			const std::string &msg);
	void sendStringWebSocketImpl(const std::string &msg);

	static WebServer& instance();
	static void eventHandler(struct mg_connection * nc, int ev, void * ev_data);

	static const std::string SVG_URI;

	struct mg_serve_http_opts http_server_opts_;
	struct mg_mgr mgr_;
	struct mg_connection * mg_connection_ = nullptr;

	std::thread server_thread_;
	std::atomic<bool> stop_server_;

	std::string document_root_;
	std::string graph_svg_;
};

}
