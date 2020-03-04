CMP502 – Programming for Games

Assessment Report by Ridam Rahman

1804335@uad.ac.uk

Introduction

This section contains brief information about the application and how to interact with it through user controls. It is fundamentally a 3D environment, built using the DirectX11 Software Development Kit. Once launched, the application will display a room, featuring a spinning globe in the middle of the room, a teapot orbiting around the globe, and a greenish skull a bit further away near the end of the room. Getting closer to the skull will change the lighting effect on the skull, as well as the volume of the sound playing in the background. There's also a rudimentary HUD, featuring four triangles constantly moving back and forth on the display.

To interact with the environment, the user has the following inputs:

--** W or Up**: forward camera movement;
--** A or Left**: move camera to the left;
--** S or Down**: backward camera movement;
--** D or Right**: move camera to the right;
--** X**: move camera downward;
--** Space**: move camera upward;
--** Left Click on the mouse (hold)**: change the camera pointing direction according to the mouse pointer.
Development

A step-by-step insight on the application building process, focused on the features that have been implemented, and how this has been achieved.

Setup

Given the preinstalled IDE available on the machine upon which the application has been developed, i.e. Visual Studio 2019, the first thought that occurred was if an older environment was required, or if it was possible to create a DirectX 11 application from scratch using what was already available. After some research, the answer to this question came up on the Microsoft DirectX Tool Kit wiki (Walbourn, 2019).

According to the official documentation by Microsoft, the DirectX SDK is now deprecated, and it is included in the Windows SDK. Nonetheless, this SDK features only a single game template, meant for the Universal Windows Platform. Luckily, the DirectXTK Wiki features a downloadable Visual Studio Extension file that can install more templates, in order to guarantee a larger range of targetable architectures. These templates have been deemed the most convenient way to start developing a Direct X 11 application, in order to avoid focusing too much on the initial setup and concentrate more on the features required for this project. The specific template chosen for this coursework is "Direct3D Win32 Game", a sample that features only an empty game window, but does have all the functions needed to run the basic game loop. To complete the setup, the only thing needed was to add DirectXTK through the NuGet packet manager, and update the #include calls on the pch.h file.

Understanding the Game Loop

Looking through the DirectX code for the first time can be a bit overwhelming, and the implementation workflow may be hard to grasp, at least at the beginning. After going through the tutorials on the DirectXTK wiki, though, it is possible to underline an implementation pattern that is shared between functionalities.

--** Step 1 – CreateDevice/Initialize**: To summarize what has been understood, DirectX relies on a structure based on device resources, which are usually declared in the game's header file. These are later on initialized in the CreateDevice function, at least in this project's implementation. To avoid boilerplate code, it is usually considered best practice to handle device resources through a separate class called DeviceResources. The reason why this project is structured differently is mostly in order to get a better understanding of the DirectX workflow. Device independent resources such as command inputs or audio effects are instantiated in the game's Initialize function.
--** Step 2 – Update**: World, view and projection changes, as well as anything involving game mechanics such as audio or input processing for camera controls, is calculated here within this function. In order to make "world" updates relatively order-independent, this project features multiple world matrices, each for every object in the game. Although it has now come to understanding that this choice was quite useless (if not taxing memory-wise, especially with a higher number of device resources requiring a world matrix), this odd implementation choice has been kept just to display the results. It is noted though that using a single world matrix can be enough.
--** Step 3 – Render**: Once the game world has been updated, the render function is usually called. This section allows users to visualize what's going on inside the game, although it should be noted that the game should run as well even without rendering. Within this function we call the shader-related operations, like lighting effects. Another operation that is done here is the camera view update.
--** Step 4 **– OnDeviceLost : The last thing that is done during the implementation phase of a new device is the reset() call on the device when it is lost, in the OnDeviceLost function. This last bit of code is meant for resource clean up.
Except for the Sound Effects and Camera implementation, the workflow above has usually been the standard process for implementing functionalities inside the application. These will be discussed in more detail in the next section.

Implementing Functionalities

A more detailed look into the implementation process of the functionalities required for this project, and the insight that has been gathered upon this subject.

Custom Vertex Geometry

The first implementation of this functionality has initially been done by using a different function than the one that has been used in the final version. Instead of using two arrays to store indexes and vertices, initially there were four distinct sets of triangle vertices, which were drawn by using the DrawTriangle function, called four times. Later on though, after some code comparison with what has been done during the third tutorial session of this course (focused on rendering custom geometry), the rendering function has been changed. The updated version uses the DrawIndexed function, featuring also a custom pixel shader and a vector shader.

In order to make the triangles move, each vertex is divided by a small float calculated by using the cosine of the elapsed seconds. This allows a cyclical animation, the effect of which can be clearly seen when running the application.

An interesting information that has been acquired after the implementation of other device resources in the scene concerns the importance of the rendering order, as due to a small oversight one 3D object appeared on top of the triangles.

Loading and Rendering Meshes

