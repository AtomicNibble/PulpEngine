


shader
{
	source "Deffered.hlsl"
	techniques
	{
                {
                    name                	"WriteDeferred"
                    vertex_shader       	"WriteDeferredVS"
                    pixel_shader         	"DeferredShadingPS"
                    cull_mode            	front
                }
	}
}