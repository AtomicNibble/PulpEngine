


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
		    depth_test        	less
			src_blend_color  	one
			src_blend_alpha  	src_alpha
			dst_blend_color  	inv_src_alpha
			dst_blend_alpha  	inv_src_alpha
                }
	}
}