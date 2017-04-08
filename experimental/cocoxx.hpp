#pragma once
#include <string>
#include <type_traits>
#include <memory>
#include <vector>
#include <iostream>

template<typename D, typename B>
using extends = typename std::enable_if<std::is_base_of<B,D>::value>::type;

namespace coco
{
	class InputPortBase {};
	class OutputPortBase {};

	class ComponentProxy
	{
	public:
		ComponentProxy(std::string n):name_(n) {};

		template <class TT>
		friend class OutputPort;

		template <class TT>
		friend class InputPort;

	private:
		std::vector<OutputPortBase*> outputs_;
		std::vector<InputPortBase*> inputs_;
		std::string name_;
	};

	/**
	 * TODO log
	 * TODO paths
	 * TODO include
	 * TODO activities
	 */
	class CoCoApp
	{
	public:
		template <class T, typename X = extends<T,ComponentProxy> >
		CoCoApp& operator << (const T & p)
		{
			proxies.push_back(std::make_shared<T>(p));
			return *this;
		}

		void generateXML();
		void run();

	private:
		std::vector<std::shared_ptr<ComponentProxy> > proxies;

	};


	template <class T>
	class Attribute
	{
	public:
		Attribute() {}
		Attribute(const T & x) : inited_(true),value_(x) {}
		Attribute& operator=(const T &x) { inited_=true; value_ = x; return *this; }

	private:
		T value_;
		bool inited_ = false;
	};

	struct Connection
	{
		std::string srctask,srcport,dsttask,dstport;
		// TODO policy
	};

	template <class T>
	class InputPort;

	template <class T>
	class OutputPort: public OutputPortBase
	{
	public:
		OutputPort(ComponentProxy * p, std::string q) : name_(q),parent_(p) { parent_->outputs_.push_back(this); }

		Connection& operator >>(InputPort<T> & x);

		template <class TT>
		friend class InputPort;

	private:
		ComponentProxy * parent_;
		std::string name_;
	};

	template <class T>
	class InputPort: public InputPortBase
	{
	public:
		InputPort(ComponentProxy * p, std::string q) : name_(q),parent_(p) { parent_->inputs_.push_back(this); }
		Connection& operator <<(OutputPort<T> & x)
		{
			outgoing.push_back({parent_->name_,name_,x.parent_->name_,x.name_});
			return outgoing.back();
		}

	private:
		ComponentProxy * parent_;
		std::string name_;
		std::vector<Connection> outgoing;
	};

	template <class T>
		Connection & OutputPort<T>::operator >>(InputPort<T> & x)
		{
			return x >> *this;
		}

	inline void CoCoApp::run()
	{
		std::cout << "NOT YET" << std::endl;
	}

	inline void CoCoApp::generateXML()
	{
		// package
		// log
		// paths
		// components
		// activities ?
		// connections
	}

// ------------------------------------------------------------------------

	// can make
	struct CameraInfo
	{};
	struct ImageBuffer
	{
	};

	// this can be literal
	struct Pose
	{
	};

	struct RgbdBuffer
	{

	};

	namespace vision
	{
		class OpenNIReaderTask: public ComponentProxy
		{
		public:
			using ComponentProxy::ComponentProxy;

			coco::OutputPort<RgbdBuffer> rgbd_buffers_OUT = {this,"rgbd_buffers_OUT"}; // RGB-D image
			coco::OutputPort<ImageBuffer> rgb_buffer_OUT = {this,"rgb_buffer_OUT"}; // color image
			coco::OutputPort<ImageBuffer> depth_buffer_OUT = {this,"depth_buffer_OUT"}; // depth image
			coco::OutputPort<CameraInfo> camera_info_OUT = {this,"camera_info_OUT"}; // camera info

		    coco::Attribute<std::string> device;
		    coco::Attribute<bool> registration;
		    coco::Attribute<bool> mirror;

		};

		class X264EncoderTask: public ComponentProxy
		{
		public:
			using ComponentProxy::ComponentProxy;

			coco::InputPort<ImageBuffer> image_IN = {this,"image_IN"}; // input Image
			coco::OutputPort<ImageBuffer> image_OUT = {this,"image_OUT"}; // compressed Image buffer


		};		
	}
}