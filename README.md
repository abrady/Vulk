# Vulk

This is the second renderer I'm taking a serious shot at after VulkanRenderer which was a great sandbox for doing lots of things but was weighing me down.

The goal of my previous project was to do enough hand-rolled stuff that I could really grok what the fundamentals of vulkan were. mission accomplished.

My goal for this project is to transition from the hand-coded samples I was doing before to a more data-driven engine capable of loading and rendering resources so I can focus on learning new rendering technologies by building them. One outcome I'd like is to have a modern renderer that displays lit rendered scenes on par with modern games (if not as efficiently or with as much capability)

# Building
* Clone the repo

Install the following. Note that CmakeLists.txt assumes these are in C:\Vulkan:
* Install [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
* Install [GLFW](https://www.glfw.org/)
* Install [GLM](https://glm.g-truc.net/0.9.9/index.html)
* Install [TinyObjLoader](https://github.com/tinyobjloader/tinyobjloader)
* Install [STB](https://github.com/nothings/stb)

* make sure your CMakeLists.txt points to the proper directory
* run `cmake -S . -B build` from the root of the project

# TODOs
* VulkResources should only need to live during the loading phase
* get some more models and make a scene. https://github.com/alecjacobson/common-3d-test-models ?
* clean up our vertex input buffers, too much crap

# Log

## 2/10/24 Gooch shading cont'd
Looks wrong. I'm not sure why, things look normal in renderdoc, let's try rendering normals and see if we can see what's going on. 
1. make the DebugNormals: pipeline, frag, geom, vert shaders
2. we need to load the pipeline in the scene and make the actors for that pipeline:
    1. the meshes and samplers can be shared
    2. the descriptor set for each actor needs to match the pipeline's layout

how VulkResources loads models for a given pipeline:
* one of the architectural decisions I made was that a binding value (for ubos, sbos, input vertices etc.) has a fixed interface, so specifying a binding is a contract that both sides will provide that given type, e.g. every scene is guaranteed to have a camera UBO or a VulkActor having a mat4 xform ubo.
* Another decision I made is that the pipeline owns the descriptor set layout. 
* given this, for our DebugNormals pipeline, we just need to take existing actors, grab their mesh and normal sampler, make a new descriptor set for it that matches the pipeline's layout, and we're good.

Screenshot of the normal map normals:
![](Assets/Screenshots/normal_map_normals_bad.png)

That isn't right. Let's check it with the normals from the geometry:
![](Assets/Screenshots/normal_map_geo_normals.png)

Okay, so the normal rendering is correct, a little looking around leads me to....

### Tangent Space
In retrospect this makes a lot of sense, but normal maps are stored in tangent space which is just
a fancy way of saying they're in a space relative to the vertices closest to them (at least that's how I'm internalizing what I read)

So: each vertex needs to be multiplied by a matrix that transforms it into model space before it then gets transformed into world/projection/clip space.

What we'll do is pass in the normal and tangent with each vert, use that to make a cotangent vert
and then do this first transformation.

Also, we apparently need to double and then subtract 1 from the normal I'm guessing because the floating point format can't handle negatives? yep, that appears to be it due to colors being in 0 - 1

* sampledNormal = sampledNormal * 2.0 - 1.0; // Remap from [0, 1] to [-1, 1]
* vec3 T = normalize(vec3(model * vec4(vertexTangent, 0.0)));
* vec3 N = normalize(vec3(model * vec4(vertexNormal, 0.0)));
* vec3 B = cross(N, T) * vertexTangent.w; // vertexTangent.w should be +1 or -1, indicating handedness
* mat3 TBN = mat3(T, B, N);



## 2/6/24 Gooch shading
* designed to increase legibility in technical drawings

goals:
* implement gooch shading
* do it in a way that is selectable for what is being rendered - i.e. render this scene as gooch, that scene as phong or whatever

Gooch in detail:
* compare the normal direction with the light, the more it faces the light the warmer the color




## 2/3/24 bootstrapping and resource loading
I took the VulkanRender project, copied it, and need to clean out the cruft.

define a model as the simplest a set of things required to render something:
* vert/index buffers
* textures for it
* globals (UBOs/SBOs)
* descriptor sets
* pipeline

the cardinality of these things with respect to the model is:
* model:vert/index bufs = 1+:1+ - we may accumulate 
* model:textures = 1+:1 - multiple models might share a texture if atlased
* model:globals = 1+:1 - some UBOs are shared (e.g. materials), while some are model specific
* descriptor sets = 1:1+ - hypothetically two models that use the same textures and globals could share a descriptorset
* 


* pipeline: (shaders, vertex input, descriptor set layout)
    * render a model: cardinality to:
        * vertex buffers: 1:1 (or many to 1) - take a ref to
        * 

cardin 


