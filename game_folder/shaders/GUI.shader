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
	source "GUI.hlsl"
	techniques
	{
		{
			name 							"Fill(Textured)"
			vertex_shader   	"guiVS"
			pixel_shader    	"guiPS"	
			src_blend_color  	src_alpha
			src_blend_alpha  	src_alpha
			dst_blend_color  	inv_src_alpha
			dst_blend_alpha  	inv_src_alpha
			cull_mode           back
			depth_test         always
			depth_write        false
		}
		{
			name 							"FillAdd(Textured)"
			vertex_shader   	"guiVS"
			pixel_shader    	"AlphaTestPS"		
			src_blend_color  	src_alpha
			src_blend_alpha 	src_alpha
			dst_blend_color  	one
			dst_blend_alpha 	one
			cull_mode           back
			depth_test         always
			depth_write        false
		}
		{
			name 							"FillMul(Textured)"
			vertex_shader   	"guiVS"
			pixel_shader    	"guiPS"		
			src_blend_color  	zero
			src_blend_alpha 	zero
			dst_blend_color  	src_color
			dst_blend_alpha 	src_color
			cull_mode           back
			depth_test         always
			depth_write        false
		}		
	}
}
