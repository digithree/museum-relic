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

using namespace minko;
using namespace minko::component;
using namespace minko::math;

#define PI 3.14f

const uint            WINDOW_WIDTH    = 800;
const uint            WINDOW_HEIGHT    = 600;
const std::string    MODEL_FILENAME    = "statueMax.dae";

int
main(int argc, char** argv)
{
    auto canvas = Canvas::create("Minko Example - Assimp", WINDOW_WIDTH, WINDOW_HEIGHT);
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

    auto root = scene::Node::create("root")
        ->addComponent(sceneManager);

    auto camera = scene::Node::create("camera")
        ->addComponent(Renderer::create(0x7f7f7fff))
        ->addComponent(Transform::create(
            Matrix4x4::create()->lookAt(Vector3::zero(), Vector3::create(0., 0., -65.f))
            //Matrix4x4::create()->lookAt(Vector3::create(0.f, 0.75f, 0.f), Vector3::create(0.25f, 0.75f, 2.5f))
            ))
        ->addComponent(PerspectiveCamera::create(
            //canvas->aspectRatio()
            (float) WINDOW_WIDTH / (float) WINDOW_HEIGHT, (float) PI * 0.25f, .1f, 1000.f
            )
        );
    root->addChild(camera);

    auto _ = sceneManager->assets()->loader()->complete()->connect([ = ](file::Loader::Ptr loader)
    {
        auto model = sceneManager->assets()->symbol(MODEL_FILENAME);
        if( !model->hasComponent<Transform>() ) {
            model->addComponent(component::Transform::create());
        }
        model->component<Transform>()->matrix()->appendTranslation(0.f, -10.f, 0.f);
        model->component<Transform>()->matrix()->appendRotationY(PI/2);

        auto surfaceNodeSet = scene::NodeSet::create(model)
            ->descendants(true)
            ->where([](scene::Node::Ptr n)
        {
            return n->hasComponent<Surface>();
        });

        root->addComponent(AmbientLight::create(0.9f));

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

    // UI
    Signal<input::Mouse::Ptr, int, int>::Slot mouseMove;
    float cameraRotationSpeed = 0.f;

    auto mouseDown = canvas->mouse()->leftButtonDown()->connect([&](input::Mouse::Ptr mouse)
    {
        mouseMove = canvas->mouse()->move()->connect([&](input::Mouse::Ptr mouse, int dx, int dy)
        {
            cameraRotationSpeed = (float) -dx * .01f;
        });
    });

    auto mouseUp = canvas->mouse()->leftButtonUp()->connect([&](input::Mouse::Ptr mouse)
    {
        mouseMove = nullptr;
    });

    auto enterFrame = canvas->enterFrame()->connect([&](Canvas::Ptr canvas, float time, float deltaTime)
    {
        camera->component<Transform>()->matrix()->appendRotationY(cameraRotationSpeed);
        cameraRotationSpeed *= .8f;

        sceneManager->nextFrame(time, deltaTime);
    });

    fxLoader->load();
    canvas->run();

    return 0;
}