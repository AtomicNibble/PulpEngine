//--------------------------------------------------------------
//  Version: 1.0
//  MadeBy: WinCat
//  Site: tom-crowley.co.uk
//
//
//------------------------------------
shader
{
	source "Model.hlsl"
	techniques
	{		
		{
			name 							"Texture"
			vertex_shader   	"BasicVS"
			pixel_shader    	"TexturePS"		
			src_blend_color  	src_alpha
			src_blend_alpha  	src_alpha
			dst_blend_color  	inv_src_alpha
			dst_blend_alpha  	inv_src_alpha
			cull_mode           back
			depth_test         less
			depth_write        true
		}	
	}
}