#include "util/logging.h"
#include "util/timing.h"

#include "web_server.h"
#include <json/json.h>

#ifndef COCO_DOCUMENT_ROOT
#warning "COCO_DOCUMENT_ROOT has not been defined !"
#define COCO_DOCUMENT_ROOT	"/usr/share/coco"
#endif

namespace coco
{

const std::string WebServer::DOCUMENT_ROOT = COCO_DOCUMENT_ROOT;
const std::string WebServer::SVG_FILE = "graph";

void WebServer::sendString(struct mg_connection *conn, const std::string& msg)
{
	mg_printf(conn, "%s",
			"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
	mg_printf_http_chunk(conn, msg.c_str());
	mg_send_http_chunk(conn, "", 0);
}

static std::string format(double v)
{
	char str[64];
	snprintf(str, sizeof(str), "%.6lf", v);
	return std::string(str);
}

static const std::string TaskStateDesc[] =
{ "INIT", "PRE_OPERATIONAL", "RUNNING", "IDLE", "STOPPED" };

void WebServer::eventHandler(struct mg_connection* nc, int ev, void * ev_data)
{
	WebServer* ws = (WebServer *) nc->mgr->user_data;
	switch (ev)
	{
	case MG_EV_HTTP_REQUEST:
	{
		struct http_message* hm = (struct http_message*) ev_data;
		if (mg_vcmp(&hm->method, "POST") == 0)
		{
			Json::Value root;
			Json::Value& data = root["data"];
			for (auto& v : ComponentRegistry::tasks())
			{
				if (std::dynamic_pointer_cast<PeerTask>(v.second))
					continue;
				Json::Value jtask;
				jtask["name"] = v.first;
				jtask["iterations"] = COCO_TIME_COUNT(v.first)
				;
				jtask["state"] = TaskStateDesc[v.second->state()];
				jtask["time_mean"] = format(COCO_TIME_MEAN(v.first));
				jtask["time_stddev"] = format(COCO_TIME_VARIANCE(v.first));
				jtask["time_exec_mean"] = format(COCO_SERVICE_TIME(v.first));
				jtask["time_exec_stddev"] = format(
						COCO_SERVICE_TIME_VARIANCE(v.first));
				jtask["time_min"] = format(COCO_MIN_TIME(v.first));
				jtask["time_max"] = format(COCO_MAX_TIME(v.first));
				data.append(jtask);
			}
			Json::StreamWriterBuilder builder;
			builder["commentStyle"] = "None";
			builder["indentation"] = "";
			std::unique_ptr<Json::StreamWriter> writer(
					builder.newStreamWriter());
			std::stringstream json;
			writer->write(root, &json);
			ws->sendString(nc, json.str());
		}
		else if (mg_vcmp(&hm->uri, SVG_FILE.c_str()) == 0)
		{
			ws->sendString(nc, ws->graph_svg_);
		}
		else
		{
			mg_serve_http(nc, hm, ws->http_server_opts_);
		}
	}
		break;
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
	stop_server_ = false;
	graph_loader_ = loader;
	char* temp = strdup("COCO_XXXXXX");
	int fd = mkstemp(temp);
	close(fd);
	if (!graph_loader_->writeSVG(std::string(temp)))
		return false;
	std::string svgfile = std::string(temp) + ".svg";
	std::stringstream s;
	std::ifstream f(svgfile);
	while (!f.eof())
	{
		char c;
		f >> c;
		s << c;
	}
	graph_svg_ = s.str();
	remove(temp);
	remove((std::string(temp) + ".dot").c_str());
	remove(svgfile.c_str());
	free(temp);
	//(DOCUMENT_ROOT + "/" + SVG_FILE).c_str());
	http_server_opts_.document_root = DOCUMENT_ROOT.c_str();
	mg_mgr_init(&mgr_, this);
	mg_connection_ = mg_bind(&mgr_, std::to_string(port).c_str(), eventHandler);
	if (mg_connection_ == NULL)
	{
		return false;
	}
	mg_set_protocol_http_websocket(mg_connection_);
	server_thread_ = std::thread(&WebServer::run, this);
	return true;
}

void WebServer::run()
{
	while (!stop_server_)
	{
		mg_mgr_poll(&mgr_, 100);
	}
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
