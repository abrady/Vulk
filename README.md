# Vulk

This is the second renderer I'm taking a serious shot at after VulkanRenderer which was a great sandbox for doing lots of things but was weighing me down.

The goal of my previous project was to do enough hand-rolled stuff that I could really grok what the fundamentals of vulkan were. mission accomplished.

My goal for this project is to transition from the hand-coded samples I was doing before to a more data-driven engine capable of loading and rendering resources so I can focus on learning new rendering technologies by building them. One outcome I'd like is to have a modern renderer that displays lit rendered scenes on par with modern games (if not as efficiently or with as much capability)

# Resources

* <https://learnopengl.com/Advanced-Lighting/Normal-Mapping> - good tangent space intro

# Setup

* Install [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
* make sure your VULKAN_SDK environment variable is set
* Clone the repo
* mkdir build
* make sure you have git lfs installed. you may need to do a git lfs pull.

# Building

* cd build
* cmake ..
* cmake --build .
* (optional - run tests) ctest -C Debug (or however you built it) in the build directory

# TODOs

* invert the TBN matrix: So now that we have a TBN matrix, how are we going to use it? There are two ways we can use a TBN matrix for normal mapping, and we'll demonstrate both of them:
  * We take the TBN matrix that transforms any vector from tangent to world space, give it to the fragment shader, and transform the sampled normal from tangent space to world space using the TBN matrix; the normal is then in the same space as the other lighting variables.
  * We take the inverse of the TBN matrix that transforms any vector from world space to tangent space, and use this matrix to transform not the normal, but the other relevant lighting variables to tangent space; the normal is then again in the same space as the other lighting variables. - this is better because we can do this in vertex space and then use the interpolated values.
* <https://github.com/KHeresy/openxr-simple-example> : integrate with OpenXR
* <https://www.reddit.com/r/GraphicsProgramming/comments/1ay0j70/realtime_pbr_catchup_developments_in_recent_years/> - chock full of links

# Log

## 3/16/24 screw it, I'm jumping into PBR

Physically Based Rendering/Shading or whatever is the current state of the art?

* <https://www.reddit.com/r/GraphicsProgramming/comments/1ay0j70/realtime_pbr_catchup_developments_in_recent_years/> - chock full of links

For a PBR lighting model to be considered physically based, it has to satisfy the following 3 conditions (don't worry, we'll get to them soon enough):

Be based on the microfacet surface model.
Be energy conserving.
Use a physically based BRDF.

## 3/17/24 todo time part 2

* DONE rendering
  * vkCmdBindVertexBuffers can take an array
    * the VkVertexInputBindingDescription specifies which buffer corresponds to which location
* buildpipelinetool should generate bindings automatically

## 3/16/24 todo time

* rename PipelineBuilder DONE
* I've wanted to get the vertex struct broken up for a while. let's do that... the right way to do this is probably at the GPU buffer level, no need to break it up beforehand I think?
  * vertex has the following fields:  pos, normal, tangent, uv
  * DONE this happens in VulkMesh? VulkModel!
    * needs a set of buffers, one for each field
    * these need to be in an array for passing to vkCmdBindVertexBuffers
  * DONE currently pipelines specify the binding, instead we should have a field specifying what they use
    * why did I think vertexInputBinding was a good idea? remove that
    * read the values from pipeline and set
* DONE get the pipeline builder to work properly
  * x we need to include a VkVertexInputBindingDescription and a VkVertexInputAttributeDescription for each type in the vertex
  * x the location in the shader seems like a good approach: LayoutLocation_Position, LayoutLocation_Normal, etc.

### Refresher: VkVertexInputBindingDescription and VkVertexInputAttributeDescription

* the binding description specifies how the buffer binds to vkCmdBindVertexBuffers. e.g. vkCmdBindVertexBuffers(commandBuffer, 0, numBufs, bufs, offsets);
  * 0 is the first binding number you're starting with.
  * numVertBufs is the number of buffers you're binding.
  * model->vertBufs is the array of buffer handles, where model->vertBufs[1] should indeed be the normals buffer, given that its corresponding binding value is 1.
  * offsets is an array of offsets into the buffers being bound; each offset corresponds to its respective buffer in model->vertBufs.
* the attribute description specifies how these map to the location in the shader
  * it is connected to the binding via the 'binding' field.
  * it specifies the vert shader location via the 'location' param, duh.
  * it has a type so the GPU knows how to interpret it, e.g. VK_FORMAT_R32G32B32_SFLOAT
  * for complex types it also has an offset into the struct. e.g. 4 for the second thing in the struct.

## 3/16/24 [Parallax Mapping](https://learnopengl.com/Advanced-Lighting/Parallax-Mapping)

The core idea behind parallax mapping is to adjust the texture coordinates based on the viewer's angle relative to the surface, simulating the parallax effect that occurs when viewing objects at different distances. This is achieved by using a height map stored in a texture, which encodes the relative height of each pixel on the surface.

There are several variations of parallax mapping, with the most common being:

* Basic Parallax Mapping: The simplest form, which uses a straightforward shift of texture coordinates but can sometimes result in less convincing depth effects, especially at steep angles.
* Parallax Occlusion Mapping (POM): An advanced version that improves upon basic parallax mapping by better simulating the occlusion and self-shadowing of the bumps and pits on the surface. POM provides a more realistic sense of depth by tracing rays through the height map to determine the visible surface point, offering significantly improved realism at the cost of higher computational expense.

Apparently this is seldom used in production. not interested, Next!

## 3/16/24 PCF - percentage close filtering

Another (partial) solution to these jagged edges is called PCF, or percentage-closer filtering, which is a term that hosts many different filtering functions that produce softer shadows, making them appear less blocky or hard. The idea is to sample more than once from the depth map, each time with slightly different texture coordinates. For each individual sample we check whether it is in shadow or not. All the sub-results are then combined and averaged and we get a nice soft looking shadow.

I reduced the shadowmap to 256x256 (approximately)

![](Assets/Screenshots/shadowmap_256_nopcf.png)

Let's see what adding some neighbor sampling does

![](Assets/Screenshots/shadowmap_256_pcf.png)

that looks identical. let's double check that we're actually doing anything. hmm, looks fine in renderdoc. I think 256x256 is just too small. let's try 512... nope looks basically the same.
I'm guessing spheres don't work well with it. Next!

## 3/16/24 Peter panning

One of the downsides of the bias factor is that you can ignore things that should be shadowed, but there's a trick! normally back-facing triangles are culled
by the graphics pipeline, but in the case of shadows, assuming closed surfaces, you can cull the front faces. let's enable that and see if it fixes some of these issues

Here's the sphere without front face culling:

![](Assets/Screenshots/shadowmap_without_front_face_culling.png)

with front face culling:

![](Assets/Screenshots/shadowmap_cull_front_face.png)

looks a lot better!

## 3/13/24 yet another dumb refactor

* I was annoyed with how tedious adding new things to json parsing was, I looked around and decided  [Cereal](https://uscilab.github.io/cereal/) was the way to go
* this turned out to be more of a pain then I thought it would be but I learned a bit.

What I learned:

1) trying to write a custom serializer for anything that could be a fundamental type or a single param ctor type that wasn't marked explicit will screw you

* fs::path : tried this, started getting duplicate definition of string errors
* Any of the Vk* types that I wanted to store in nice strings was a no-go, they're all VkFlags and you'd get duplicate def errors

2) I mixed source assets and built assets.

