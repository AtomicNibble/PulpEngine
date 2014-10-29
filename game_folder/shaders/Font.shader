
shader
{
	source "Font.hlsl"
	techniques
	{
		{
			name 		"Font"
			vertex_shader   	"BasicVS"
			pixel_shader    	"FontPS"		
			src_blend_color  	src_alpha
			src_blend_alpha  	src_alpha
			dst_blend_color  	inv_src_alpha
			dst_blend_alpha  	inv_src_alpha
			cull_mode           back
			depth_test         always
			depth_write        false
		}
	}
}