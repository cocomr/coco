#include "cocoxx.hpp"


int main(int argc, char const *argv[])
{
	coco::CoCoApp h;

	// create objects and assign names (TODO anonymous)
	auto x = coco::vision::OpenNIReaderTask("reader");
	auto y = coco::vision::X264EncoderTask("enc");

    x.
	// returns Connectionf or customization
	y.image_IN << x.rgb_buffer_OUT;

	// put into container
	h << x << y;

	h.generateXML();
	//h.run();

	return 0;
}