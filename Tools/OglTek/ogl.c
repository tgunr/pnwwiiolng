// ogl.c

#include "syscfg.h"
#include "vars.h"
#include "vars.h"
#include "ogl.h"
#include "ddp.h"

typedef struct tagEXTENSION
{
	char	name[48];
	BOOL	*pflag;

} OGL_EXTENSION, *LPOGL_EXTENSION;

#define OPENGL_MAJOR_VERSION	1
#define OPENGL_MINOR_VERSION	1

#define NO_LOCK_TO_REFRESH		0
#define LOCK_TO_REFRESH			1

#define oglSRCBLENDCAPS_ALL					0x000001ff
#define oglSRCBLENDCAPS_IMAGING_ALL			0x00001e00
#define oglDSTBLENDCAPS_ALL					0x000000ff
#define oglDSTBLENDCAPS_IMAGING_ALL			0x00000f00

static OGL_EXTENSION	ogl_extension[]=
		{
			{"GL_ARB_imaging", &gARB_imaging},
			{"GL_ARB_multitexture", &gARB_multitexture},
			{"GL_ARB_texture_mirrored_repeat", &gARB_texture_mirrored_repeat},
			{"GL_ARB_transpose_matrix", &gARB_transpose_matrix},
			{"GL_ARB_multisample", &gARB_multisample},
			{"GL_ARB_texture_env_add", &gARB_texture_env_add},
			{"GL_ARB_texture_cube_map", &gARB_texture_cube_map},
			{"GL_ARB_texture_compression", &gARB_texture_compression},
			{"GL_ARB_texture_border_clamp", &gARB_texture_border_clamp},
			{"GL_ARB_point_parameters", &gARB_point_parameters},
			{"GL_ARB_matrix_palette", &gARB_matrix_palette},
			{"GL_ARB_texture_env_combine", &gARB_texture_env_combine},
			{"GL_ARB_texture_env_crossbar", &gARB_texture_env_crossbar},
			{"GL_ARB_texture_env_dot3", &gARB_texture_env_DOT3},
			{"GL_EXT_vertex_array", &gEXT_vertex_array},
			{"GL_EXT_polygon_offset", &gEXT_polygon_offset},
			{"GL_EXT_blend_logic_op", &gEXT_blend_logic_op},
			{"GL_EXT_texture", &gEXT_texture},
			{"GL_EXT_copy_texture", &gEXT_copy_texture},
			{"GL_EXT_subtexture", &gEXT_subtexture},
			{"GL_EXT_texture_object", &gEXT_texture_object},
			{"GL_EXT_texture3D", &gEXT_texture3D},
			{"GL_EXT_abgr", &gEXT_abgr},
			{"GL_EXT_bgra", &gEXT_bgra},
			{"GL_EXT_texture_env_combine", &gEXT_texture_env_combine},
			{"GL_EXT_texture_env_add", &gEXT_texture_env_add},
			{"GL_EXT_packed_pixels", &gEXT_packed_pixels},
			{"GL_EXT_rescale_normal", &gEXT_rescale_normal},
			{"GL_EXT_separate_specular_color", &gEXT_separate_specular_color},
			{"GL_SGIS_texture_edge_clamp", &gSGIS_texture_edge_clamp},
			{"GL_SGIS_texture_lod", &gSGIS_texture_lod},
			{"GL_EXT_clip_volume_hint", &gEXT_clip_volume_hint},
			{"GL_EXT_draw_range_elements", &gEXT_draw_range_elements},
			{"GL_EXT_color_table", &gEXT_color_table},
			{"GL_EXT_color_subtable", &gEXT_color_subtable},
			{"GL_EXT_convolution", &gEXT_convolution},
			{"GL_HP_convolution_border_modes", &gHP_convolution_border_modes},
			{"GL_SGI_color_matrix", &gSGI_color_matrix},
			{"GL_EXT_histogram", &gEXT_histogram},
			{"GL_EXT_blend_color", &gEXT_blend_color},
			{"GL_EXT_blend_minmax", &gEXT_blend_minmax},
			{"GL_EXT_blend_subtract", &gEXT_blend_subtract},
			{"GL_EXT_compiled_vertex_array", &gEXT_compiled_vertex_array},
			{"GL_ARB_depth_texture", &gARB_depth_texture},
			{"GL_ARB_fragment_program", &gARB_fragment_program},
			{"GL_ARB_shadow", &gARB_shadow},
			{"GL_ARB_shadow_funcs", &gARB_shadow_funcs},
			{"GL_ARB_shadow_ambient", &gARB_shadow_ambient},
			{"GL_ARB_vertex_blend", &gARB_vertex_blend},
			{"GL_ARB_vertex_program", &gARB_vertex_program},
			{"GL_ARB_window_pos", &gARB_window_pos},
			{"GL_ARB_vertex_buffer_object", &gARB_vertex_buffer_object},
			{"GL_EXT_blend_func_separate", &gEXT_blend_func_separate},
			{"GL_EXT_fog_coord", &gEXT_fog_coord},
			{"GL_EXT_multi_draw_arrays", &gEXT_multi_draw_arrays},
			{"GL_EXT_point_parameters", &gEXT_point_parameters},
			{"GL_EXT_secondary_color", &gEXT_secondary_color},
			{"GL_EXT_stencil_wrap", &gEXT_stencil_wrap},
			{"GL_EXT_texgen_reflection", &gEXT_texgen_reflection},
			{"GL_EXT_texture_compression_s3tc", &gEXT_texture_compression_s3tc},
			{"GL_EXT_texture_cube_map", &gEXT_texture_cube_map},
			{"GL_EXT_texture_edge_clamp", &gEXT_texture_edge_clamp},
			{"GL_EXT_texture_env_dot3", &gEXT_texture_env_dot3},
			{"GL_EXT_texture_filter_anisotropic", &gEXT_texture_filter_anisotropic},
			{"GL_EXT_texture_lod", &gEXT_texture_lod},
			{"GL_EXT_texture_lod_bias", &gEXT_texture_lod_bias},
			{"GL_EXT_texture_rectangle", &gEXT_texture_rectangle},
			{"GL_EXT_vertex_shader", &gEXT_vertex_shader},
			{"GL_EXT_paletted_texture", &gEXT_paletted_texture},
			{"GL_EXT_shared_texture_palette", &gEXT_shared_texture_palette},
			{"GL_SGIS_texture_border_clamp", &gSGIS_texture_border_clamp},
			{"GL_SGIS_generate_mipmap", &gSGIS_generate_mipmap},
			{"GL_SGIS_multitexture", &gSGIS_multitexture},
			{"GL_SGIS_texture_filter4", &gSGIS_texture_filter4},
			{"GL_SGI_color_table", &gSGI_color_table},
			{"GL_SGIS_pixel_texture", &gSGIS_pixel_texture},
			{"GL_SGIX_pixel_texture", &gSGIX_pixel_texture},
			{"GL_SGIS_texture4D", &gSGIS_texture4D},
			{"GL_SGI_texture_color_table", &gSGI_texture_color_table},
			{"GL_EXT_cmyka", &gEXT_cmyka},
			{"GL_SGIS_detail_texture", &gSGIS_detail_texture},
			{"GL_SGIS_sharpen_texture", &gSGIS_sharpen_texture},
			{"GL_SGIS_multisample", &gSGIS_multisample},
			{"GL_EXT_misc_attribute", &gEXT_misc_attribute},
			{"GL_SGIX_clipmap", &gSGIX_clipmap},
			{"GL_SGIX_shadow", &gSGIX_shadow},
			{"GL_SGIX_interlace", &gSGIX_interlace},
			{"GL_SGIX_pixel_tiles", &gSGIX_pixel_tiles},
			{"GL_SGIS_texture_select", &gSGIS_texture_select},
			{"GL_SGIX_sprite", &gSGIX_sprite},
			{"GL_SGIX_texture_multi_buffer", &gSGIX_texture_multi_buffer},
			{"GL_SGIX_instruments", &gSGIX_instruments},
			{"GL_SGIX_texture_scale_bias", &gSGIX_texture_scale_bias},
			{"GL_SGIX_framezoom", &gSGIX_framezoom},
			{"GL_SGIX_tag_sample_buffer", &gSGIX_tag_sample_buffer},
			{"GL_FfdMaskSGIX", &gFfdMaskSGIX},
			{"GL_SGIX_polynomial_ffd", &gSGIX_polynomial_ffd},
			{"GL_SGIX_reference_plane", &gSGIX_reference_plane},
			{"GL_SGIX_flush_raster", &gSGIX_flush_raster},
			{"GL_SGIX_depth_texture", &gSGIX_depth_texture},
			{"GL_SGIS_fog_function", &gSGIS_fog_function},
			{"GL_SGIX_fog_offset", &gSGIX_fog_offset},
			{"GL_HP_image_transform", &gHP_image_transform},
			{"GL_INGR_palette_buffer", &gINGR_palette_buffer},
			{"GL_SGIX_texture_add_env", &gSGIX_texture_add_env},
			{"GL_PGI_vertex_hints", &gPGI_vertex_hints},
			{"GL_PGI_misc_hints", &gPGI_misc_hints},
			{"GL_SGIX_list_priority", &gSGIX_list_priority},
			{"GL_SGIX_ir_instrument1", &gSGIX_ir_instrument1},
			{"GL_SGIX_calligraphic_fragment", &gSGIX_calligraphic_fragment},
			{"GL_SGIX_texture_lod_bias", &gSGIX_texture_lod_bias},
			{"GL_SGIX_shadow_ambient", &gSGIX_shadow_ambient},
			{"GL_EXT_index_texture", &gEXT_index_texture},
			{"GL_EXT_index_material", &gEXT_index_material},
			{"GL_EXT_index_func", &gEXT_index_func},
			{"GL_EXT_index_array_formats", &gEXT_index_array_formats},
			{"GL_EXT_cull_vertex", &gEXT_cull_vertex},
			{"GL_SGIX_ycrcb", &gSGIX_ycrcb},
			{"GL_SGIX_fragment_lighting", &gSGIX_fragment_lighting},
			{"GL_IBM_rasterpos_clip", &gIBM_rasterpos_clip},
			{"GL_HP_texture_lighting", &gHP_texture_lighting},
			{"GL_WIN_phong_shading", &gWIN_phong_shading},
			{"GL_WIN_specular_fog", &gWIN_specular_fog},
			{"GL_EXT_light_texture", &gEXT_light_texture},
			{"GL_SGIX_blend_alpha_minmax", &gSGIX_blend_alpha_minmax},
			{"GL_SGIX_async", &gSGIX_async},
			{"GL_SGIX_async_pixel", &gSGIX_async_pixel},
			{"GL_SGIX_async_histogram", &gSGIX_async_histogram},
			{"GL_INTEL_texture_scissor", &gINTEL_texture_scissor},
			{"GL_INTEL_parallel_arrays", &gINTEL_parallel_arrays},
			{"GL_HP_occlusion_test", &gHP_occlusion_test},
			{"GL_EXT_pixel_transform", &gEXT_pixel_transform},
			{"GL_EXT_pixel_transform_color_table", &gEXT_pixel_transform_color_table},
			{"GL_EXT_texture_perturb_normal", &gEXT_texture_perturb_normal},
			{"GL_REND_screen_coordinates", &gREND_screen_coordinates},
			{"GL_EXT_coordinate_frame", &gEXT_coordinate_frame},
			{"GL_APPLE_specular_vector", &gAPPLE_specular_vector},
			{"GL_APPLE_transform_hint", &gAPPLE_transform_hint},
			{"GL_SGIX_fog_scale", &gSGIX_fog_scale},
			{"GL_SUNX_constant_data", &gSUNX_constant_data},
			{"GL_SUN_global_alpha", &gSUN_global_alpha},
			{"GL_SUN_triangle_list", &gSUN_triangle_list},
			{"GL_SUN_vertex", &gSUN_vertex},
			{"GL_INGR_color_clamp", &gINGR_color_clamp},
			{"GL_INGR_interlace_read", &gINGR_interlace_read},
			{"GL_EXT_422_pixels", &gEXT_422_pixels},
			{"GL_NV_texgen_reflection", &gNV_texgen_reflection},
			{"GL_SUN_convolution_border_modes", &gSUN_convolution_border_modes},
			{"GL_EXT_vertex_weighting", &gEXT_vertex_weighting},
			{"GL_NV_light_max_exponent", &gNV_light_max_exponent},
			{"GL_NV_vertex_array_range", &gNV_vertex_array_range},
			{"GL_NV_register_combiners", &gNV_register_combiners},
			{"GL_NV_fog_distance", &gNV_fog_distance},
			{"GL_NV_texgen_emboss", &gNV_texgen_emboss},
			{"GL_NV_blend_square", &gNV_blend_square},
			{"GL_NV_texture_env_combine4", &gNV_texture_env_combine4},
			{"GL_MESA_resize_buffers", &gMESA_resize_buffers},
			{"GL_MESA_window_pos", &gMESA_window_pos},
			{"GL_IBM_cull_vertex", &gIBM_cull_vertex},
			{"GL_IBM_multimode_draw_arrays", &gIBM_multimode_draw_arrays},
			{"GL_IBM_vertex_array_lists", &gIBM_vertex_array_lists},
			{"GL_SGIX_subsample", &gSGIX_subsample},
			{"GL_SGIX_ycrcb_subsample", &gSGIX_ycrcb_subsample},
			{"GL_SGIX_ycrcba", &gSGIX_ycrcba},
			{"GL_SGI_depth_pass_instrument", &gSGI_depth_pass_instrument},
			{"GL_3DFX_texture_compression_FXT1", &g3DFX_texture_compression_FXT1},
			{"GL_3DFX_multisample", &g3DFX_multisample},
			{"GL_3DFX_tbuffer", &g3DFX_tbuffer},
			{"GL_EXT_multisample", &gEXT_multisample},
			{"GL_SGIX_vertex_preclip", &gSGIX_vertex_preclip},
			{"GL_SGIX_convolution_accuracy", &gSGIX_convolution_accuracy},
			{"GL_SGIX_resample", &gSGIX_resample},
			{"GL_SGIS_point_line_texgen", &gSGIS_point_line_texgen},
			{"GL_SGIS_texture_color_mask", &gSGIS_texture_color_mask},
			{"GL_SGIX_igloo_interface", &gSGIX_igloo_interface},
			{"GL_NV_fence", &gNV_fence},
			{"GL_NV_evaluators", &gNV_evaluators},
			{"GL_NV_packed_depth_stencil", &gNV_packed_depth_stencil},
			{"GL_NV_register_combiners2", &gNV_register_combiners2},
			{"GL_NV_texture_compression_vtc", &gNV_texture_compression_vtc},
			{"GL_NV_texture_rectangle", &gNV_texture_rectangle},
			{"GL_NV_texture_shader", &gNV_texture_shader},
			{"GL_NV_texture_shader2", &gNV_texture_shader2},
			{"GL_NV_vertex_array_range2", &gNV_vertex_array_range2},
			{"GL_NV_vertex_program", &gNV_vertex_program},
			{"GL_SGIX_texture_coordinate_clamp", &gSGIX_texture_coordinate_clamp},
			{"GL_SGIX_scalebias_hint", &gSGIX_scalebias_hint},
			{"GL_WIN_swap_hint", &gWIN_swap_hint},
			{"GL_ATI_envmap_bumpmap", &gATI_envmap_bumpmap},
			{"GL_ATI_fragment_shader", &gATI_fragment_shader},
			{"GL_ATI_pn_triangles", &gATI_pn_triangles},
			{"GL_ATI_separate_stencil", &gATI_separate_stencil},
			{"GL_ATI_texture_mirror_once", &gATI_texture_mirror_once},
			{"GL_ATI_vertex_streams", &gATI_vertex_streams},
			{"GL_ATI_vertex_array_object", &gATI_vertex_array_object},
			{"GL_ATI_element_array", &gATI_element_array},
			{"GL_ATI_map_object_buffer", &gATI_map_object_buffer},
			{"GL_ATI_vertex_attrib_array_object", &gATI_vertex_attrib_array_object},
			{"GL_ATI_texture_float", &gATI_texture_float},
			{"GL_ATI_draw_buffers", &gATI_draw_buffers},
			{"GL_ATI_texture_env_combine3", &gATI_texture_env_combine3},
			{"GL_ATIX_texture_env_route", &gATIX_texture_env_route},
			{"GL_ATIX_vertex_shader_output_point_size", &gATIX_vertex_shader_output_point_size},
			{"GL_NV_copy_depth_to_color", &gNV_copy_depth_to_color},
			{"GL_NV_depth_clamp", &gNV_depth_clamp},
			{"GL_NV_occlusion_query", &gNV_occlusion_query},
			{"GL_NV_point_sprite", &gNV_point_sprite},
			{"GL_NV_texture_shader3", &gNV_texture_shader3},
			{"GL_NV_vertex_program1_1", &gNV_vertex_program1_1},
			{"GL_NV_multisample_filter_hint", &gNV_multisample_filter_hint},
			{"", NULL}	// Last in array
		};