* Why this was a problem:
  * I'd hacked this a bit in the 'pipelinedecldef'/'pipelinedef' structs but that was a hack
  * Not having a clean divide between built assets and source was confusing, and even when looking at existing tests I nearly broke them.
* source data is different than built data. initially I was just copying things over, but once I started building pipelines this became clearly necessary
* having a 'SourceXXXDef' and 'BuiltXXXDef' allows me to cleanly keep variables that only show up in the built side out of the source data and avoiding confusion
  * right now BuiltXXXDef derives from SourceXXXDef. is this dumb? probably, but the alternative is duplicate copying of a bunch of fields.

## 3/10/24 lightmap works

![](Assets/Screenshots/lightmap_basic_works.png)

In the end the dumb thing I did was this: you need to normalize the x and y from -1 to 1 to 0 to 1 by halving and adding .5 to them. I was doing this to the z value as well

feeling dumb that this took so long, but also feeling good that I figured it out!

What did I learn with this?

* nearly working might be hiding a big bug
* dig deep into small problems and find the simplest repro case you can

## 3/10/24 lightmap kinda works

But something is funny. if I

* take a quad 12/12 with 6 verts
* put the camera at 0,0,10
* put the light at 0,0,10,
* have the same frustum for both

then I would expect the depth to be the same, right? It's not happening

