#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <sstream>
#include <deque>

#include "json/json.h"
#include "mongoose/mongoose.h"

#include "coco/register.h"
#include "coco/util/logging.h"
#include "coco/util/timing.h"
#include "coco/web_server/web_server.h"

#ifndef COCO_DOCUMENT_ROOT
#define COCO_DOCUMENT_ROOT    "."
#endif

namespace coco
{

class WebServer::WebServerImpl
{
public:
    bool start(unsigned port, const std::string& appname,
            const std::string& graph_svg, const std::string& web_server_root);
    void stop();
    bool isRunning() const;
    void sendStringWebSocket(const std::string &msg);
    void sendStringHttp(struct mg_connection *conn, const std::string& type,
            const std::string &msg);
    void addLogString(const std::string &msg);

private:
    void run();
    static void eventHandler(struct mg_connection * nc, int ev, void * ev_data);
    std::string buildJSON();

    static const std::string SVG_URI;

    struct mg_serve_http_opts http_server_opts_;
    struct mg_mgr mgr_;
    struct mg_connection * mg_connection_ = nullptr;

    std::thread server_thread_;
    std::atomic<bool> stop_server_;

    std::string appname_;
    std::string document_root_;
    std::string graph_svg_;
    std::mutex log_mutex_;
    std::stringstream log_stream_;
/*
    struct Stats
    {
    	uint window = 50;
    	std::unordered_map<std::string, std::deque<double>> samples_;

    	std::deque<double>& operator[](const std::string& key)
    	{
    		return samples_[key];
    	}

    	void put(const std::string& key, double v)
    	{
    		auto& queue = samples_[key];
    		if (queue.size() > 0)
    			queue[0] = v;
    		else
    			queue.push_back(v);
    	}

    	void add(const std::string& key, double v)
    	{
    		auto& queue = samples_[key];
    		if (queue.size() > window)
    		{
    			queue.pop_front();
    		}
    		queue.push_back(v);
    	}

        void toJSONArray(Json::Value& node)
        {
            for (auto& sample : samples_)
            {
                Json::Value g;
                g["name"] = sample.first;
                Json::Value& data = g["data"];
                for (auto& v : sample.second)
                	data.append(v);
                node.append(g);
            }
        }
    };
    Stats stats_;
*/
};

const std::string WebServer::WebServerImpl::SVG_URI = "/graph.svg";

WebServer::WebServer()
{
    impl_ptr_.reset(new WebServerImpl());
}

WebServer & WebServer::instance()
{
    static WebServer instance;
    return instance;
}

bool WebServer::start(unsigned port, const std::string& appname,
        const std::string& graph_svg, const std::string& web_server_root)
{
    return instance().impl_ptr_->start(port, appname, graph_svg,
            web_server_root);
}
bool WebServer::WebServerImpl::start(unsigned port, const std::string& appname,
        const std::string& graph_svg, const std::string& web_server_root)
{
    if (web_server_root.empty())
        document_root_ = COCO_DOCUMENT_ROOT;
    else
        document_root_ = web_server_root;
    COCO_LOG(0)<< "Document root is " << document_root_;

    appname_ = appname;
    graph_svg_ = graph_svg;
    stop_server_ = false;

    http_server_opts_.document_root = document_root_.c_str();
    mg_mgr_init(&mgr_, this);
    mg_connection_ = mg_bind(&mgr_, std::to_string(port).c_str(), eventHandler);
    if (mg_connection_ == NULL)
    {
        return false;
    }
    mg_set_protocol_http_websocket(mg_connection_);
    server_thread_ = std::thread(&WebServer::WebServerImpl::run, this);
    return true;
}

void WebServer::stop()
{
    instance().impl_ptr_->stop();
}
void WebServer::WebServerImpl::stop()
{
    stop_server_ = true;
}

bool WebServer::isRunning()
{
    return instance().impl_ptr_->isRunning();
}
bool WebServer::WebServerImpl::isRunning() const
{
    return mg_connection_ != nullptr;
}

void WebServer::WebServerImpl::sendStringHttp(struct mg_connection *conn,
        const std::string &type, const std::string& msg)
{
    mg_printf(conn,
            "HTTP/1.1 200 OK\r\nContent-type: %s\nTransfer-Encoding: chunked\r\nAccess-Control-Allow-Origin: *\r\n\r\n",
            type.c_str());
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

static const std::string SchedulePolicyDesc[] =
{ "PERIODIC", "HARD", "TRIGGERED" };

std::string WebServer::WebServerImpl::buildJSON()
{
    Json::Value root;
    Json::Value& info = root["info"];
    info["project_name"] = appname_;
    {
        std::unique_lock<std::mutex>(log_mutex_);
        root["log"] = log_stream_.str();
        log_stream_.str("");
    }
    Json::Value& acts = root["activities"];
    int acti = 0;
    for (auto& v : ComponentRegistry::activities())
    {
        Json::Value jact;
        jact["id"] = acti++;
        jact["active"] = v->isActive() ? "Yes" : "No";
        jact["periodic"] = v->isPeriodic() ? "Yes" : "No";
        jact["period"] = v->period();
        jact["policy"] = SchedulePolicyDesc[v->policy().scheduling_policy];
        acts.append(jact);
    }
    Json::Value& tasks = root["tasks"];
    for (auto& v : ComponentRegistry::tasks())
    {
        Json::Value jtask;
        jtask["name"] = v.second->instantiationName();
        jtask["class"] = v.second->name();
        jtask["type"] =
                std::dynamic_pointer_cast<PeerTask>(v.second) ? "Peer" : "Task";
        jtask["state"] = TaskStateDesc[v.second->state()];
        tasks.append(jtask);
    }
    Json::Value& stats = root["stats"];
    for (auto& v : ComponentRegistry::tasks())
    {
        if (std::dynamic_pointer_cast<PeerTask>(v.second))
            continue;
        Json::Value jtask;
        auto name = v.second->instantiationName();
        auto tm = COCO_TIME_MEAN(v.first);
        auto tv = COCO_TIME_VARIANCE(v.first);
        jtask["name"] = name;
        jtask["iterations"] = COCO_TIME_COUNT(v.first);
        jtask["time"] = COCO_TIME(v.first);
        jtask["time_mean"] = format(tm);
        jtask["time_stddev"] = format(tv);
        jtask["time_exec_mean"] = format(COCO_SERVICE_TIME(v.first));
        jtask["time_exec_stddev"] = format( COCO_SERVICE_TIME_VARIANCE(v.first));
        jtask["time_min"] = format(COCO_MIN_TIME(v.first));
        jtask["time_max"] = format(COCO_MAX_TIME(v.first));
        stats.append(jtask);
/*
        stats_.add("t_" + name, COCO_TIME(v.first));
        stats_.put("tm_" + name, tm);
        stats_.put("tv_" + name, tv);
*/
    }
//    stats_.toJSONArray(root["graphs"]);

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::stringstream json;
    writer->write(root, &json);
    return json.str();
}

void WebServer::WebServerImpl::eventHandler(struct mg_connection* nc, int ev,
        void * ev_data)
{
    auto ws = (WebServer::WebServerImpl*) nc->mgr->user_data;
    switch (ev)
    {
    case MG_EV_HTTP_REQUEST:
    {
        struct http_message* hm = (struct http_message*) ev_data;
        if (mg_vcmp(&hm->method, "POST") == 0)
        {
            if (mg_vcmp(&hm->uri, "/info") == 0)
            {
                ws->sendStringHttp(nc, "text/json", ws->buildJSON());
            }
            else if (mg_vcmp(&hm->body, "action=reset_stats") == 0)
            {
                COCO_RESET_TIMERS;
                ws->sendStringHttp(nc, "text/plain", "ok");
            }
            else
            {
                ws->sendStringHttp(nc, "text/json", ws->buildJSON());
            }
        }
        else if (mg_vcmp(&hm->method, "GET") == 0
                && mg_vcmp(&hm->uri, SVG_URI.c_str()) == 0)
        {
            ws->sendStringHttp(nc, "text/svg", ws->graph_svg_);
        }
        else
        {
            mg_serve_http(nc, hm, ws->http_server_opts_);
        }
    }
    break;
    case MG_EV_POLL:
    if (nc->flags & MG_F_IS_WEBSOCKET)
    {
    	// TODO do only every x milliseconds, x >= mg_mgr_poll's polling time
        const auto& json = ws->buildJSON();
        mg_send_websocket_frame(nc, WEBSOCKET_OP_TEXT, json.c_str(), json.size());
    }
    break;
    default:
    break;
}
}

void WebServer::WebServerImpl::run()
{
    while (!stop_server_)
    {
        mg_mgr_poll(&mgr_, 100);
    }
    mg_mgr_free(&mgr_);
}

void WebServer::addLogString(const std::string &msg)
{
    instance().impl_ptr_->addLogString(msg);
}

void WebServer::WebServerImpl::addLogString(const std::string &msg)
{
    std::unique_lock<std::mutex> lock(log_mutex_);

    log_stream_ << msg;
}

}  // end of namespace coco