static void DisplayLastError(void)
{
LPVOID lpMsgBuf;

	FormatMessage(	FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL);

	// Display the string
	Write_Debug(lpMsgBuf);

	// Free the buffer
	LocalFree(lpMsgBuf);
}

static BOOL HandleGLError(BOOL bMessage)
{
BOOL	bError=TRUE;
int		enumber;

	while ((enumber = glGetError()) != GL_NO_ERROR)
	{
	char	*ptr;

		if (bMessage)
		{
			switch (enumber)
			{
			case GL_INVALID_ENUM:
				ptr = "An unacceptable value is specified for an enumerated argument. The offending function is ignored, having no side effect other than to set the error flag.";
				break;
			case GL_INVALID_VALUE:
				ptr = "A numeric argument is out of range. The offending function is ignored, having no side effect other than to set the error flag.";
				break;
			case GL_INVALID_OPERATION:
				ptr = "The specified operation is not allowed in the current state. The offending function is ignored, having no side effect other than to set the error flag.";
				break;
			case GL_STACK_OVERFLOW:
				ptr = "This function would cause a stack overflow. The offending function is ignored, having no side effect other than to set the error flag.";
				break;
			case GL_STACK_UNDERFLOW:
				ptr = "This function would cause a stack underflow. The offending function is ignored, having no side effect other than to set the error flag.";
				break;
			case GL_OUT_OF_MEMORY:
				ptr = "There is not enough memory left to execute the function. The state of OpenGL is undefined, except for the state of the error flags, after this error is recorded";
				break;
			default:
				break;
			}

			Write_Debug("GL error 0x%04x encountered\r\n %s", enumber, ptr);
		}

		bError = FALSE;
	}

	return(bError);
}