Looking in renderdoc I see:

* currentDepth: 0.97138041
* closestDepth: 0.94276088

So the sampled depth is almost .03 closer, why?

on the c++ side I have:

* ubo.view [[1 -0 -0 0] [0 1 -0 0] [0 0 1 0] [-0 -0 -10 1]]
* lightView [[1 -0 -0 0] [0 1 -0 0] [0 0 1 0] [-0 -0 -10 1]]
* ubo.proj  [[1.81066 0 0 0] [0 2.41421 0 0] [0 0 -1.0101 -1] [0 0 -1.0101 0]]
* lightProj [[1.81066 0 0 0] [0 2.41421 0 0] [0 0 -1.0101 -1] [0 0 -1.0101 0]]

So the view and projection matrices are the same.

viewProj

* row 1: [1.81066 0 0 0]
* row 2: [0 2.41421 0 0]
* row 3: [0 0 -1.0101 9.09091]
* row 4: [0 0 -1 10]

Let's check the shadow transformation:

* viewProjMatrix.row0: 1.81066012, 0.00, 0.00, 0.00
* viewProjMatrix.row1: 0.00, 2.41421342, 0.00, 0.00
* viewProjMatrix.row2: 0.00, 0.00, -1.01010096, 9.09090805
* viewProjMatrix.row3: 0.00, 0.00, -1.00, 10.00

same as viewProj

Let's see what the lighting pass is doing:

* depth outPosLightspace: x, y, 14.14141273 15
* light outPosLightspace: x, y, 14.14141273 15

## 3/10/24 lightmap generation is messed up

trying to get more data about why I'm seeing the funny issue on the edges of things I done broke my lightmap creation. sigh. good learning experience though.

Let's start by debugging the verts. the problem is that it is hard to interpret projection matrices, so let's see what we can learn about them:

* NDC: normalized device coordinates x, y should be between -1 and 1 and z is 0-1 (in opengl it is -1 to 1, just watch out)

![](Assets/Screenshots/NDCs_badmaybe.png)

I do see that my NDCs are all very close to 1 in the z value. it feels like if the far plane is at, say, 100, than they should be much closer to -1 if I understand this correctly

This seems like a good thread to pull on:

### Projection matrix refresher

Vulkan uses a right handed coordinate system and a depth range of 0 to 1

```c
[ f/aspect, 0,     0,              0             ]
[ 0,        f,     0,              0             ]
[ 0,        0,     (f+n)/(n-f),    fn/(n-f)     ]
[ 0,        0,     -1,             0             ]
```

simplified perspective matrix. the secret sauce is in the -1 in the third column: it sets the w to the z value so things further away down the z axis get shrunk down

* n = near clip (.1)
* f = far clip (100)
* aspect = aspect ratio (1)

```c
[ 100,      0,     0,              0             ]
[ 0,      100,     0,              0             ]
[ 0,        0,  (100.1)/(-99.9), 10/(-99.9)      ]
[ 0,        0,     -1,             0             ]
```

## 3/8/24 how do we fix this issue

### first make sure we understand it

we're checking if currentDepth > closestDepth + 0.01 for shadowing.

example pixel that is not shadowed:

* currentDepth 0.98946941
* closestDepth 0.98885888
* 0.00061053 deeper than closest depth, but this fails the 0.01 test, so is considered in the light

example pixel that is shadowed:

* currentDepth 0.98946172
* closestDepth 0.97924232
* 0.0102194 deeper, just over the 0.01 threshold, so in shadow

note that the non-shadowed has a greater depth! by 0.00000769. it would appear that the texture sampling is off by just enough that passes while the other succeeds

so: I think the problem is how we're sampling the depth texture. I wonder if we can introduce anything to fix this? like what if we take multiple samples?

## 3/7/24 more goofing around

![](Assets/Screenshots/shadowmapbug2.png)

Here it looks a little strange, kinda like the sphere is occluding itself?

![](Assets/Screenshots/shadowmap_bug2_closer.png)

However if I move the camera just a scooch closer it suddenly works.

1. for sure it looks like part of the sphere's shadow is falling outside the light's frustum for the shadowmap, so that's easy to fix.
2. this stange striping on the sphere is strange. here's another screenshot:

