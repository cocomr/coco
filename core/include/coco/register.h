/**
Copyright 2015, Filippo Brizzi"
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

@Author 
Filippo Brizzi, PhD Student
fi.brizzi@sssup.it
Emanuele Ruffaldi, PhD
e.ruffaldi@sssup.it

PERCRO, (Laboratory of Perceptual Robotics)
Scuola Superiore Sant'Anna
via Luigi Alamanni 13D, San Giuliano Terme 56010 (PI), Italy
*/

#pragma once
#include <unordered_set>
#include <unordered_map>
#include <execinfo.h>
#include <signal.h>

#include "coco/task.h"
#include "coco/execution.h"
#include "coco/connection.h"

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
    using make_fx_t = std::function<std::shared_ptr<TaskContext> ()>;

	/// instantiate a specification with name and virtual ctor
    ComponentSpec(const std::string &class_name,
                  const std::string &name,
                  make_fx_t fx);

	std::string class_name_;
	std::string name_;
	make_fx_t fx_;
};

struct TypeSpec
{
	const char * name_; // pure name
    std::function<bool(std::ostream&, void*)> out_fx_; // conversion fx
	const std::type_info & type_; // internal name (unique)

    TypeSpec(const char * name, const std::type_info & type,
             std::function<bool(std::ostream&, void*)>  out_fx);
};
/**
 * Component Registry that is singleton per each exec or library. Then when the component library is loaded 
 * the singleton is replaced
 */
class ComponentRegistry
{
public:
	/// creates a component by name
    static std::shared_ptr<TaskContext> create(const std::string &name,
                                               const std::string &instantiation_name);
	/// adds a specification that will be used to retreive the task
	static void addSpec(ComponentSpec * s);
    static bool addLibrary(const std::string &library_name);
	/// adds a library
	static bool addLibrary(const std::string &l, const std::string &path );
	/// defines an alias. note that old_name should be present
	static void alias(const std::string &new_name, const std::string &old_name);
	/// iterate over the names of the components
    static const std::unordered_map<std::string, ComponentSpec*> & components();
    /// adds a type specification
    static void addType(TypeSpec * s);
    static TypeSpec *type(std::string name);
    static TypeSpec *type(const std::type_info & ti);
    template <class T>
    static TypeSpec *type()
    {
    	return type(typeid(T).name());
    }
    /// Allow to retreive any task by its instantiation name. This function is usable by any task
    static std::shared_ptr<TaskContext>  task(std::string name);

    static bool profilingEnabled();
    static void enableProfiling(bool enable);

    static int numTasks();
    static int increaseConfigCompleted();
    static int numConfigCompleted();

    static void setResourcesPath(const std::vector<std::string> & resources_path);
    static std::string resourceFinder(const std::string &value);

    static const std::unordered_map<std::string, std::shared_ptr<TaskContext> >& tasks();

    static void setActivities(const std::vector<std::shared_ptr<Activity>> &activities);
    static const std::vector<std::shared_ptr<Activity>>& activities();

private:
	static ComponentRegistry & get();
    std::shared_ptr<TaskContext> createImpl(const std::string &name,
                                            const std::string &instantiation_name);
	void addSpecImpl(ComponentSpec *s);
    bool addLibraryImpl(const std::string &library_name);
	bool addLibraryImpl(const std::string &lib, const std::string &path );
	void aliasImpl(const std::string &newname, const std::string &oldname);
    const std::unordered_map<std::string, ComponentSpec*> & componentsImpl() const;
    void addTypeImpl(TypeSpec *s);
	TypeSpec *typeImpl(std::string name);
	TypeSpec *typeImpl(const std::type_info & ti);
    std::shared_ptr<TaskContext>  taskImpl(std::string name);
    void setActivitiesImpl(const std::vector<std::shared_ptr<Activity>> &activities);

	bool profilingEnabledImpl();
	void enableProfilingImpl(bool enable);

	int numTasksImpl() const;
	int increaseConfigCompletedImpl();
	int numConfigCompletedImpl() const;

	void setResourcesPathImpl(const std::vector<std::string> & resources_path);
	std::string resourceFinderImpl(const std::string &value);

private:
	std::unordered_map<std::string, ComponentSpec*> specs_;
    std::unordered_map<std::string, TypeSpec*> typespecs_; // conflict less
    std::unordered_map<std::uintptr_t, TypeSpec*> typespecs2_; // with conflicts
	std::unordered_set<std::string> libs_;
    std::unordered_map<std::string, std::shared_ptr<TaskContext> > tasks_; /// Contains all the tasks created and it is accessible by every component
    std::vector<std::shared_ptr<Activity>> activities_;

	std::vector<std::string> resources_paths_;

	int tasks_config_ended_ = 0;
	int num_tasks_ = 0;

	bool profiling_enabled_ = false;
};

} // end of namespace coco

/// Return the pointer to the task with name T
#define COCO_TASK(T) \
	coco::ComponentRegistry::task(T)

/// Return the number of Task (withou peer)
#define COCO_NUM_TASK \
	coco::ComponentRegistry::numTasks()

#define COCO_CONFIGURATION_COMPLETED \
	coco::ComponentRegistry::numTasks() == coco::ComponentRegistry::numConfigCompleted()

#define COCO_TERMINATE \
	raise(SIGINT);

#define COCO_FIND_RESOURCE(x) \
	coco::ComponentRegistry::resourceFinder(x)

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

#define COCO_REGISTER(T) \
    coco::ComponentSpec T##_spec = { #T, #T, [] () -> std::shared_ptr<coco::TaskContext> {\
            std::shared_ptr<coco::TaskContext> task(new T);\
    		task->setType<T>();\
            return task;} };\

#if 0
#define COCO_REGISTER_NAMED(T,name) \
     coco::ComponentSpec T##_spec = { #T, name, [] () -> coco::TaskContext* {return new T(); } };\
     extern "C" const char * T##_coco_name = name;\
     extern "C" coco::TaskContext* T##_coco_make() { return new T(); }
#endif 
