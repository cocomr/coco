
#include <coco/coco.h>

class Task1 : public coco::TaskContext
{
public:
    void init()
    {}

    void onConfig()
    {}

    void onUpdate()
    {
        COCO_LOG(1) << this->instantiationName() << " " << initial_value;
        out_value_.write(initial_value++);
    }


    coco::OutputPort<int> out_value_ = {this, "value_OUT"};

private:
    int initial_value = 0;
};
COCO_REGISTER(Task1)

class Task2 : public coco::TaskContext
{
public:
    void init()
    {}

    void onConfig()
    {}

    void onUpdate()
    {
        int value;
        if (in_value_.read(value) == coco::NEW_DATA)
            out_value_.write(value);

        #ifndef WIN32
sleep(1);
#else
#endif
    }

    coco::InputPort<int> in_value_ = {this, "value_IN", true};
    coco::OutputPort<int> out_value_ = {this, "value_OUT"};

private:
    int initial_value = 0;
};
COCO_REGISTER(Task2)

class Task3 : public coco::TaskContext
{
public:
    void init()
    {}

    void onConfig()
    {}

    void onUpdate()
    {
        int value;
        if (in_value_.read(value) == coco::NEW_DATA)
            out_value_.write(value);
    }

    coco::InputPort<int> in_value_ = {this, "value_IN", true};
    coco::OutputPort<int> out_value_ = {this, "value_OUT"};

private:
    int initial_value = 0;
};
COCO_REGISTER(Task3)

class Task4 : public coco::TaskContext
{
public:
    void init()
    {}

    void onConfig()
    {}

    void onUpdate()
    {
        int value;
        if (in_value_.read(value) == coco::NEW_DATA)
            out_value_.write(value);
    }

    coco::InputPort<int> in_value_ = {this, "value_IN", true};
    coco::OutputPort<int> out_value_ = {this, "value_OUT"};

private:
    int initial_value = 0;
};
COCO_REGISTER(Task4)

class Task5 : public coco::TaskContext
{
public:
    void init()
    {}

    void onConfig()
    {}

    void onUpdate()
    {
        int value = -1;
        in_value_.read(value);
        COCO_LOG(2) << this->instantiationName() << " " << value;

    }

    coco::InputPort<int> in_value_ = {this, "value_IN", true};

private:
    int initial_value = 0;
};

COCO_REGISTER(Task5)