![](Assets/Screenshots/shadowmap_sphere_bug3.png)

almost looks like a nan type error. to renderdoc!

Okay, it is pretty clearly related to sampling:

* the artifacts are all in shadow while the other parts aren't
* when I debug I can find a frag further back, but the depth sample is also further back so it isn't shadowed.

conclusion: my depth buffer is the problem. some investigation:

* the depth buffer is 800x600, because I just copy-pasta'd some code. changing that to 1024x1024
* the format has a stencil buffer, we don't really need that, I should get rid of that...

## 3/6/24 shadowmap renderpass part 2

Let's fire up renderdoc and see what's going on with this depth buffer not looking the way I expect it to

viewProj matrix for the light cpu side:

```c++
  row 1: [0.310685  0        -0.950513 3.82051]
  row 2: [0.731499  0.638546  0.239098 -1.47435]
  row 3: [-0.607553 0.770354 -0.198585 5.02655]
  row 4: [-0.606946 0.769584 -0.198387 5.12152]
```

in the vert shader:

* viewProjMatrix.row0 _35.viewProjMatrix.row0 float4 0.31068492, 0.00, -0.95051295, 3.82051015
* viewProjMatrix.row1 _35.viewProjMatrix.row1 float4 0.73149949, 0.63854563, 0.23909813, -1.47435224
* viewProjMatrix.row2 _35.viewProjMatrix.row2 float4 -0.60755348, 0.77035427, -0.19858511, 5.02654839
* viewProjMatrix.row3 _35.viewProjMatrix.row3 float4 -0.60694593, 0.76958394, -0.19838652, 5.12152195

Looks good. now let's view the shadowmap:

![](Assets/Screenshots/shadowmap1.png)

Well it looks like the sphere is in the view, but the plane isn't. hypothetically this should work though...

Let's see what's happening when we try to use it to actually light the model

* closestDepth _195.x float 0.98442852
* currentDepth _203.x float 0.98949158

These are 0.00506306 apart, which is causing the test to fail. let me try upping my bias here to .006 and see what happens.

![](Assets/Screenshots/spherelitagain.png)

That seems to have worked, but the background still looks like ass, why?

![](Assets/Screenshots/shadowmap_clamped_values.png)

Aha! the background being outside the light's frustum was causing UVs outside the 0-1 range. The best fix, I assume, is to deal with this in the sampler:

* samplerInfo.addressModeU/V = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER
* samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

These two things result in 1.0 for the depth if outside the frustum, a little indirect, but problem solved.

The next thing is that black stuff at the bottom right which I did not include in my screenshot. why is that happening?

Ah! the z value was also outside the frustum. you gotta be careful with these frustums (frusta?) apparently. here's the fix:

* float currentDepth = clamp(projCoords.z, 0.0, 1.0);

holy cow, except for making things in shadows totally black, it actually looks like it is working! cool!

## 3/5/24 renderpass, shmenderpass

My efforts at synchronization have been failing. Vulkan was warning me about not having internal dependencies when I tried to put a pipeline barrier in. I could spend
more time trying to get it working so I can learn about this in more detail, but instead it looks like the best practice is to have two renderpasses.

Time for a refresher...
a renderpass describes the operations you perform to render to a framebuffer.

It has:

* framebuffer is what is rendered to
* subpass+ do the actual rendering
* each of which has a pipeline defines the shaders etc.

Okay, refactoring done, here's the result:

![](Assets/Screenshots/shadowmap_bad.png)

Hey! yes this is definitely incorrect, but we no longer have incorrect matrices, so I just need to debug this. progress!

## 3/4/24 debugging the shader map

Not getting any shadows:

![](Assets/Screenshots/buggy_shadow_map.png)

It's pretty clear the shadow map is using the view's depth buffer and not the light's view. let's debug... okay so the shadowmap creation shader is clearly getting the ubo for the view transform. probably shouldn't have used the same value for both.

so...

1. I could put a pipeline barrier to make sure the ubo is used
2. I could use a different ubo for the light,

2 is probably the right long term approach, but let's try the mem barrier just to learn how it works:

```c
// before this: update the UBO

VkMemoryBarrier memoryBarrier = {};
memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT; // After CPU UBO update
memoryBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT; // Before vertex shader reads UBO

vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_HOST_BIT, // After this stage
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, // Before this stage
    0, // Flags
    1, &memoryBarrier,
    0, nullptr, // No buffer memory barriers
    0, nullptr  // No image memory barriers
);
```

