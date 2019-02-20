# Stanford CS248 Assignment 3: Real-Time Shader Programming

The repository is located at https://github.com/stanford-cs248/shading

## Due date

The assignment is due Tue Feb 26th, at 11:59:59 PM.

## Summary

In this assignment, you are a given a simple real-time renderer and a simple 3D scene.  However, the starter code implements only very simple material and lighting models, so rendered images do not look particularly great. In this assignment, you will improve the quality of the rendering by implementing a number of lighting and material shading effects using the GLSL, OpenGL's [shading language](https://thebookofshaders.com/01/).  The point of this assignment is to get basic experience with modern shader programming, and to build understanding of how material pattern logic, material BRDFs, and lighting computations are combined in a shader to compute surface reflectance.  Compared to assignments 1 and 2, this is a relatively short assignment, but there are countless ways you can keep going by adding more complex materials and lighting.  This is will be great fodder for final projects. We are interested to see what you can do!

## Build Instructions

In order to ease the process of running on different platforms, we will be using [CMake](http://www.cmake.org/) for our assignments. You will need a CMake installation of version 2.8+ to build the code for this assignment. It should also be relatively easy to build the assignment and work locally on your OSX or 64-bit version of Linux or Windows.
The project can be run by SSH'ing to rice.stanford.edu with your SUNet ID, password, and two-step authentication using MobaXterm (remember to turn on X11 forwarding). If you choose to do so, you can skip over to the next step.

### OS X/Linux Build Instructions

If you are working on OS X and do not have CMake installed, we recommend installing it through [Homebrew](http://brew.sh/): `$ brew install cmake`.  You may also need the freetype package `$ brew install freetype`.

If you are working on Linux, you should be able to install dependencies with your system's package manager as needed (you may need cmake and freetype, and possibly others).

#### To install freetype on Linux:

```
$ sudo apt-get install libfreetype6
$ sudo apt-get install libfreetype6-dev
```

#### To install other possible dependencies (RandR, etc) on Linux:

```
$ sudo apt-get install libglu1-mesa-dev
$ sudo apt-get install xorg-dev
```

#### To build your code for this assignment on OS X or Linux:

```
$ cd shading && mkdir build && cd build
$ cmake ..
$ make
```

These 3 steps (1) create an out-of-source build directory, (2) configure the project using CMake, and (3) compile the project. If all goes well, you should see an executable `render` in the build directory. As you work, simply typing `make` in the build directory will recompile the project.

### Windows Build Instructions

You need to install the latest version of [CMake](http://www.cmake.org/) and [Visual Studio](https://www.visualstudio.com/). Visual Studio Community is free. After installing these programs, replace `SOURCE_DIR` to the cloned directory (`shading/` in our case), and `BUILD_DIR` to `SOURCE_DIR/build`.

![Sample locations](misc/cmake_initial_setup.png?raw=true)

Then, press `Configure` button, select proper version of Visual Studio (**You should probably select Win64**), and you should see `Configuring done` message. Then, press `Generate` button and you should see `Generating done`.

![Sample locations](misc/cmake_final_setup.png?raw=true)

This should create a `build` directory with a Visual Studio solution file in it named `Render.sln`. You can double-click this file to open the solution in Visual Studio.

If you plan on using Visual Studio to debug your program, you must change `render` project in the Solution Explorer as the startup project by right-clicking on it and selecting `Set as StartUp Project`. You can also set the command line arguments to the project by right-clicking `render` project again, selecting `Properties`, going into the `Debugging` tab, and setting the value in `Command Arguments`. If you want to run the program with the test folder, you can set this command argument to `../../media/spheres/spheres.json`. After setting all these, you can hit F5\press `Local Windows Debugger` button to build your program and run it with the debugger.

You should also change the build mode to `Release` from `Debug` occasionally by clicking the Solution Configurations drop down menu on the top menu bar, which will make your program run faster. Note that you will have to set `Command Arguments` again if you change the build mode. Note that your application must run properly in both debug and release build.

## Summary of Viewer Controls

A table of all the keyboard controls in the application is provided below.

| Command                                  |  Key  |
| ---------------------------------------- | :---: |
| Print camera parameters                  | 'C'   |

## Getting Oriented in the Code ##

To complete this assignment, you'll be writing shaders in a language called GLSL (OpenGL Shading Language).  GLSL's syntax is C/C++ "like", but it features a number of built in types specific to graphics. In this assignment, there are two shaders, a __vertex shader__ `media/shader.vert` that is executed _once per vertex_ of each rendered triangle mesh.  And a __fragment shader__ `media/shader.frag` that executes _once per fragment_ (a.k.a. once per screen sample covered by a triangle.)

We didn't specifically talk about the specifics of GLSL programming in class, so you'll have to pick it up on your own. Fortunately, there are a number of great GLSL tutorials online, but we recommend [The Book of Shaders](https://thebookofshaders.com/).  Here are a few things to know:

* You'll want to start by looking at `main()` in both `shader.vert` and `shader.frag`.  This is the function that executes once per vertex or once per fragment.

* Make sure you understand the difference between __uniform parameters__ to a shader, which are read-only variables that take on the same value for all invocations of a shader, and __varying values__ that assume different values for each invocation.  Per-vertex input attributes are inputs to the vertex shader that have a unique value per vertex (such as position, normal, texcoord, etc.).  The vertex's shader's outputs are interpolated by the rasterizer, and then the interpolated values are provided as inputs to the fragment shader when it is invoked for a specific surface point.  Notice how the name of the vertex shader's output variables matches the name of the fragment shader's varying input variables.

* The assignment starter code JIT compiles your GLSL vertex and fragment shaders on-the-fly when the `render` application starts running.  Therefore, you won't know if the code successfully compiles until run time.  If you see a black screen while rendering, it's likely because your GLSL shader failed to compile.  __Look to your console for error messages about a failed compile.__

### Part 1: Implementing Phong Reflectance (20%)

In the first part of this assignment you will implement two important aspects of defining a material.  First, you will implement a simple BRDF that implements the phong reflectance model to render a shiny surface.

To begin, render the spheres scene using the command:

    ./render ../media/spheres/spheres.json

You should see an image that looks a bit like the one below:

![Spheres starter image](misc/step1.jpg?raw=true)

__What you need to do:__

Notice that the scene is rendered with a texture map on the ground plane and two of the three spheres, but there is no lighting in the rendering. To add basic lighting, we'd like you to implement `Phong_BRDF()` in `media/shader.frag`. This function should implement the [Phong Reflectance](https://en.wikipedia.org/wiki/Phong_reflection_model) model.  Your implementation should compute the diffuse and specular components of this reflectance model in `Phong_BRDF()`.  Note that the ambient term is independent of scene light sources and should be added in to the total surface reflectance outside of the light integration loops (the starter code already does this).  

A correct implementation of Phong reflectance should yield shaded spheres, which should look like this.

![Spheres with phong](misc/step2.jpg?raw=true)

### Part 2: Normal mapping (25%)

Although there is a texture map on the ground plane and spheres to add detail to these surfaces, the surfaces continue to look "flat". Your next task to implement [normal mapping](http://cs248.stanford.edu/winter19/lecture/texture/slide_039) to create the illusion of a surface having more detail that what is modeled by the underlying geometry.  They idea of normal mapping is to perturb the surface's geometric normal with an offset by a vector encoded in a texture map.  An example "normal map" is shown at right in the image below.

![Tangent space figure](misc/tangent_fig.png?raw=true)

Each RGB sample in the "normal map" encodes a 3D vector that is a perturbation of the geometry's actual normal at the corresponding surface point.  However, instead of encoding this offset in single coordinate space (e.g, object space or world space), the vectors in the normal map are represented in the __surface's tangent space.___ Tangent space is a coordinate frame that is relative to the surface's orientation at each point. It is defined by the surface's normal, aligned with the Z-axis [0 0 1] in tangent space, and a tangent vector to the surface, corresponding to X-axis [1 0 0].  The tangent space coordinate frame at a point on the surface is illustrated at left in the figure above.

Normal mapping works as follows: given a point on the surface, your shader needs to sample the normal map at the appropriate location to obtain the _tangent space normal_ at this point.  Then the shader should convert the tangent space normal to its world space representation so that the normal can be used for reflectance calculations. 

__What you need to do:__ 

First, modify the vertex shader `shader.vert` to compute a transform `tan2World`.  This matrix should convert vectors in tangent space back to world space.  You should think about creating a rotation matrix that converts tangent space to object space, and then applying an additional transformation to move the object space frame world space.  The vertex shader emits `tan2World` for later use in fragment shading.

* Notice that in `shader.vert` you are given the normal (N) and surface tangent (T) at the current vertex.  But you are not given the third vector, often called the "binormal vector" (B) defining the Y-axis of tangent space.  How do you compute this vector given the normal and tangent?
* How do you create a rotation matrix that takes tangent space vectors to object space vector?  [See this slide](http://cs248.stanford.edu/winter19/lecture/transforms/slide_049) for a hint.

Second, in `shader.frag`, you need to sample the normal map (see `normalTextureSampler`), and then use `tan2World` to compute a world space normal at the current surface sample point.

We recommend that you debug normal mapping on the sphere scene (`media/spheres/spheres.json`).  

Without normal mapping, the sphere looks like a flat sphere with a brick texture, as shown at left. But with normal mapping, notice how the bumpy surface creates more plausible reflective highlights in the rendering on the right.

With a correct implementation of normal mapping the scene will look like this:

![Spheres with normal mapping](misc/step3.jpg?raw=true)

### Part 3: Adding Environment Lighting (25%)

So far, your shaders have used simple point and directional light sources in the scene. (Notice that in `shader.frag` the code iterated over light sources and accumulated reflectance.)  We'd now like you to implement a more complex form of light source.  This light source, called an image based environment light, described [here in lecture](http://cs248.stanford.edu/winter19/lecture/materials/slide_037) represents light incoming on the scene from an _infinitely far source, but from all directions_.  Pixel (x,y) in the texture map encodes the magnitude and color and light from the direction (phi, theta).  Phi and theta encode a direction in [spherical coordinates](https://en.wikipedia.org/wiki/Spherical_coordinate_system).

__What you need to do:__

In `media/shader.frag`, we'd like you to implement a perfectly mirror reflective surface.  The shader should [reflect the vector](http://cs248.stanford.edu/winter19/lecture/materials/slide_051) from the surface to the camera about the surface normal, and the use the reflected vector to perform a lookup in the environment map.  A few notes are below:

* `dir2camera` conveniently gives you the direction from the surface _to the camera_.  It is not normalized.
* The function `vec3 SampleEnvironmentMap(vec3 L)` takes as input a direction (outward from the scene), and returns the radiance from the environment light arriving from this direction (this is lightarriving at the surface from an infinitely far light source from the direction -L).
* To perform an environment map lookup, you have to convert the reflection direction from its 3D Euclidean representation to spherical coordinates phi and theta.  In this assignment rendering is set up to use a a right-handed coordinate system (where Y is up, X is pointing to the right, and Z is pointing toward the camera), so you'll need to adjust the standard equations of converting from XYZ to spherical coordinates accordingly.  Specifically, in this assignment, the polar (or zenith) angle __theta__ is the angle between the direction and the Y axis.  The azimuthal angle __phi__ is zero when the direction vector is in the YZ plane, and increases as this vector rotates toward the XY plane. 

Once you've correctly implemented an environment map lookups, the rightmost sphere will look as if it's a perfect mirror.

![Spheres with normal mapping](misc/step4.jpg?raw=true)

You'll also be able to render a reflective teapot (`media/teapot/teapot.json`), as shown below.

![Mirror teapot](misc/teapot_mirror.png?raw=true)

### Part 4: Adding Spotlights and Shadows (30 pts)

__PLEASE PULL THE LATEST ASSIGNMENT 3 CODE FROM GITHUB AS IT IS NEEDED FOR PART 4.__

In the final part of this assignment you will implement a more advanced type of light source, a spotlight, as well as use shadow mapping with percentage closer filtering (PCF) to compute smooth shadows for your spotlights.  These more advanced lighting conditions will significantly improve the realism of your rendering.  When you are done with this part of the assignment, you will be able to render the scene `media/spheres/spheres_shadow.json` to get a rendering like this:

![Nice shadowed spotlights](misc/spotlight_final.jpg?raw=true)

The scene is illuminated by two [spotlights](http://cs248.stanford.edu/winter19/lecture/materials/slide_035), a red spotlight coming from the front-left of the scene, and a white spotlight coming from the front-right.  The image below is a view from above the scene.

![View from above](misc/spotlight_above.jpg?raw=true)

#### Part 4.1 Adding Spotlights (15 points) ####

The first step in this part of the assignment is to extend your fragment shader for rendering spotlights. You will need to modify `media/shader_shadow.frag` for this task.  Note that if you have completed Parts 1-3 of the assignment, you can drop your solutions from `shader.vert` and `shader.frag` into this file so that you can render `media/spheres/spheres_shadow.json` with correct texture mapping, normal mapping, and environment lighting. 

1. Modify the `shader_shadow.frag' to compute the illumination from a spotlight by adding code to the body of the loop over spotlights.  Details about the implementation are in the starter code, but as a quick summary, a spotlight is a light that has non-zero illumination  only in directions that are within `cone_angle` of the light direction. Note that the intensity of a spotlight falls off with a 1/D^2 factor (where D is the distance from the light source). If you implement this logic, you should see an image that looks like the one below.  (Shown from front and from above).  In the image below, notice how the intensity of the spotlight falls off with distance.

![Hardspot spotlights](misc/spotlight_hard.jpg?raw=true)

2. Notice that since your implementation of spotlights cuts off all illumination beyond the cone angle, the spotlights feature a hard boundary between light and dark.  You can soften this boundary by linearly interpolating illumination intensity from near the edge of the cone.  Please see details in the code, but in our reference solution, intensity of the spotlight starts falling off once the angle to the surface point is at least 90\% of the cone angle, and drops to zero illumination at 110% of the cone angle. With this smoother spotlight falloff you will see an image like this.

![Softer spotlights](misc/spotlight_soft.jpg?raw=true)

#### Part 4.2 Adding Spotlights (15 points) ####

Now you will improve your spotlights so they cast shadows.  In class we discussed the [shadow mapping algorithm](http://cs248.stanford.edu/winter19/lecture/geometricqueries/slide_046) for approximating shadows in a rasterization-based rendering pipeline. Recall that shadow mapping requires two steps.
   1. In step 1, for each light source, we render scene geometry using a camera that is positioned at the light source. The result of rendering the scene from this vantage point is a depth buffer encoding the *closest point in the scene at each sample point*.  This depth buffer will be used as a single channel texture map provided to a fragment shader in step 2.
   2. In step 2, when calculating illumination during rendering (when shading a surface point), the fragment shader must compute whether the current surface point is in shadow from the perspective of the light source.  To do this, the fragment shader computes the coordinate of the current scene point *in the coordinate system* of a camera positioned at the light (in "light space"). It then uses the (x,y) values of this "light space" coordinate to perform a lookup into the shadow map texture.  If the surface is not the closest scene element to the light at this point, then it is in shadow.  

We have implemented the first step of shadow mapping for you.  (Interested students can read through the implementaton of `Scene::render_shadow_pass()` in `src/dynamic_scene/scene.cpp`, however knowledge of this implementation is not necessary to do your part of the assignment. What you do need to know is that the application starter code provides your shader with two texture maps (`shadowTextureSampler0` and `shadowTextureSampler1` in `shader_shadow.frag`) that encode depth buffer contents after rendering the scene from the two respective spotlight positions, and two transformation matrices (`obj2shadowlight0` and `obj2shadowlight1`) that transform object-space points into the coordinate system of the light.  Your job is to use these variables to compute shadows for the two spotlights lights. Note, your implementation will only compute shadows for up to two spotlights, since we only give you two shadow maps.
   
__What to do in the vertex shader:__

Your work in the vertex shader is quite easy.  Just transform the triangle vertex positions into light space, storing the results in vertex shader output variables `position_shadowlight0` and `position_shadowlight1`.  We won't say much more than this, since an example of how to transform these vertex positions into world space is already in the starter code. 

However, you might be wondering what is the definition of the "light space"?  It is a coordinate space where the virtual camera is at the position of the spotlight, looking directly in the spotlight's direction, and after applying perspective projection.  We've adjusted the transform so that *after homogeneous divide* vertices that fall on screen during shadow map rendering are in the [0,1]^2 range and valid scene depths between the near and far clipping planes are in the [0-1] range.  You encourage you to read more about the [coordinate systems of shadow mapping here](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping).

__What to do in the fragment shader:__

Your work in the fragment shader is a bit more complex.  The shader receives the light space *homogenous* position of the surface `position_shadowlight0` (for light 0), and must use this position to determine if the surface is in shadow from the perspective of this light.  Since the position is in 3D-homogeneous coordinates, you first need to extract a non-homogeneous XY via the homogeneous divide:

~~~~
  vec2 shadow_uv = position_shadowlight0.xy / position_shadowlight0.w;
~~~~
Now you have a screen-space XY that you can use to sample the shadow map, and obtain the closest scene point at this location.  You will need to test the value returned by the texture lookup against the distance between the current surface point and the light to determine if the surface is in shadow. (How do you compute this?)

At this point, you may notice errors in your image.  Read about the phenomenon of ["shadow acne"](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping) and correct any artifacts by adding a bias term to your shadow distance comparison.  Biasing pushes the surface being tested *closer to the light* so that only surfaces a good bit closer to the light can cast shadows on the current surface.  At this point, with a properly biased shadow test (the reference solution uses a quite large bias value of 0.05), you should get an image that looks like this (shown from a top view):

![Hard shadows](misc/shadows_hard.jpg?raw=true)

As you move the camera and look at the scene from different viewpoints, you might observe aliasing (jaggies) at the edges of your shadows.  One way to approximate a smoother shadow boundary is to take multiple samples of the shadow map around a desired sample point, and then compute the fraction of samples that pass the test.  This technique, which is quite costly because it requires making multiple texture lookups (a form of supersampling), is called __percentage closure filtering__. Inuitively, by perturbing the location of the shadow map lookup, PCF is trying to determine how close to a shadow boundary the current surface point is. Points slower to the boundary are considered to be partially occluded, and the result is a smoother gradation of intensity near the shadow boundary. The basic pseudocode for a 25-sample implementation of PCF is:

~~~~
float pcf_step_size = 256;
for (int j=-2; j<=2; j++) {
  for (int k=-2; k<=2; k++) {
     vec2 offset = vec2(j,k) / pcf_step_size;
     // sample shadow map at shadow_uv + offset
     // and test if the surface is in shadow according to this sample
  }
}
// record the fraction (out of 25) of shadow tests that are in shadow
// and attenuate illumination accordingly
~~~~

Notice that changing the parameter `pcf_step_size` governs how far apart your samples are, and thus has the effect of controlling the blurriness of the shadows.  Here is an example (from-above) of rendering with pleasing smooth shadows.

![Smooth shadows](misc/shadows_soft.jpg?raw=true)

You've done it!  You've now written a few key parts of a real time renderer!

### Extra Credit

There are many ways to go farther in this assignment.  Some ideas include:

* Implement other BRDFs that are interesting to you.  Google terms like "physically based shading".  

* Consider adding more sophisticated light types such as area lights via [linear transformed cosine](https://eheitzresearch.wordpress.com/415-2/)

* Improve the shadow mapping algorithm with [cascaded shadow maps](https://developer.download.nvidia.com/SDK/10.5/opengl/src/cascaded_shadow_maps/doc/cascaded_shadow_maps.pdf).

## Writeup

Please submit a short document explaining what you have implemented, and any particular details of your submission. If your submission includes any implementations which are not entirely functional, please detail what works and what doesn't, along with where you got stuck. This document does not need to be long; correctly implemented features may simply be listed, and incomplete features should be described in a few sentences at most.

The writeup must be a pdf, markdown, or plaintext file. Include it in the root directory of your submission as writeup.pdf, writeup.md, or writeup.txt.

Failure to submit this writeup will incur a penalty on the assignment.

## Handin Instructions

We are using [Canvas](https://canvas.stanford.edu) as our submission tool. You should create and upload a tar archive of your entire src subdirectory along with the writeup (e.g. writeup.txt) and scene submission (scene.json).
