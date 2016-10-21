#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Kinect2.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CapstoneApp : public App {
public:
	void setup() override;
	void mouseDown(MouseEvent event) override;
	void update() override;
	void draw() override;
private:
	ci::Channel16uRef  mChannelDepth;
	ci::Channel8uRef   mChannelBodyIndex;
	Kinect2::BodyFrame mBodyFrame;
	Kinect2::DeviceRef mDevice;
};

void CapstoneApp::setup()
{
	mDevice = Kinect2::Device::create();
	mDevice->start();
	// connect to the depth channel.
	mDevice->connectDepthEventHandler([&](const Kinect2::DepthFrame frame)
	{
		mChannelDepth = frame.getChannel();
	});
	// connect to the body channel.
	mDevice->connectBodyEventHandler([&](const Kinect2::BodyFrame frame)
	{
		mBodyFrame = frame;
	});
	// get the body indeces (???)
	mDevice->connectBodyIndexEventHandler([&](const Kinect2::BodyIndexFrame frame)
	{
		mChannelBodyIndex = frame.getChannel();
	});
	mDevice->enableHandTracking(true);
}

void CapstoneApp::mouseDown(MouseEvent event)
{
}

void CapstoneApp::update()
{
}

void CapstoneApp::draw()
{
	// TODO: not sure what any of this does...
	const gl::ScopedViewport scopedViewport(ivec2(0), getWindowSize());
	const gl::ScopedMatrices scopedMatrices;
	const gl::ScopedBlendAlpha scopedBlendAlpha;
	gl::setMatricesWindow(getWindowSize());


	// clear the screen.
	gl::clear();
	gl::color(ColorAf::white());

	// TODO: not sure why we need to disable rw on this...
	gl::disableDepthRead();
	gl::disableDepthWrite();

	// draw the depth channel.
	if (mChannelDepth) {
		gl::enable(GL_TEXTURE_2D);
		const gl::TextureRef tex = gl::Texture::create(*Kinect2::channel16To8(mChannelDepth));
		gl::draw(tex, tex->getBounds(), Rectf(getWindowBounds()));
	}
	// draw the body channel overtop of the depth.
	if (mChannelBodyIndex) {
		// drawing the bodies.
		gl::enable(GL_TEXTURE_2D);
		const gl::TextureRef tex = gl::Texture::create(*Kinect2::colorizeBodyIndex(mChannelBodyIndex));
		gl::draw(tex, tex->getBounds(), Rectf(getWindowBounds()));

		// drawing the circled hands.
		const gl::ScopedModelMatrix scopedModelMatrix; // why?
		gl::scale(vec2(getWindowSize()) / vec2(mChannelBodyIndex->getSize()));
		gl::disable(GL_TEXTURE_2D); // why?
		for (const Kinect2::Body body : mBodyFrame.getBodies()) {
			if (body.isTracked()) {
				// get the left hand state.
				const Kinect2::Body::Hand leftHand = body.getHandLeft();
				// get the left hand coordinates.
				const ivec2 lhpos = mDevice->mapCameraToDepth(body.getJointMap().at(JointType_HandLeft).getPosition());
				if (leftHand.getState() == HandState_Closed) {
					gl::color(ColorAf(1.0f, 0.0f, 0.0f, 0.5f));
				}
				else {
					gl::color(ColorAf(0.0f, 1.0f, 0.0f, 0.5f));
				}
				gl::drawSolidCircle(lhpos, 30.0f, 32);
			}
		}
	}
}

CINDER_APP(CapstoneApp, RendererGl, [&](App::Settings *settings) {
	settings->setWindowSize(1280, 720);
})