Similar to the image transition:

* specify the before/after stage where this barrier can go

Different than the image transition:

* VkImageMemoryBarrier is the type for the transition, vs. VkMemoryBarrier here

Note that src/dstAccessMask and the VK_PIPELINE_... params work together to define what the barriers are,
e.g. all HOST_WRITEs must be finished in the HOST stage
before any UNIFORM_READs can happen in the VERTEX_SHADER

## 3/3/24 sampling the depth

Now that we have the depth buffer we need to get the closest depth for each vert:

1. project each vert just like before for generating the lightmap, pass this as an output of the vertex shader to the frag shader as e.g. out/inPosLightSpace
2. the frag shader then gets the inPosLightSpace in clip coords and divides by w to get into NDC (normalized device coordinates) space. the z value of this is your depth.
3. by convention this is from -1 to 1 in the x and y axes but your depth buffer is a texture and expects things in the range of 0-1 so just multiply by .5 and and add .5
4. at this point you have a uv for the depth buffer, sample it, and compare that with the depth value of the fragment's depth from step 2 and you know if it is visible to the light nor not.

Running into some speedbumps:

### ds updater specified VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL but the shadowmap is VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL

why does the graphics card care? VkImageLayout is an enumeration in Vulkan that specifies the layout of image data. The layout of an image represents the arrangement of the image data in memory and can affect how the image data is accessed.

I need to transition this image view to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL using pipeline barriers, how exciting. I've been wanting to get some of this.

To do that we make a pipeline memory barrier. At a high level you say:

1. here's what the current type is and what I want it to be (old/newLayout)
2. what part of the image to convert: depth/color/stencil
3. after which the op starts and before which it has to finish (pipline state frag test/frag shader)

This is the call to to create the barrier, see the annotations:

```c
VkImageMemoryBarrier barrier = {};
barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
barrier.oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;      // 1. old layout: this is a depth buffer
barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;              // 1. new layout: we want it to be readable by a shader
barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
barrier.image = depthImage;                                                // image to convert
barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;           // 2. we only care about the depth part of the image
barrier.subresourceRange.baseMipLevel = 0;
barrier.subresourceRange.levelCount = 1;
barrier.subresourceRange.baseArrayLayer = 0;
barrier.subresourceRange.layerCount = 1;
barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
vkCmdPipelineBarrier(
    commandBuffer,
    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, // the stage we're coming from: Depth/stencil attachment output stage
    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, // the stage we're going to be using this in: Fragment shader stage
    0,
    0, nullptr,
    0, nullptr,
    1, &barrier
);
```

#### Key Parts of this

##### oldLayout/newLayout

we're telling the GPU to turn this memory from a format we can write to, to one that can be used in a sampler for getting depth values.

The layout of an image can be changed using a layout transition, which is performed using a pipeline barrier. When you transition an image to a new layout, you need to specify the old and new layouts in a VkImageMemoryBarrier structure, and then submit this barrier using vkCmdPipelineBarrier.

Here are the possible values for VkImageLayout:

* VK_IMAGE_LAYOUT_UNDEFINED: The image layout is not known or does not matter. This is often used when an image is first created or after it has been written to, before it's transitioned to a more specific layout.
* VK_IMAGE_LAYOUT_GENERAL: The image can be accessed by any pipeline stage.
* VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: The image is optimal for use as a color attachment in a framebuffer.
* VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: The image is optimal for use as a depth/stencil attachment in a framebuffer.
* VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: The image is optimal for read-only access in a shader, and is also a depth/stencil attachment.
* VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: The image is optimal for read-only access in a shader.
* VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: The image is optimal for use as the source of a transfer command.
* VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: The image is optimal for use as the destination of a transfer command.
* VK_IMAGE_LAYOUT_PREINITIALIZED: The image data has been preinitialized and can be read by a transfer operation without needing a layout transition, but the image is not fully prepared for optimal use in other operations.
* VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL: The image is optimal for read-only depth access in a shader, and is also a stencil attachment.
* VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL: The image is optimal for read-only stencil access in a shader, and is also a depth attachment.
* VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: The image is optimal for presentation with a swapchain.

