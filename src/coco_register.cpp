#include "coco_register.h"

// dlopen cross platform
#ifdef WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	typedef HANDLE dlhandle;
	#define DLLEXT ".dll"
	#define DLLPREFIX "lib"
	#define dlopen(x,y) LoadLibrary(x)
	#define dlsym(x,y) GetProcAddress((HMODULE)x,y)
	#define dlerror() GetLastError()
	#define RTLD_NOW 0
	#define PATHSEP ';'
	#define DIRSEP '\\'
#else
	#include <dlfcn.h>
	typedef void* dlhandle;
	#define PATHSEP ':'
	#define DIRSEP '/'

	#ifdef __APPLE__
		#define DLLEXT ".dylib"
		#define DLLPREFIX "lib"
	#else
		#define DLLEXT ".so"
		#define DLLPREFIX "lib"
	#endif
#endif

/// as pointer to avoid issues of order in ctors
static coco::ComponentRegistry *singleton;

namespace coco
{

ComponentSpec::ComponentSpec(const std::string &name, make_fx_t fx)
	: name_(name), fx_(fx)
{
	COCO_LOG(1) << "[coco] " << this << " spec selfregistering " << name;
	ComponentRegistry::addSpec(this);
}

ComponentRegistry & ComponentRegistry::get()
{
	ComponentRegistry * a = singleton;
	if(!a)
	{
		singleton = new ComponentRegistry();
		COCO_LOG(1) << "[coco] registry creation " << singleton;
		return *singleton;
	}
	else
		return *a;
}

// static
TaskContext * ComponentRegistry::create(const std::string &name,
										const std::string &instantiation_name)
{
	return get().createImpl(name, instantiation_name);
}
TaskContext * ComponentRegistry::createImpl(const std::string &name,
										    const std::string &instantiation_name)
{
    auto it = specs_.find(name);
	if(it == specs_.end())
		return 0;
	//return it->second->fx_();
	tasks_[instantiation_name] = it->second->fx_();
    return tasks_[instantiation_name];
}

// static
void ComponentRegistry::addSpec(ComponentSpec * s)
{
	get().addSpecImpl(s);
}
void ComponentRegistry::addSpecImpl(ComponentSpec * s)
{
	COCO_LOG(1) << "[coco] " << this << " adding spec " << s->name_ << " " << s;	
	specs_[s->name_] = s;
}

// static
void ComponentRegistry::alias(const std::string &new_name,const std::string &old_name)
{
	return get().aliasImpl(new_name, old_name);
}
void ComponentRegistry::aliasImpl(const std::string &new_name,const std::string &old_name)
{
	auto it = specs_.find(old_name);
	if(it != specs_.end())
		specs_[new_name] = it->second;
}

// static
bool ComponentRegistry::addLibrary(const std::string &l, const std::string &path)
{
	return get().addLibraryImpl(l, path);
}
bool ComponentRegistry::addLibraryImpl(const std::string &lib, const std::string &path)
{
    // Concatenate the library name with the path and the extensions
    std::string library_name;
    if (!path.empty())
    {
        library_name = std::string(path);
        if (library_name[library_name.size() - 1] != DIRSEP)
            library_name += (DIRSEP);
        library_name += DLLPREFIX + lib + DLLEXT;
    }
    else
    {
        library_name = lib;
    }
	if(libs_.find(library_name) != libs_.end())
		return true; // already loaded

	// TODO: OS specific shared library extensions: so dylib dll
	
	dlhandle dl_handle = dlopen(library_name.c_str(), RTLD_NOW);
	if(!dl_handle)
	{
		COCO_ERR() << "Error opening library: " << library_name << "\nError: " << dlerror();
		return false;		
	}

	typedef ComponentRegistry ** (*getRegistry_fx)();
	getRegistry_fx get_registry_fx = (getRegistry_fx)dlsym(dl_handle, "getComponentRegistry");
	if(!get_registry_fx)
		return false;

	ComponentRegistry ** other_registry = get_registry_fx();
	if(!*other_registry)
	{
		COCO_LOG(1) << "[coco] " << this << " propagating to " << other_registry;
		*other_registry = this;
	}
	else if(*other_registry != this)
	{
		COCO_LOG(1) << "[coco] " << this << " embedding other " << *other_registry << " stored in " << other_registry;
		// import the specs and the destroy the imported registry and replace it inside the shared library
		for(auto&& i : (*other_registry)->specs_)
		{
			specs_[i.first] = i.second;
		}		
		delete *other_registry;
		*other_registry = this;
	}
	else
	{
		COCO_LOG(1) << "[coco] " << this << " skipping self stored in " << other_registry;
	}
	
	libs_.insert(library_name);
	return true;
}

impl::map_keys<std::string, ComponentSpec *> ComponentRegistry::componentsName()
{
    return get().componentsNameImpl();
}
impl::map_keys<std::string, ComponentSpec *> ComponentRegistry::componentsNameImpl()
{
    return impl::make_map_keys(specs_);
}

TaskContext *ComponentRegistry::task(std::string name)
{
	return get().taskImpl(name);
}
TaskContext *ComponentRegistry::taskImpl(std::string name)
{
	auto t = tasks_.find(name);
	if (t == tasks_.end())
		return nullptr;
	return t->second;
}

impl::map_values<std::string, TaskContext *> ComponentRegistry::tasks()
{
	return get().tasksImpl();
}
impl::map_values<std::string, TaskContext *> ComponentRegistry::tasksImpl()
{
	return coco::impl::make_map_values(tasks_);
}




} // end of namespace

extern "C" 
{
	coco::ComponentRegistry ** getComponentRegistry() { return &singleton; }
}
