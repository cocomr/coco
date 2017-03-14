/**
 * Project: CoCo
 * Copyright (c) 2016-2017, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi <e.ruffaldi@sssup.it>
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#include <string>
#include <vector>
#include <atomic>
#include <sstream>
#include <deque>
#include "coco/util/threading.h"
#include "coco/util/accesses.hpp"

#include "json/json.h"
#include "mongoose/mongoose.h"

#include "coco/register.h"

#ifndef COCO_DOCUMENT_ROOT
#define COCO_DOCUMENT_ROOT    "."
#endif

namespace coco
{

static std::string makeJSON(Json::Value &root)
{
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::stringstream json;
    writer->write(root, &json);
    return json.str();    
}

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
    void sendError(struct mg_connection *conn, int code, std::string msg);
    void addLogString(const std::string &msg);

private:
    bool handleTask(struct mg_connection* nc,std::shared_ptr<TaskContext> pt, coco::util::split_iterator it, coco::util::split_iterator ite);

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

void WebServer::WebServerImpl::sendError(struct mg_connection *conn, int code, std::string msg)
{
    mg_printf(conn,
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %d\r\n"
            "Connection: close\r\n"
            "\r\n%s", code, msg.c_str(), msg.size(),msg.c_str());       
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

bool WebServer::WebServerImpl::handleTask(struct mg_connection* nc,std::shared_ptr<TaskContext> pt, coco::util::split_iterator si, coco::util::split_iterator se)
{
    auto ws = (WebServer::WebServerImpl*) nc->mgr->user_data;
    ++si;
    if(si != se)
    {
        if(*si == "call")
        {
            ++si;
            if(si != se)
            {
                // call!
                std::string op = *si++;
                bool r;
                if(si != se)
                {
                    std::string arg = *si;
                    r = pt->enqueueOperation<void(std::string)>(op,arg);
                }
                else
                {
                    r = pt->enqueueOperation<void(void)>(op);                                    
                }
                if(r)
                    ws->sendError(nc, 200,"done");
                else
                    ws->sendError(nc, 500,"error");
            }
            else
            {
                // list of methods
                auto & ops = pt->operations();
                Json::Value root;
                root["task"] = pt->name();
                Json::Value& tasks = root["operations"];
                for (auto& op : ops)
                    tasks.append(op.first);

                ws->sendStringHttp(nc, "text/json", makeJSON(root));
            }                            
        }
        // NOTE: we should have init-only attributes
        else if(*si == "get")
        {
            ++si;
            if(si != se)
            {
                std::shared_ptr<AttributeBase> at = pt->attribute(*si);
                if(at)
                {
                    ws->sendStringHttp(nc,"text/plain",at->toString());
                }
                else
                    ws->sendError(nc, 404,"not found");
            }
            else
            {
                Json::Value root;
                root["task"] = pt->name();
                Json::Value& atts = root["attributes"];
                for(auto & l: pt->attributes())
                {
                    atts.append(l.first);
                }
                ws->sendStringHttp(nc, "text/json", makeJSON(root));
            }
        }
        // NOTE: we should have init-only attributes
        else if(*si == "set")
        {
            ++si;
            if(si != se)
            {
                std::shared_ptr<AttributeBase> at = pt->attribute(*si++);
                if(at)
                {
                    if(si != se)
                    {
                        at->setValue(*si);
                        ws->sendError(nc, 200,"done");                                                
                    }
                    else
                    {
                        ws->sendError(nc, 400,"missing value");                                                
                    }
                }
                else
                {
                    ws->sendError(nc, 404,"not found");
                }
            }
            else
            {
                Json::Value root;
                root["task"] = pt->name();
                Json::Value& atts = root["attributes"];
                for(auto & l: pt->attributes())
                {
                    atts.append(l.first);
                }
                ws->sendStringHttp(nc, "text/json", makeJSON(root));
            }
        }                                
        else if(*si == "peers")
        {
            Json::Value root;
            root["task"] = pt->name();
            Json::Value& s = root["peers"];
            for(auto & l: pt->peers())
            {
                s.append(l->name());
            }
            ws->sendStringHttp(nc, "text/json", makeJSON(root));            
        }
        else
        {
            for(auto & l: pt->peers())
            {
                if(l->name() == *si)
                {
                    ws->handleTask(nc,l,si,se);
                    return true;
                }
            }

            ws->sendError(nc, 404,"only: call,get,set,peers or PEER name");   
        }
    }
    return true;
}

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
        jtask["state"] = TaskStateDesc[static_cast<int>(v.second->state())];
        tasks.append(jtask);
    }
    Json::Value& taskspecs = root["taskspecs"];
    int itaskspecs = 0;
    //const std::unordered_map<std::string, ComponentSpec*> &
    for (auto& v : ComponentRegistry::components())
    {
        Json::Value jcs;
        jcs["id"] = itaskspecs++;
        jcs["class"] = v.second->class_name_;
        jcs["name"] = v.first; // for alias, while second.name_ is originakl
        acts.append(jcs);
    }    
    Json::Value& stats = root["stats"];
    for (auto& task : ComponentRegistry::tasks())
    {
        if (std::dynamic_pointer_cast<PeerTask>(task.second))
            continue;
        Json::Value jtask;
        auto name = task.second->instantiationName();
        auto time = task.second->timeStatistics();

        auto tm = time.mean;
        auto tv = time.variance;
        jtask["name"] = name;
        jtask["iterations"] = static_cast<uint32_t>(time.iterations);
        jtask["time"] = format(time.elapsed);
        jtask["time_inst"] = format(time.last);
        jtask["time_mean"] = format(tm);
        jtask["time_stddev"] = format(tv);
        jtask["time_exec_mean"] = format(time.service_mean);
        jtask["time_exec_stddev"] = format(time.service_variance);
        jtask["time_min"] = format(time.min);
        jtask["time_max"] = format(time.max);
        stats.append(jtask);
    }

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
        bool dodefault = true;
        struct http_message* hm = (struct http_message*) ev_data;
        if (mg_vcmp(&hm->method, "POST") == 0)
        {
        	// handles commands from the web UI
            if (mg_vcmp(&hm->uri, "/info") == 0)
            {
                ws->sendStringHttp(nc, "text/json", ws->buildJSON());
            }
            else if (mg_vcmp(&hm->body, "action=reset_stats") == 0)
            {
                //  COCO_RESET_TIMERS;
                for (auto& task : ComponentRegistry::tasks())
                {
                    if ( std::dynamic_pointer_cast<PeerTask>(task.second))
                        continue;
                    task.second->resetTimeStatistics();
                }
                ws->sendStringHttp(nc, "text/plain", "ok");
            }
            else
            {
                ws->sendStringHttp(nc, "text/json", ws->buildJSON());
            }
            dodefault = false;
        }
        // TODO: call should be also POST so we can send full JSON
        else if (mg_vcmp(&hm->method, "GET") == 0)
        {
            if(mg_vcmp(&hm->uri, SVG_URI.c_str()) == 0)
            {
                ws->sendStringHttp(nc, "text/svg", ws->graph_svg_);
                dodefault = false;
            }
            else
            {
                // new operation system
                // /task/nodename/call/action
                coco::util::string_splitter ss(std::string(hm->uri.p+1,hm->uri.len-1),'/');
                auto se = ss.end();
                auto si = ss.begin();
                if(*si == "task")
                {
                    ++si;
                    dodefault = false;
                    if(si != se)
                    {
                        // lookup by name
                        auto pt = coco::ComponentRegistry::task(*si);
                        if(!pt)
                        {
                            ws->sendError(nc, 404,"missing task");
                        }
                        else
                        {
                            ws->handleTask(nc,pt,si,se);
                        }
                    }
                    else
                    {
                        // list of nodes
                        auto L = coco::ComponentRegistry::taskNames();
                        Json::Value root;
                        Json::Value& tasks = root["tasks"];
                        for (auto& l : L)
                            tasks.append(l);

                        ws->sendStringHttp(nc, "text/json", makeJSON(root));
                    }
                }

            }
        }
        if(dodefault)
        {
            mg_serve_http(nc, hm, ws->http_server_opts_);
        }
    }
    break;
    // MG_EV_POLL is generated by mongoose every mg_mgr_poll's polling time
    case MG_EV_POLL:
    if (nc->flags & MG_F_IS_WEBSOCKET)
    {
    	// TODO update json only every x milliseconds, x >= mg_mgr_poll's polling time
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