##### srcAccessMask/dstAccessMask

* VK_ACCESS_INDIRECT_COMMAND_READ_BIT: Indicates that a command read operation can occur.
* VK_ACCESS_INDEX_READ_BIT: Used for reading from an index buffer.
* VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT: Specifies that a vertex buffer can be read.
* VK_ACCESS_UNIFORM_READ_BIT: Indicates that reading from a uniform buffer is possible.
* VK_ACCESS_INPUT_ATTACHMENT_READ_BIT: Used for reading an input attachment within a fragment shader.
* VK_ACCESS_SHADER_READ_BIT and VK_ACCESS_SHADER_WRITE_BIT: Specify that shader reads and writes can occur, respectively.
* VK_ACCESS_COLOR_ATTACHMENT_READ_BIT and VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT: Indicate that operations can read from and write to a color attachment, respectively.
* VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT and VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT: Used for depth and stencil buffer accesses.
* VK_ACCESS_TRANSFER_READ_BIT and VK_ACCESS_TRANSFER_WRITE_BIT: Specify that transfer operations can read from or write to a resource.
* VK_ACCESS_HOST_READ_BIT and VK_ACCESS_HOST_WRITE_BIT: Indicate that the host (CPU) can read from or write to a resource.

##### stages (VkPipelineStageFlags) VK_PIPELINE_STAGE_*

Just your standard pipeline stages.

* VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT: This is the earliest point in the pipeline. Using this flag typically means that the operation could be executed at any stage in the pipeline.
* VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT: This stage is where indirect draw call parameters are read from a buffer.
* VK_PIPELINE_STAGE_VERTEX_INPUT_BIT: This stage is where vertex and index buffers are read.
* VK_PIPELINE_STAGE_VERTEX_SHADER_BIT: This stage is where vertex shader operations are performed.
* VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT and VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT: These stages are where tessellation control and evaluation shader operations are performed, respectively.
* VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT: This stage is where geometry shader operations are performed.
* VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT: This stage is where fragment shader operations are performed.
* VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT and VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT: These stages are where depth and stencil testing are performed.
* VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT: This stage is where color values are written to color attachments.
* VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT: This stage is where compute shader operations are performed.
* VK_PIPELINE_STAGE_TRANSFER_BIT: This stage is where transfer operations (like copying data between buffers or images) are performed.
* VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT: This is the latest point in the pipeline. Using this flag typically means that the operation doesn't have any specific stage dependencies and could be executed at any later point in the pipeline.
* VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT: This is a shortcut for specifying all stages of the graphics pipeline.
* VK_PIPELINE_STAGE_ALL_COMMANDS_BIT: This is a shortcut for specifying all stages of both the graphics and compute pipelines.

These flags can be combined using bitwise OR operations to specify multiple stages. For example, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT would specify both the vertex shader and fragment shader stages.

## 3/2/24 shadow mapping: render the shadow map

It looks like to make a shadowmap I need to make my own renderpass to make the imageview that I can sample from. so let's build that and see what happens:

* make a VulkDepthRenderpass which
  * has a depth imagview
  * renders the camera's view of the scene

One thing right now is that SampleRunner only has one implicit renderpass. Let's not refactor anything just yet, but consider pulling the renderpass into the scene or something in the future

Let's see what we got:
![](Assets/Screenshots/depth_buf_screenshot.png)
Here's the depth buffer as it is rendered.

![](Assets/Screenshots/depth_draw_calls.png)

And here are the depth only drawcalls.

One thing to keep in mind with renderdoc is that you can scale the visualization, so if everything show up white you can click this to fix the scale by clicking that wand in the top right corner. you can see the range change to 0.997 - 1 so the contrast is clearer:

![](Assets/Screenshots/scaled_depth_buffer_values.png)

## 2/27 [Shadow Mapping](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping)

* we don't have a great rasterized shadow algorithm.
* one basic approach is to render a scene from the light's point of view, keep a depth buffer for it, and then sample that when lighting a given pixel.

![](Assets/Screenshots/shadow_map_creation.png)

## 2/24 Brightness and gamma correction

