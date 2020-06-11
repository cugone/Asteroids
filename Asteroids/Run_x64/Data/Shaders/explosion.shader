<shader name="explosion">
    <shaderprogram src="__unlit"/>
    <!--
    <shaderprogram src="Data/ShaderPrograms/explosion.hlsl">
        <pipelinestages>
            <vertex entrypoint="VertexFunction" />
            <pixel entrypoint="PixelFunction" />
            <geometry entrypoint="GeometryFunction" />
        </pipelinestages>
    </shaderprogram>
    -->
    <!--
    <shaderprogram src="Data/ShaderPrograms/explosion_VS.cso"/>
    <shaderprogram src="Data/ShaderPrograms/explosion_PS.cso"/>
    <shaderprogram src="Data/ShaderPrograms/explosion_GS.cso"/>
    -->
    <raster>
        <fill>solid</fill>
        <cull>none</cull>
        <antialiasing>false</antialiasing>
    </raster>
    <blends>
        <blend enable = "true">
            <color src = "src_alpha" dest = "inv_src_alpha" op = "add" />
        </blend>
    </blends>
    <depth enable = "false" writable = "false" />
    <stencil enable = "false" readable = "false" writable = "false" />
</shader>
