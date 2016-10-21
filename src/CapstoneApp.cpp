#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class CapstoneApp : public App {
  public:
	void setup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;
};

void CapstoneApp::setup()
{
}

void CapstoneApp::mouseDown( MouseEvent event )
{
}

void CapstoneApp::update()
{
}

void CapstoneApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
}

CINDER_APP( CapstoneApp, RendererGl )
