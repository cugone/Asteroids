<shader name="ufo">
    <shaderprogram src="Data/ShaderPrograms/ufo_VS.cso"/>
    <shaderprogram src="Data/ShaderPrograms/ufo_PS.cso"/>
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
