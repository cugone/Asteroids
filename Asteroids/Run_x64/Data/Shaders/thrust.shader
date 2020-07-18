<shader name="thrust">
    <shaderprogram src="Data/ShaderPrograms/entity_VS.cso"/>
    <shaderprogram src="Data/ShaderPrograms/entity_PS.cso"/>
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