When running the application, as can be seen towards the furthermost side of the room there's a rotating skull with a greenish palette. It is fundamentally a .sdkmesh object, obtained by converting a .obj model and a .mtl file using a special tool called meshconverter, available on the DirectXTK Wiki page. The device resource is initialized as a Model class object, binding together the .sdkmesh file and an IEffect class object. The latter is used to add lighting and fog effects during the render phase. That's actually what's making the skull look green: the more distant the camera is from the skull, the greener it looks, while getting closer changes the fog's thickness, and the lighting's direction.

World, View and Projection Transformations

As it has been mentioned earlier, the current project's implementation features multiple world matrices for each of the device resources associated with 3D rendering. While it has been argued that this could've been avoided without affecting the application, there are certain downsides that could've come out for example if certain camera mechanics were to be implemented. Before discussing that, it is worth mentioning the existence of two distinct views as well: while most of the object share the same view, based on the camera position, there's one view that is unaffected: the HUD view in fact is separate, thus allowing to display a separately running animation that is constantly in front of the player's view.

Lighting

Lighting effects in this application have been achieved by using the Effects classes from the DirectX API, although multiple separate effects instances have been created to achieve different results occurring at the same time. This is definitely not ordinary, as one would usually want the lighting to be consistent within the entire environment; the reason why there are many Effects resources is based upon the difference occurring from one effect to the other, as they're based on three different sub-classes, and allow to simulate distinct visual effects. Standard Blinn-Phong lighting, applied on the rotating globe, is achieved through the BasicEffect class, while more particular effects, such as the fog (applied on the skull), are created through the IEffectFactory class. The last effect, which is a bit more advanced, is achieved by using the EnvironmentMapEffect, and it is fundamentally based upon the Cook-Torrance lighting model. Code-wise speaking, the effect relies on one extra texture loaded into the model, an Environment Map. Once this is loaded, the environment's reflection strength is changed during the Update method through a specific setter method that belongs to this special Effect class, SetFresnelFactor. The Fresnel factor, a fundamental component of the Cook-Torrance lighting model, basically describes how much light is reflected from a surface. As this factor increases, the Environment Map texture is displayed on top of the original texture of the rendered object, thus applying a metallic reflection on the object. In the application, this lighting effect is applied on the teapot.

Textures and Materials

There are four examples of texture loading within this project. The first one is the room, based on a .DDS file, loaded inside the CreateDevice initialization phase. Other textures that are similarly loaded are the porcelain texture used for the teapot (another DDS file), the Environment Map used for the Cook-Torrance lighting effect, and the Earth's texture, which is different from the others as it is a .BMP file. The only difference caused by this alternative format is the function that binds the texture device resource to the file, which, instead of calling CreateDDSTextureFromFile calls CreateWICTextureFromFile. Similarly to all the other device resources, the texture have to be reset inside the OnDeviceLost function.

Camera featuring Keyboard Controls

The Camera integration process has revealed itself to be slightly more tedious than the other resources' implementation, as the input binding requires to add code that is not only inside the Game class (or the Device Resource class), but inside the Main.cpp file as well. To be more precise, what had to be changed and added in order to handle the input received from either mouse or keyboard were a short list of switch cases inside the WndProc() function.

Once this part had been achieved, the initialization of the input handler had to be called inside the Initialize function instead of the CreateDevice function. Last but not least, input binding was done inside the Update function, through which the camera position was constantly updated according to the detected keypresses.

The current camera created for the scope of this project allows to move according to pitch and yaw, i.e. it allows to look almost in every direction, except straight up at 0° or down at -180°, due to the addition of a gimbal lock in order to avoid the camera flipping upside down.

It was noted that one thing the camera doesn't allow is to roll, a game mechanic that is seen rarely and almost exclusively in flight simulators. What was found out though, after trying to bind new inputs in order to achieve so, was that changing the roll factor didn't make the camera rotate, it only flipped the coordinate system. It was then realized how the rolling could've been achieved, e.g. by applying a transform on the game world based upon the rolling direction. Although this "fake" camera movement would've been a fascinating add-on, it hasn't been implemented due to the existence of multiple world resources inside the game, and most importantly the trouble that came into updating the camera movement boundaries, as the current implementation doesn't allow the camera to go beyond the room's walls.

Audio

The last functionality that has been implemented for this project is Sound Effects, which, similarly to the Camera integration, required to change classes beyond the Game class or the Device Resource class. Audio integration has required updating the project's include libraries, as well as the Main.cpp file. The function that had to be updated this time was wWinMain, where a few lines were added in order to register an audio device. A new #include "Audio.h" had to be added as well to the pch.h file, and lastly, the device resources had to be instantiated inside the Game class' Initialize function.

The entire functionality is based upon six items: an AudioEngine class, used to bind the sound to the SoundEffect class, a SoundEffectInstance to which the SoundEffect object is bound to and upon which the Play function is called, a boolean to restart the audio file when it is done playing (to loop the sound), a float to represent the sound's volume and a small offset by which this volume is increased.

Within the game's Update function, the volume is increased according to the camera's distance from the skull. This has been done in order to simulate positional audio, as the user won't hear any sound effect until it gets closer to the skull.

References

Walbourn, C. (2019). microsoft/DirectXTK. [online] GitHub. Available at: https://github.com/microsoft/DirectXTK/wiki/Getting-Started [Accessed 9 Dec. 2019].