![](Assets/Screenshots/brightness_gamma_correction.png)
from <https://learnopengl.com/Advanced-Lighting/Gamma-Correction>:
    * The top line looks like the correct brightness scale to the human eye, doubling the brightness (from 0.1 to 0.2 for example) does indeed look like it's twice as bright with nice consistent differences. However, when we're talking about the physical brightness of light e.g. amount of photons leaving a light source, the bottom scale actually displays the correct brightness. At the bottom scale, doubling the brightness returns the correct physical brightness, but since our eyes perceive brightness differently (more susceptible to changes in dark colors) it looks weird.

basically the human eye has a non-linear response to brightness, whereas digital cameras etc. tend to capture and display things with linear changes in intensity which look 'wrong' to the human eye.

Enter Gamma correction which adjusts the luminance or the brightness of an image to account for this

This process involves encoding and decoding luminance values using a gamma curve:
![](Assets/Screenshots/gamma_curve1.png)

(side note: the equation for this is simply V_gamma = V_linear^γ where γ=2.2 typically. the 2.2 value approximates sRGB space)

You can see here that a linear brightness of .5 turns into a gamma-corrected brightness just over .2. and if you doubled the linear brightness to 1, you'd get a gamma-corrected brightness of 1 as well so you think you're making
the color twice as bright, but it ends up being almost 5 times as bright.

Why this matters:

1. you need to make sure your textures are in linear space: textures are typically made in a format calls sRGB which is the gamma corrected color space. when you load textures you need to make sure you either convert them to linear space or (better) use Vulkan's `VK_FORMAT_R8G8B8_SRGB` flag when loading so that it is correct automatically (you may remember I was doing this accidentally when loading normal maps which was messing them up)
2. do all your calculations in linear space
3. output your colors in sRGB (gamma corrected) space: you can do this yourself with the above equation, or make sure your swapchain buffers are in a Vulkan supported format to get this to happen automatically (format = `VK_FORMAT_B8G8R8A8_SRGB` and colorSpace = `VK_COLOR_SPACE_SRGB_NONLINEAR_KHR`)

### sRGB Textures

So artists are making textures using monitors displaying things in sRGB space. Vulkan linearizes these automatically if loaded properly.

I guess that's about it...

## 2/19 cmake fun

I learned a little more about cmake and vcpkg and refactored the whole project as a result. did I learn anything interesting? yeah

* vcpkg works pretty well:
  * make a vcpkg.json and add the packages you need in the "dependencies" section.
  * `find_package(nlohmann_json CONFIG REQUIRED)` then pulls it into the project you're building
  * `target_link_libraries(Vulk PRIVATE nlohmann_json::nlohmann_json)` will then add it to your project (vcpkg tells you how to do this per project btw.)
* the root project just includes the child directory projects with `add_subdirectory` to bring it all together:
  * `add_subdirectory("${CMAKE_SOURCE_DIR}/Source/Samples")` now has the main.cpp which builds whatever sample we're rendering.
* moving Vulk into a library was surprisingly easy
  * `find_package` all the externalities
  * `add_library(Vulk ${SOURCES})`
* using vulk just requires `target_link_libraries` just like the vcpkg deps:
  * e.g. `target_link_libraries(PipelineBuilder PRIVATE Vulk)`
* I also accumulated all the assets being built into `build_assets` so I can depend on that in pipelinebuilder:
  * `add_dependencies(PipelineBuilder compile_shaders build_assets)`

## 2/17 pipeline/shader annoyances part 2

having thought about it some more, the root problem is that it is easy to have a shader use, say, a UBO, but the pipeline doesn't declare it, or an upstream shader to not bind properly to a downstream one (e.g. incorrect outputs to inputs). it feels like this could be pretty easy to fix:

* shaders themselves export their dependencies, can we generate them from that?

I just wrote some tests that reader a spirv file and checks the bindings, it works as expected.

so next I just need to:

* make the code to load a pipeline and the respective shaders referenced by the pipeline
* check that the inputs and outputs match
* write out the pipeline file in the build director
* wire this up to cmake

## 2/14 pipeline annoyances

* it sucks to define the pipeline and the inputs and then get runtime errors
* what would be nice would be some way to tie this all together. how can I do this?

1. parse the shader and generate the piepline: cons - too fiddly
2. create an upstream file that generates the shaders but you fill out what they do
    * cons: still tedious!
3. upstream file where you write the shader code and define inputs and outputs?

what would that look like?

* ubos: could be any stage
* samplers: any stage
* in verts: only vertex stage
* outputs feed into inputs in the next stage

