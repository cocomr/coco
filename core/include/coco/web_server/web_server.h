/**
 * Project: CoCo
 * Copyright (c) 2016, Scuola Superiore Sant'Anna
 *
 * Authors: Filippo Brizzi <fi.brizzi@sssup.it>, Emanuele Ruffaldi
 * 
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 */

#pragma once
#include <memory>
#include <string>

#include "coco/platform.h"
namespace coco
{

class COCO_EXPORT WebServer
{
public:
	static bool start(unsigned port, const std::string& appname,
			const std::string& graph_svg, const std::string& web_server_root);
	static void stop();
	static bool isRunning();
	static void addLogString(const std::string &msg);

private:
	WebServer();
	static WebServer& instance();

	class WebServerImpl;
	std::unique_ptr<WebServerImpl> impl_ptr_;

};

}
