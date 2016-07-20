#include <coco/coco.h>


class WebServerTask : public coco::TaskContext
{
public:


	coco::InputPort<std::string> in_message();
private:


};
COCO_REGISTER(WebServerTask)
