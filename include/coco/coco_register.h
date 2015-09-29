#pragma once

#include "coco_core.hpp"
namespace coco
{

/**
	Specification of the component, as created by the macro.
	This class contains the name of the component plus the function to be called to 
	instantiate it (fx_)
*/
class ComponentSpec
{
public:
	using make_fx_t = std::function<TaskContext * ()>;

	/// instantiate a specification with name and virtual ctor
	ComponentSpec(const std::string &classname, const std::string &name, make_fx_t fx);

	std::string classname_;
	std::string name_;
	make_fx_t fx_;
};


struct TypeSpec
{
	const char * name_; // pure name
	std::function<bool(std::ostream&,void*)> out; // conversion fx
	const std::type_info & ti; // internal name (unique)

	TypeSpec(const char * xname,const std::type_info & xti, std::function<bool(std::ostream&,void*)>  xout);
};



/**
 * Component Registry that is singleton per each exec or library. Then when the component library is loaded 
 * the singleton is replaced
 */
class ComponentRegistry
{
public:
	/// creates a component by name
	static TaskContext * create(const std::string &name, const std::string &instantiation_name);
	/// adds a specification that will be used to retreive the task
	static void addSpec(ComponentSpec * s);
	/// adds a type specification
	static void addType(TypeSpec * s);
	/// adds a library
	static bool addLibrary(const std::string &l, const std::string &path );
	/// defines an alias. note that old_name should be present
	static void alias(const std::string &new_name, const std::string &old_name);
	/// iterate over the names of the components
    static impl::map_keys<std::string, ComponentSpec *> componentsName();
    /// Allow to retreive any task by its instantiation name. This function is usable by any task
    static TypeSpec *type(std::string name);
    static TypeSpec *type(const std::type_info & ti);

    template <class T>
    static TypeSpec *type()
    {
    	return type(typeid(T).name());
    }

    static TaskContext *task(std::string name);
    static impl::map_values<std::string, TaskContext *> tasks();

private:
	static ComponentRegistry & get();

	TaskContext * createImpl(const std::string &name, const std::string &instantiation_name);
	void addSpecImpl(ComponentSpec *s);
	void addTypeImpl(TypeSpec *s);
	bool addLibraryImpl(const std::string &lib, const std::string &path );
	void aliasImpl(const std::string &newname, const std::string &oldname);
    impl::map_keys<std::string, ComponentSpec *> componentsNameImpl();
	TaskContext *taskImpl(std::string name);
	TypeSpec *typeImpl(std::string name);
	TypeSpec *typeImpl(const std::type_info & ti);
	impl::map_values<std::string, TaskContext *> tasksImpl();

	std::map<std::string, ComponentSpec*> specs_;
	std::map<std::string, TypeSpec*> typespecs_; // conflict less
	std::map<std::uintptr_t, TypeSpec*> typespecs2_; // with conflicts
	std::set<std::string> libs_;
	// std::map<std::string, std::shared_ptr<TaskContext> > tasks_; // TODO but using mutex for protection
	std::map<std::string, TaskContext *> tasks_; /// Contains all the tasks created and it is accessible by every component
};


} // end of namespace coco


/// registration
#define COCO_TYPE(T) \
	coco::TypeSpec T##_spec = coco::TypeSpec ( #T, typeid(T), \
		[] (std::ostream & ons, void * p) -> bool { \
			ons << *(T*)p;\
			return (bool)ons;\
		}  \
	);

/// registration
#define COCO_TYPE_NAMED(T,name) \
	coco::TypeSpec name##_spec = coco::TypeSpec ( #T, typeid(T), \
		[] (std::ostream & ons, void * p) -> bool { \
			ons << *(T*)p;\
			return (bool)ons;\
		}  \
	);

/// registration macro, the only thing needed
#define COCO_REGISTER(T) \
    coco::ComponentSpec T##_spec = { #T, #T, [] () -> coco::TaskContext* {return new T(); } };\
    extern "C" const char * T##_coco_name = #T;\
    extern "C" coco::TaskContext* T##_coco_make() { return new T(); }

#define COCO_REGISTER_NAMED(T,name) \
    coco::ComponentSpec T##_spec = { #T, name, [] () -> coco::TaskContext* {return new T(); } };\
    extern "C" const char * T##_coco_name = name;\
    extern "C" coco::TaskContext* T##_coco_make() { return new T(); }

