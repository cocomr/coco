#pragma once

#ifdef _MSC_BUILD
	#ifndef COCO_EXPORT
		#ifdef coco_EXPORTS
			/* We are building this library */
			#define COCO_EXPORT __declspec(dllexport)
		#else
			/* We are using this library */
			#define COCO_EXPORT __declspec(dllimport)
		#endif
	#endif
#else
	#define COCO_EXPORT
#endif