XFORMS_UBO(xform);
MODELXFORM_UBO(modelUBO);
VERTEX_IN(inPosition, inNormal, inTangent, inTexCoord);
VERTEX_OUT(outPos, outNorm, outTangent, outTexCoord);

```c++
// FRAG
//  VERT_IN: inPos, inNorm, inTan, inTex
@ubo(XformsUBO xformsUBO, ModelXform modelUBO)
void vert(in Pos inPos, in Norm inNorm, in Tan inTan, in TexCoord inTex, out  )
{
    mat4 worldXform = xform.world * modelUBO.xform;
    gl_Position = xform.proj * xform.view * worldXform * vec4(inPosition, 1.0);
    outTexCoord = inTexCoord;
    outPos = vec3(worldXform * vec4(inPosition, 1.0));
    outNorm = vec3(worldXform * vec4(inNormal, 0.0));
    outTangent = vec3(worldXform * vec4(inTangent, 0.0)); 
}

@ubo(EyePos eyePos, Lights lights, MaterialUBO materialUBO)
@sampler(TextureSampler texSampler, TextureSampler normSampler)
void frag(in Pos fragPos, in TexCoord fragTexCoord) 
{
    vec4 tex = texture(texSampler, fragTexCoord);
    vec3 norm = vec3(texture(normSampler, fragTexCoord));
    outColor = blinnPhong(tex.xyx, norm, eyePosUBO.eyePos, lightBuf.light.pos, fragPos, lightBuf.light.color, true);
}
```

## 2/12/24 wrapping up sampled normals debugging

I'm curious about the multiple tangents coming off of my verts, what is going on here? Ah! when you subdivide adjacent tris you'll duplicate verts if you're not
careful. quick and dirty vert lookup seems to be working. good enough.

## 2/11/24 debugging

![](Assets/Screenshots/quad_correct_tangents.png)

Quads look correct tangent-wise

Looking at the normal map directly:

* first these normals aren't normalized? maybe I'm loading them wrong, let's ee
* oooh, I'm loading them as VK_FORMAT_R8G8B8_SRGB, but that automatically converts the texture from sRGB space to linear space (i.e. gamma un-corrects them)
* what I need to do is use the _UNORM format.
* also I see this only has 3 channels but I'm loading 4, does that matter? let's see

![](Assets/Screenshots/fixed_texture_normals.png)

Look at that! just had to load the normals in _UNORM format and not_SRGB which was doing some sort of correction. let's see the sphere:

![](Assets/Screenshots/fixed_sphere_normals.png)

looking good too!

## 2/11/24 Implementing tangent space

<https://learnopengl.com/Advanced-Lighting/Normal-Mapping>

Reading this I can see that I need to be more careful about how I implement the tangent vector - something something UV mapping.

Imagining a triangle and its UVs you can imagine that the 2D set of UVs being interpolated across needs to match up with what the normal
map has stored. At its very simplest you could imagine P0,P1, and P2 each having a normal. You sample that normal but then you'd have the
wrong basis for transforming it into world space so it would be incorrect.

Let

* E1 = P1 - P0
* E2 = P2 - P1
* dUV0 = UV1 - UV0
* dUV1 = UV2 - UV0
* T = the tangent
* B = the bitangent

then by convention:

* E1 = dUV0.U *T + dUV0.V* B
* E2 = dUV0.U *T + dUV0.V* B

That is, T and B make a basis for E1 defined by the dUV0:

Imagine a matrix [T, B] that forms a basis M for a coordinate system in this triangle P0 at origin, E1 at M . dUV0 and E2 at M . dUV1

let EM = [E1, E2]^T

You can find the inverse of dUV1 and post-multiply both sides by it and you get:
EM . dUV1_inv = M

or something like that.

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

i.e. we make basis in model space based on the tangent and normal (and the derived bitangent) and project the tangent space vectors onto it.

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

# Done Todos

* DONE VulkResources should only need to live during the loading phase
* x rename PipelineBUILDER.h to PipelineCOMPILER.h or something similar: builders are what I call the foo.bar.baz.build() paradigm
* x clean up our vertex input buffers so we can handle passing only parameters we use.
  * x fix up debugtangents/normals too
* x probably need schematized json files at some point. flatbuffers looks like the winner based on some quick research
* DONE now that I'm using flatc for enum metadata I could generate common.glsl in the build tools...
