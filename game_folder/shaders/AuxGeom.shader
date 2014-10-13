shader
{
	source "AuxGeom.hlsl"
	techniques
	{
		{
			name 		"AuxGeometry"
			vertex_shader   	"AuxGeomVS"
			pixel_shader    	"AuxGeomPS"	
			src_blend_color  	src_alpha
			src_blend_alpha  	src_alpha
			dst_blend_color  	inv_src_alpha
			dst_blend_alpha  	inv_src_alpha
			cull_mode           none
			depth_test         always
			depth_write        false
		}
		{
			name 		"AuxGeometryObj"
			vertex_shader   	"AuxGeomObjVS"
			pixel_shader    	"AuxGeomPS"	
			src_blend_color  	src_alpha
			src_blend_alpha  	src_alpha
			dst_blend_color  	inv_src_alpha
			dst_blend_alpha  	inv_src_alpha
			cull_mode           none
			depth_test         always
			depth_write        false
			wireframe		false
		}			
	}
}