//--------------------------------------------------------------
//  Version: 1.0
//  MadeBy: WinCat
//  Site: tom-crowley.co.uk
//
//  Info: Defines the techniques for GUI shader.
//  Blend options: http://msdn.microsoft.com/en-gb/library/windows/desktop/bb204892(v=vs.85).aspx
//
//------------------------------------
shader
{
	source "FFE.hlsl"
	techniques
	{
		{
			name 							"Solid"
			vertex_shader   	"BasicVS"
			pixel_shader    	"SolidPS"	
			src_blend_color  	src_alpha
			src_blend_alpha  	src_alpha
			dst_blend_color  	inv_src_alpha
			dst_blend_alpha  	inv_src_alpha
			cull_mode           back
			depth_test         always
			depth_write        false
		}		
		{
			name 							"Texture"
			vertex_shader   	"BasicVS"
			pixel_shader    	"TexturePS"		
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