


shader
{
	source "Deffered.hlsl"
	techniques
	{
                {
                    name                	"WriteDeferred"
                    vertex_shader       	"WriteDeferredVS"
                    pixel_shader         	"DeferredShadingPS"
                    cull_mode            	back
		    depth_write          	true
                }
	}
}