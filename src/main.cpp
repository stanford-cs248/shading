#include "CS248/CS248.h"
#include "CS248/viewer.h"

#define TINYEXR_IMPLEMENTATION
#include "CS248/tinyexr.h"

#include "application.h"

#include <iostream>

#ifndef gid_t
typedef unsigned int gid_t;  // XXX Needed on some platforms, since gid_t is
                             // used in unistd.h but not always defined!
                             // (WARNING: May not be the right size!!)
#endif

using namespace std;
using namespace CS248;

#define msg(s) cerr << "[Render] " << s << endl;


void usage(const char* binaryName) {
    printf("Usage: %s [options] <scenefile>\n", binaryName);
    printf("Program Options:\n");
    printf("  -h               Print this help message\n");
    printf("\n");
}

int main(int argc, char** argv) {

    if (1 >= argc) {
        usage(argv[0]);
        return 1;
    }

    string sceneFilePath = argv[1];
    msg("Input scene file: " << sceneFilePath);

    // parse scene
    Collada::SceneInfo* sceneInfo = new Collada::SceneInfo();
    if (Collada::ColladaParser::load(sceneFilePath.c_str(), sceneInfo) < 0) {
        msg("Error: parsing failed!");
        delete sceneInfo;
        return 1;
      }

    // create viewer
    Viewer viewer = Viewer();

    // create application
    Application app;

    // init viewer
    viewer.init(&app);

    // load scene.
    // NOTE: This must come after the viewer.init() since loading
    // the scene creates OGL assets (and the viewer.init() is where the OGL context is created)
    app.load(sceneInfo);

    delete sceneInfo;

    // start viewer
    viewer.start();

    // TODO: FIXME
    // apparently the meshEdit renderer instance was not destroyed properly
    // not sure if this is due to the recent refactor but if anyone got some
    // free time, check the destructor for Application.
    exit(EXIT_SUCCESS);  // shamelessly faking it

    return 0;
}
