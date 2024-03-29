#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/params/Params.h"

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
	// helper functions
	void setupKinect();
	void setupParameters();
	void updateScreenParameters();
	void updateNotePosition();
	bool inBounds(vec2 hand, vec2 note);
	void drawBackground();
	void drawBodies();
	// kinect variables
	ci::Channel16uRef  mChannelDepth;
	ci::Surface8uRef   mSurfaceColor;
	ci::Channel8uRef   mChannelBodyIndex;
	Kinect2::BodyFrame mBodyFrame;
	Kinect2::DeviceRef mDevice;
	// screen parameters
	float						mFrameRate;
	bool						mFullScreen;
	ci::params::InterfaceGlRef	mParams;
	// hand state variables
	vec2 leftHandCoord = vec2(0, 0);
	// sequencer note variables
	int noteSize      = 45;
	vec2 noteTopLeft  = vec2(100, 175);
	ColorAf noteColor = ColorAf(1.0f, 1.0f, 1.0f, 0.85f);
	bool noteCaught   = false;
};

// ---------------
// Setup Functions
// ---------------

void CapstoneApp::setup()
{
	setupKinect();
	setupParameters();
}

void CapstoneApp::setupKinect()
{
	// connect + start the kinect.
	mDevice = Kinect2::Device::create();
	mDevice->start();

	mDevice->enableHandTracking(true);

	// connect to the depth channel.
	mDevice->connectDepthEventHandler([&](const Kinect2::DepthFrame frame)
	{
		mChannelDepth = frame.getChannel();
	});
	// connect to the hd video channel.
	mDevice->connectColorEventHandler([&](const Kinect2::ColorFrame& frame)
	{
		mSurfaceColor = frame.getSurface();
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
}

void CapstoneApp::setupParameters() 
{
	// set parameters to default values.
	mFrameRate = 0.0f;
	mFullScreen = false;
	// create interface for parameters.
	mParams = params::InterfaceGl::create("Params", ivec2(200, 100));
	mParams->addParam("Frame rate", &mFrameRate, "", true);
	mParams->addParam("Full screen", &mFullScreen).key("f");
	mParams->addButton("Quit", [&]() { quit(); }, "key=q");
}

// --------------
// Event Handlers
// --------------

void CapstoneApp::mouseDown(MouseEvent event)
{
}

// ----------------
// Update Functions
// ----------------

void CapstoneApp::update()
{
	updateScreenParameters();
	updateNotePosition();
}

void CapstoneApp::updateScreenParameters()
{
	mFrameRate = getAverageFps();

	if (mFullScreen != isFullScreen()) {
		setFullScreen(mFullScreen);
		mFullScreen = isFullScreen();
	}
}

void CapstoneApp::updateNotePosition()
{
	for (const Kinect2::Body body : mBodyFrame.getBodies()) {
		if (body.isTracked()) {
			const Kinect2::Body::Hand leftHand = body.getHandLeft();
			const Kinect2::Body::Hand rightHand = body.getHandRight();
			const ivec2 lhpos = mDevice->mapCameraToDepth(body.getJointMap().at(JointType_HandLeft).getPosition());
			//const ivec2 rhpos = mDevice->mapCameraToDepth(body.getJointMap().at(JointType_HandRight).getPosition());
			if (leftHand.getState() == HandState_Closed && noteCaught) {
				OutputDebugString(L"note is caught \n");
				vec2 leftHandDelta = vec2(lhpos.x - leftHandCoord.x, lhpos.y - leftHandCoord.y);
				noteTopLeft = vec2(noteTopLeft.x + leftHandDelta.x, noteTopLeft.y + leftHandDelta.y);
			}
			//else if (rightHand.getState() == HandState_Closed && noteCaught) {
			//	OutputDebugString(L"note is caught \n");
			//	noteTopLeft = vec2(rhpos.x, rhpos.y);
			//}
			else if (inBounds(lhpos, noteTopLeft)) {
				// || inBounds(rhpos, noteTopLeft)
				OutputDebugString(L"note got caught \n");
				noteCaught = true;
			}
			else {
				OutputDebugString(L"note did not get caught \n");
				noteCaught = false;
			}
			leftHandCoord = lhpos;
		}
	}
}

bool CapstoneApp::inBounds(vec2 hand, vec2 note) 
{
	int noteXCenter = note.x + (noteSize / 2);
	int noteYCenter = note.y + (noteSize / 2);
	// assumes that hand coordinate is the center of the hand.
	int delta = abs(hand.x - noteXCenter) + abs(hand.y - noteYCenter);
	return delta < noteSize;
}

// --------------
// Draw Functions
// --------------

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

	drawBackground();
	drawBodies();

	// draw the sequencer note.
	// see: drawBodies() for the drawing of the note.

	// draw the parameters interface.
	mParams->draw();
}

void CapstoneApp::drawBackground()
{
	// draw the hd color channel.
	//if (mSurfaceColor) {
	//	gl::enable(GL_TEXTURE_2D);
	//	const gl::TextureRef tex = gl::Texture::create(*mSurfaceColor);
	//	gl::draw(tex, tex->getBounds(), Rectf(getWindowBounds()));
	//}
	// draw the depth channel.
 	if (mChannelDepth) {
 		gl::enable(GL_TEXTURE_2D);
 		const gl::TextureRef tex = gl::Texture::create(*Kinect2::channel16To8(mChannelDepth));
 		gl::draw(tex, tex->getBounds(), Rectf(getWindowBounds()));
 	}
}

void CapstoneApp::drawBodies()
{
	// draw the body channel overtop of the hd color.
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
		// TODO: the coordinates are getting readjusted due to the rescaling.
		//       therefore drawing the rectangle has to occur in here (should readjust this somehow...)
		gl::color(noteColor);
		gl::drawSolidRect(Rectf(noteTopLeft, vec2(noteTopLeft.x + noteSize, noteTopLeft.y + noteSize)));
	}
}

// --------
// Settings
// --------

CINDER_APP(CapstoneApp, RendererGl, [&](App::Settings *settings) {
	settings->prepareWindow(Window::Format().size(1280, 720).title("Basic App"));
})