// Parse the extensions list returned from OpenGL
static BOOL ParseExtensions(char *tp)
{
int		i;

	tp = (char*)glGetString(GL_EXTENSIONS);
	if (tp != NULL)
	{
	char	*ptr;

		Write_Debug("Extensions:");
		ptr = tp;
		do
		{
			ptr = strtok(tp, " ");
			if (ptr == NULL)
			{
				break;
			}

//			Write_Debug(" Checking %s", ptr);

			tp = NULL;
			for (i=0 ; ogl_extension[i].pflag != NULL ; i++)
			{
				if (strcmp(ogl_extension[i].name, ptr) == 0)
				{

					Write_Debug("  %s is available", ogl_extension[i].name);

					*ogl_extension[i].pflag = TRUE;
					break;
				}
			}
		}
		while (TRUE);
	}

	return(TRUE);
}

BOOL oglInit(void)
{
char		*tp, *ptr;
int			i;
int			numcomps, *comps;

	// Extract data from OpenGL
	tp = (char*)glGetString(GL_VENDOR);
	if (tp != NULL)
	{
		Write_Debug("Vendor:    %s", tp);
	}

	tp = (char*)glGetString(GL_VERSION);
	if (tp != NULL)
	{
		Write_Debug("Version:   %s", tp);
		gOpenGLMajorVersion = atoi(tp);
		gOpenGLMinorVersion = 0;
		tp = strchr(tp, '.');
		if (tp != NULL)
		{
			tp += 1;
			gOpenGLMinorVersion = atoi(tp);
		}

		Write_Debug("OpenGL version: %d.%d", gOpenGLMajorVersion, gOpenGLMinorVersion);

		if (!(gOpenGLMajorVersion > OPENGL_MAJOR_VERSION || (gOpenGLMajorVersion == OPENGL_MAJOR_VERSION && gOpenGLMinorVersion >= OPENGL_MINOR_VERSION)))
		{
			Write_Debug("Required OpenGL version: %d.%d or higher", OPENGL_MAJOR_VERSION, OPENGL_MINOR_VERSION);
			return(FALSE);
		}
	}
	else
	{
		Write_Debug("OpenGL version %d.%d or better is required", OPENGL_MAJOR_VERSION, OPENGL_MINOR_VERSION);
		return(FALSE);
	}

	tp = (char*)glGetString(GL_RENDERER);
	if (tp != NULL)
	{
		Write_Debug("Renderer:  %s", tp);
	}

	tp = (char*)glGetString(GL_EXTENSIONS);
	if (tp != NULL)
	{
		if (!ParseExtensions(tp))
		{
			Write_Debug("Parsing of OpenGL extensions failed");
			return(FALSE);
		}
	}

	glGetIntegerv(GL_MAX_LIGHTS, (GLint*)&gMaxLights);

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, (GLint*)&gMaxTextureWidth);
	gMaxTextureHeight = gMaxTextureWidth;

	gSupportedSrcBlendModes = oglSRCBLENDCAPS_ALL;
	if (gARB_imaging)
	{
		gSupportedSrcBlendModes |= oglSRCBLENDCAPS_IMAGING_ALL;
	}

	gSupportedDstBlendModes = oglDSTBLENDCAPS_ALL;
	if (gARB_imaging)
	{
		gSupportedDstBlendModes |= oglDSTBLENDCAPS_IMAGING_ALL;
	}

	// This causes a second, post texture lighting pass just for specular (looks better)
	glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

	gSupportDXT1 = FALSE;
	gSupportDXT1A = FALSE;
	gSupportDXT3 = FALSE;
	gSupportDXT5 = FALSE;
	if (gARB_texture_compression && gEXT_texture_compression_s3tc)
	{
		glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, (GLint*)&numcomps);
		Write_Debug("Number of compressed texture formats supported: %d", numcomps);
		if (numcomps > 0)
		{
			comps = malloc(numcomps * sizeof(int));
			glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS_ARB, (GLint*)comps);
			for (i=0 ; i < numcomps ; i++)
			{
				switch (comps[i])
				{
				case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:	// 0x83F0
					ptr = "DXT1 - RGB ";
					gSupportDXT1 = TRUE;
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:	// 0x83F1
					ptr = "DXT1 - RGBA";
					gSupportDXT1A = TRUE;
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:	// 0x83F2
					ptr = "DXT3 - RGBA";
					gSupportDXT3 = TRUE;
					break;
				case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:	// 0x83F3
					ptr = "DXT5 - RGBA";
					gSupportDXT5 = TRUE;
					break;
				default:
					ptr = "Unknown!";
					break;
				}

				Write_Debug(" Format %d: %s (0x%08x)", i, ptr, comps[i]);
			}

			free(comps);
		}
	}

	SupportDXT1 = gSupportDXT1 && gSupportDXT1A;	// DXT1 Opaque / one-bit alpha
	SupportDXT2 = FALSE;							// DXT2 Explicit alpha
	SupportDXT3 = gSupportDXT3;						// DXT3 Explicit alpha
	SupportDXT4 = FALSE;							// DXT4 Interpolated alpha - Alpha-premultiplied
	SupportDXT5 = gSupportDXT5;						// DXT5 Interpolated alpha - Alpha-premultiplied

	glGetIntegerv(GL_MAX_CLIP_PLANES, (GLint*)&gMaxUserClipPlanes);	// 6 is the minimum required by the OpenGL spec

	glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, (GLint*)&gMaxTextureUnits);

	if (!HandleGLError(TRUE))
	{
		Write_Debug(" GL error in oglInit");
	}

	Write_Debug("Completed oglInit");

	return TRUE;
}

