# Asteroids
Asteroids clone

## How To Play

The game supports keyboard, mouse, and XBox 360 controllers.

- Keyboard
  - W - Thrust
  - A - Rotate CW
  - D - Rotate CCW
  - Space - Fire

- Mouse
  - Left mouse - Fire
  - Right mouse - Thrust
  - The ship will automatically orient to face the mouse cursor.

- XBox 360 controller
  - A - Thrust
  - Right Trigger - Fire
  - Left Thumb Stick - Rotate

## Building from source

Asteroids is a game built using the [Abrams2019](https://github.com/cugone/Abrams2019.git) game engine. In order to build an executable from source you must clone Abrams2019 into a folder at the same level:

    E:/Git/Abrams2019/...
    E:/Git/Asteroids/...

## Offline compiled Shaders

This project uses offline compiled shaders via Microsoft's `fxc.exe` [Effect-Compiler Tool](https://docs.microsoft.com/en-us/windows/win32/direct3dtools/fxc) for efficiency. The human-readable shader programs located in `/Data/ShaderPrograms/` are for reference only and are not actually used at run-time. The shader metadata located at `/Data/Shaders/` can be edited as follows to use the `.hlsl` files in-place but this will cause a noticable increase in initial load time because the shader has to be compiled at program start:

```xml
<shader name="example">
    <shaderprogram src="Data/ShaderPrograms/entity.hlsl">
        <pipelinestages>
            <vertex entrypoint="VertexFunction" />
            <pixel entrypoint="PixelFunction" />
        </pipelinestages>
    </shaderprogram>
    <!-- Rest of meta file -->
</shader>
```
