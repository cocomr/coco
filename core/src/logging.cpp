#include "coco/util/logging.h"


namespace coco
{
namespace util
{
	static LoggerManager *xlog;
	        
	LoggerManager* LoggerManager::instance()
	{
		if(!xlog)
		{
			xlog = new LoggerManager();
		}
	    return xlog;
	}
}
}