BOOL oglBegin(HWND hwnd)
{
PIXELFORMATDESCRIPTOR	pfd;

	// Already begun
	if (myhrc != NULL)
	{
		return(TRUE);
	}

	if (myhdc == NULL)
	{
		myhdc = GetDC(hwnd);
		if (myhdc == NULL)
		{
			Write_Debug("Failed to GetDC");
			return(FALSE);
		}
	}

	// Choose a compatible pixel format
	if (!ddpChooseScreenFormat(myhdc, &pfd))
	{
		ReleaseDC(hwnd, myhdc);
		myhdc = NULL;
		return(FALSE);
	}

	// Create the rendering context
	myhrc = wglCreateContext(myhdc);
	if (myhrc == NULL)
	{
		DisplayLastError();
		Write_Debug("Could not create the rendering context");
		ReleaseDC(hwnd, myhdc);
		myhdc = NULL;
		return(FALSE);
	}

	// Make the rendering context current
	if (!wglMakeCurrent(myhdc, myhrc))
	{
		Write_Debug("Could not make the rendering context current");
		ReleaseDC(hwnd, myhdc);
		myhdc = NULL;
		return(FALSE);
	}

	gSupportSwapInterval = NO_LOCK_TO_REFRESH;	// Do not lock flip to screen refresh
	wglSwapIntervalEXT(gSupportSwapInterval);

	return(TRUE);
}

void oglEnd(void)
{
	if (myhdc && myhrc)
	{
		wglMakeCurrent(myhdc, NULL);
		wglDeleteContext(myhrc);
		myhrc = NULL;
	}

	if (myhdc)
	{
		ReleaseDC(myhwnd, myhdc);
		myhdc = NULL;
	}
}
