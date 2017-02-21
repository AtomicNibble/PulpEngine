


shader
{
	source "Defferedvis.hlsl"
	techniques
	{
		{
		    name                 	"VisualizeAlbedo"
		    vertex_shader        	"DeferredPassVS"
		    pixel_shader         	"VisualizeAlbedoPS"
		    depth_test           	always
		    depth_write          	false
		}
		{
		    name                 	"VisualizeNormals"
		    vertex_shader        	"DeferredPassVS"
		    pixel_shader         	"VisualizeNormalsPS"
		    depth_test           	always
		    depth_write          	false
		    wireframe		false
		}             
		{
		    name                 	"VisualizeDepth"
		    vertex_shader        	"DeferredPassVS"
		    pixel_shader         	"VisualizeDepthPS"
		    depth_test           	always
		    depth_write          	false
		} 
	}
}
