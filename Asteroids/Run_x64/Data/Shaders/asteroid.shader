<shader name="asteroid">
    <!--
    <shaderprogram src="__unlit"/>
    <shaderprogram src="Data/ShaderPrograms/asteroid.hlsl">
        <pipelinestages>
            <vertex entrypoint="VertexFunction" />
            <pixel entrypoint="PixelFunction" />
        </pipelinestages>
    </shaderprogram>
    -->
    <shaderprogram src="Data/ShaderPrograms/asteroid_VS.cso"/>
    <shaderprogram src="Data/ShaderPrograms/asteroid_PS.cso"/>
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
