/*
Copyright (c) 2014 Aerys

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction,
including without limitation the rights to use, copy, modify, merge, publish, distribute,
sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or
substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "minko/Minko.hpp"
#include "minko/MinkoASSIMP.hpp"
#include "minko/MinkoSDL.hpp"
#include "minko/MinkoJPEG.hpp"
#include "minko/MinkoPNG.hpp"
#include "minko/MinkoLeap.hpp"

using namespace minko;
using namespace minko::component;
using namespace minko::math;

#define PI 3.14f

const uint            WINDOW_WIDTH    = 800;
const uint            WINDOW_HEIGHT    = 600;
std::string    MODEL_FILENAME;

const float         Y_ROTATION_FACTOR = -(2* PI) / 170.f;
const float         X_ROTATION_FACTOR = -(2* PI) / 300.f;
const double         ZOOM_FACTOR = 0.4f;
//const float         HAND_POS_LERP_RATIO = 0.02f;
const double         TRANS_LERP_RATIO = 1.f;
const double         ROT_LERP_RATIO = 1.f;

int
main(int argc, char** argv)
{
    int modelNum = 0;
    if( argc == 2 )  {
        if( argv[1][0] == '1' ) {
            modelNum = 0;
            MODEL_FILENAME = "statueMax.dae";
        } else if( argv[1][0] == '2' ) {
            modelNum = 1;
            MODEL_FILENAME = "stone-minx-dense.dae";
        } else if( argv[1][0] == '3' ) {
            modelNum = 2;
            MODEL_FILENAME = "ball-rough-mat.dae";
        }
    }
    auto canvas = Canvas::create("v-interact : Museum Relic", WINDOW_WIDTH, WINDOW_HEIGHT);
    auto sceneManager = SceneManager::create(canvas);
    auto defaultOptions = sceneManager->assets()->loader()->options();

    // setup assets
    defaultOptions
        ->generateMipmaps(true)
        ->skinningFramerate(60)
        ->skinningMethod(SkinningMethod::HARDWARE)
        //->startAnimation(true)
        ->registerParser<file::OBJParser>("obj")
        ->registerParser<file::ColladaParser>("dae")
        ->registerParser<file::PNGParser>("png")
        ->registerParser<file::JPEGParser>("jpg");

    auto fxLoader = file::Loader::create(sceneManager->assets()->loader())
        ->queue("effect/Basic.effect")
        ->queue("effect/Phong.effect");

    auto fxComplete = fxLoader->complete()->connect([&](file::Loader::Ptr l)
    {
        sceneManager->assets()->loader()->queue(MODEL_FILENAME);
        sceneManager->assets()->loader()->load();
    });


    // LEAP MOTION
    auto controller     = input::leap::Controller::create(canvas);
    const clock_t      START_CLOCK                = clock();
    double    previousTime              = 0.0f;

    Vector3::Ptr handPos_persist = Vector3::create();
    Vector3::Ptr handPos_grab = Vector3::create();
    double translationXAmount = 0.f;
    double translationXAmount_target = 0.f;
    double translationYAmount = 0.f;
    double translationYAmount_target = 0.f;
    double translationZAmount = 0.f;
    double translationZAmount_target = 0.f;
    double rotationYAmount = 0.f;
    double rotationYAmount_target = 0.f;
    double rotationXAmount = 0.f;
    double rotationXAmount_target = 0.f;

    bool handClosedLast = false;

    auto leapEnterFrame = controller->enterFrame()->connect([&](input::leap::Controller::Ptr c)
    {
        if (!c->frame()->isValid())
            return;

        const double currentTime    = (clock() - START_CLOCK) / double(CLOCKS_PER_SEC);
        const double deltaTime      = currentTime - previousTime;
        previousTime               = currentTime;

        auto   frame               = c->frame();

        Vector3::Ptr handPos = Vector3::create();
        bool handClosed = false;
        if (frame->numHands() == 1)
        {
            frame->handByIndex(0)->palmPosition(handPos);
            handClosed = frame->handByIndex(0)->grabStrength() >= 0.5f;
        } else if (frame->numHands() >= 1) {
            frame->handByIndex(0)->palmPosition(handPos);
            handClosed = frame->handByIndex(0)->grabStrength() >= 0.5f;
            // TODO : fix this two handed averaging
            /*
            Vector3::Ptr leftHand, rightHand;
            frame->leftmostHand()->palmPosition(leftHand);
            frame->rightmostHand()->palmPosition(rightHand);
            handPos = Vector3::create(
                (leftHand->x()+rightHand->x())/2,
                (leftHand->y()+rightHand->y())/2,
                (leftHand->z()+rightHand->z())/2
            );
            */
        }
        /*
        handPos_persist->lerp(
            handPos,
            HAND_POS_LERP_RATIO
            );
            */
        handPos_persist->setTo(
            handPos->x(),
            handPos->y(),
            handPos->z()
            );
        if( handClosed ) {
            if( !handClosedLast ) {
                handPos_grab->setTo(
                    handPos_persist->x(),
                    handPos_persist->y(),
                    handPos_persist->z()
                    );
            }
            translationXAmount_target = (handPos_persist->x()-handPos_grab->x())
                    * ZOOM_FACTOR;
            translationYAmount_target = (handPos_grab->y()-handPos_persist->y())
                    * ZOOM_FACTOR;
            translationZAmount_target = (handPos_persist->z()-handPos_grab->z())
                    * ZOOM_FACTOR;
            //translationZAmount_target = handPos_persist->length() * ZOOM_FACTOR;
        } else {
            rotationYAmount_target = handPos_persist->x() * Y_ROTATION_FACTOR;
            rotationXAmount_target = -handPos_persist->z() * X_ROTATION_FACTOR;
            translationXAmount_target = 0.f;
            translationYAmount_target = 0.f;
            translationZAmount_target = 0.f;
        }
        handClosedLast = handClosed;
    });

    // on initialization of the Leap controller
    auto leapConnected = controller->connected()->connect([](input::leap::Controller::Ptr c)
    {
        //c->enableGesture(input::leap::Gesture::Type::ScreenTap);
        //c->enableGesture(input::leap::Gesture::Type::Swipe);

#ifdef DEBUG
        std::cout << "Leap controller connected" << std::endl;
#endif // DEBUG
    });

    auto root = scene::Node::create("root")
        ->addComponent(sceneManager);

    auto camera = scene::Node::create("camera")
        ->addComponent(Renderer::create(0x7f7f7fff)) //d5ce81
        ->addComponent(Transform::create(
            Matrix4x4::create()->lookAt(Vector3::zero(), Vector3::create(0., 0., -65.f))
            //Matrix4x4::create()->lookAt(Vector3::create(0.f, 0.75f, 0.f), Vector3::create(0.25f, 0.75f, 2.5f))
            ))
        ->addComponent(PerspectiveCamera::create(
            //canvas->aspectRatio()
            (float) WINDOW_WIDTH / (float) WINDOW_HEIGHT,
            (float) PI * 0.25f, .1f, 1000.f
            )
        );
    root->addChild(camera);

    Matrix4x4::Ptr camOriginMatrix = Matrix4x4::create();
    Matrix4x4::Ptr camModMatrix = Matrix4x4::create();

    camOriginMatrix->copyFrom(camera->component<Transform>()->matrix());
    camModMatrix->copyFrom(camOriginMatrix);

    auto _ = sceneManager->assets()->loader()->complete()->connect([ = ](file::Loader::Ptr loader)
    {
        auto model = sceneManager->assets()->symbol(MODEL_FILENAME);
        if( !model->hasComponent<Transform>() ) {
            model->addComponent(component::Transform::create());
        }

        if( modelNum == 0 ) {
            // these are for the buddha statue
            model->component<Transform>()->matrix()->appendTranslation(0.f, -10.f, 0.f);
            model->component<Transform>()->matrix()->appendRotationY(PI/2);
        } else if( modelNum == 1 ) {
            // these are for the stone minx
            model->component<Transform>()->matrix()->appendRotationY(PI);
            model->component<Transform>()->matrix()->appendScale(25.f);
        } else if( modelNum == 2 ) {
            // these are for the ball
            model->component<Transform>()->matrix()->appendRotationY(PI);
            model->component<Transform>()->matrix()->appendScale(35.f);
        }

        auto surfaceNodeSet = scene::NodeSet::create(model)
            ->descendants(true)
            ->where([](scene::Node::Ptr n)
        {
            return n->hasComponent<Surface>();
        });

        root->addComponent(AmbientLight::create(1.f));

        // Add a simple directional light to really see the camera rotation
        /*
        auto directionalLight = scene::Node::create("directionalLight")
            ->addComponent(DirectionalLight::create())
            ->addComponent(Transform::create(Matrix4x4::create()->lookAt(
                Vector3::zero(), Vector3::create(0., -40.f, -50.f))));
        root->addChild(directionalLight);
        */

        root->addChild(model);

        auto skinnedNodes = scene::NodeSet::create(model)
            ->descendants(true)
            ->where([](scene::Node::Ptr n)
        {
			return n->hasComponent<MasterAnimation>();
        });

        auto skinnedNode = !skinnedNodes->nodes().empty()
                           ? skinnedNodes->nodes().front()
                           : nullptr;
    });

    auto resized = canvas->resized()->connect([&](AbstractCanvas::Ptr canvas, uint w, uint h)
    {
        camera->component<PerspectiveCamera>()->aspectRatio(float(w) / float(h));
    });

    auto enterFrame = canvas->enterFrame()->connect([&](Canvas::Ptr canvas, float time, float deltaTime)
    {
        double deltaTimeSecs = (double)deltaTime / 1000;
        // update variables
        translationXAmount = translationXAmount
                + ((translationXAmount_target - translationXAmount) 
                * TRANS_LERP_RATIO * deltaTimeSecs);
        translationYAmount = translationYAmount
                + ((translationYAmount_target - translationYAmount)
                * TRANS_LERP_RATIO * deltaTimeSecs);
        translationZAmount = translationZAmount
                + ((translationZAmount_target - translationZAmount)
                * TRANS_LERP_RATIO * deltaTimeSecs);
        rotationXAmount = rotationXAmount
                + ((rotationXAmount_target - rotationXAmount)
                * ROT_LERP_RATIO * deltaTimeSecs);
        rotationYAmount = rotationYAmount
                + ((rotationYAmount_target - rotationYAmount)
                * ROT_LERP_RATIO * deltaTimeSecs);

        // reset camera position
        camModMatrix->copyFrom(camOriginMatrix);
        // apply changes to camera
        //camModMatrix->appendTranslation(0.f, 0.f, translationZAmount);
        camModMatrix->appendTranslation(
            translationXAmount, translationYAmount, translationZAmount);
        camModMatrix->appendRotationY(rotationYAmount);
        camModMatrix->appendRotationX(rotationXAmount);
        // set camera position
        camera->component<Transform>()->matrix()->copyFrom(camModMatrix);

        sceneManager->nextFrame(time, deltaTime);
    });

    controller->start();
    fxLoader->load();
    canvas->run();

    return 0;
}