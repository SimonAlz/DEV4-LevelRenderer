// minimalistic code to draw a single triangle, this is not part of the API.
// TODO: Part 1b
#include "FSLogo.h"
#include "h2bParser.h"
#include "Blacksmith.h"
#include <vector>
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#ifdef _WIN32 // must use MT platform DLL libraries on windows
	#pragma comment(lib, "shaderc_combined.lib") 
#endif
// Simple Vertex Shader
const char* vertexShaderSource = R"(

#pragma pack_matrix(row_major)
// TODO: 2i
[[vk::push_constant]] 
cbuffer MESH_INDEX
{
    uint meshid; 
};

// an ultra simple hlsl vertex shader
// TODO: Part 2b
struct OBJ_ATTRIBUTES
{
    float3    Kd; // diffuse reflectivity
    float        d; // dissolve (transparency) 
    float3    Ks; // specular reflectivityA
    float       Ns; // specular exponent
    float3    Ka; // ambient reflectivity
    float       sharpness; // local reflection map sharpness
    float3    Tf; // transmission filter
    float       Ni; // optical density (index of refraction)
    float3    Ke; // emissive reflectivity
    uint    illum; // illumination model
};


struct SHADER_MODEL_DATA
{
    float4 sunDirection;
    float4 SunColor;//light info
	float4 camWorldpos;
    matrix viewMatrix;
    matrix ProjectrionMatrix;//view info

    matrix matrices[1024];//world space
    OBJ_ATTRIBUTES materials[1024];//color/texture
};

StructuredBuffer<SHADER_MODEL_DATA> SceneData;

struct OBJ_VERT_IN
{
    float3 xyzw : POSITION;
    float3 uv : TEXCOORD;
    float3 nxnynz : NORMAL;

};

struct OBJ_VERT_OUT
{
    float4 xyzw : SV_POSITION;
    float3 uv : TEXCOORD;
    float3 nxnynz : NORMAL;
    float3 POSW : WORLD;
};

// TODO: Part 4b
OBJ_VERT_OUT main(OBJ_VERT_IN inputVertex) : SV_POSITION
{
    OBJ_VERT_OUT temp;
    temp.xyzw = float4(inputVertex.xyzw, 1);
    temp.xyzw = mul(temp.xyzw, SceneData[0].matrices[meshid]);
	temp.POSW = temp.xyzw;
    temp.xyzw = mul(temp.xyzw, SceneData[0].viewMatrix);
    temp.xyzw = mul(temp.xyzw, SceneData[0].ProjectrionMatrix);
    temp.nxnynz = mul(float4(inputVertex.nxnynz, 0), SceneData[0].matrices[meshid]);
    return temp;
}
)";


// Simple Pixel Shader
const char* pixelShaderSource = R"(
// TODO: Part 2b
[[vk::push_constant]] 
cbuffer MESH_INDEX
{
    uint meshid; 
};
struct OBJ_ATTRIBUTES
{
    float3    Kd; // diffuse reflectivity
    float        d; // dissolve (transparency) 
    float3    Ks; // specular reflectivity
    float       Ns; // specular exponent
    float3    Ka; // ambient reflectivity
    float       sharpness; // local reflection map sharpness
    float3    Tf; // transmission filter
    float       Ni; // optical density (index of refraction)
    float3    Ke; // emissive reflectivity
    uint    illum; // illumination model
};


struct SHADER_MODEL_DATA
{
    float4 sunDirection;
    float4 SunColor;//light info
	float4 camWorldpos;
    matrix viewMatrix;
    matrix ProjectrionMatrix;//view info

    matrix matrices[1024];//world sapace
    OBJ_ATTRIBUTES materials[1024]; //color/texture
};

struct OBJ_VERT_OUT
{ 
    float4 xyzw : SV_POSITION;
    float3 uv : TEXCOORD;
    float3 nxnynz :  NORMAL;
    float3 POSW : WORLD;
}; 


// TODO: Part 4g
// TODO: Part 2i
StructuredBuffer<SHADER_MODEL_DATA> SceneData;

// TODO: Part 3e
// an ultra simple hlsl pixel shader
// TODO: Part 4b
float4 main(OBJ_VERT_OUT inputVertex) : SV_TARGET 
{

	float4 Normal = normalize(float4(inputVertex.nxnynz, 0));
    float Fangle = saturate(dot(Normal.xyz, -SceneData[0].sunDirection.xyz));
    float4 DirectLight = SceneData[0].SunColor * Fangle;
    float4 Ambient = float4(0.25, 0.25, 0.35, 1.0);
    float4 IndirectLight =  Ambient;

    //return saturate(DirectLight + IndirectLight) * (float4(SceneData[0].materials[meshid].Kd, 1) + 0 + float4(SceneData[0].materials[meshid].Ke, 1));

    // TODO: Part 4c
    // TODO: Part 4g (half-vector or reflect method your choice)
	float3 ViewDir = normalize(SceneData[0].camWorldpos.xyz - inputVertex.POSW);
    float3 Halfvector = normalize((-SceneData[0].sunDirection.xyz) + ViewDir);
    float Intensity = pow(saturate(dot(Normal, Halfvector)), SceneData[0].materials[meshid].Ns);
    float4 ReflectedLight = SceneData[0].SunColor * float4(SceneData[0].materials[meshid].Ks, 1) * Intensity;

    return saturate(DirectLight + IndirectLight) * (float4(SceneData[0].materials[meshid].Kd, 1) + ReflectedLight + float4(SceneData[0].materials[meshid].Ke, 1));
}
)";
using namespace std;
using namespace GW;
struct logoVertex
{
	float x, y, z;
	float UVx, UVy, UVz;
	float x_norm, y_norm, z_norm;
};
// Creation, Rendering & Cleanup
class Renderer
{
	// TODO: Part 2b
	
	// proxy handles
	SYSTEM::GWindow win;
	GRAPHICS::GVulkanSurface vlk;
	CORE::GEventReceiver shutdown;

	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	// TODO: Part 1g
	VkBuffer vertexHandle2 = nullptr;
	VkDeviceMemory vertexData2 = nullptr;
	// TODO: Part 2c
	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;
	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	// TODO: Part 2e
	// TODO: Part 2f
	// TODO: Part 2g
	vector<VkBuffer> handleStorage;
	vector<VkDeviceMemory> dataStorage;

	//Camera control
	GW::INPUT::GInput GInput;
	GW::INPUT::GController GController;
	MATH::GMATRIXF cameraMtrxOut;
	//MATH::GMATRIXF matrixOfView2;

	VkDescriptorSetLayout layoutOfDescriptor = nullptr;
	VkDescriptorPool poolDescriptor = nullptr;
	vector<VkDescriptorSet> setDescriptor;
		// TODO: Part 4f
	// TODO: Part 2a
	MATH::GMATRIXF worldMatrix;
	MATH::GMatrix proxy;
	MATH::GMATRIXF viewMatrix;
	MATH::GMATRIXF perspectiveLeftMtrx;
	MATH::GMATRIXF viewAndPerspective;
	MATH::GVECTORF lightDir;
	MATH::GVECTORF lightColor;
	MATH::GVector proxy2;
	MATH::GMATRIXF worldCamera;
	// TODO: Part 2b
#define MAX_SUBMESH_PER_DRAW 1024
	struct SHADER_MODEL_DATA
	{
		MATH::GVECTORF sunDir, sunColor, camWorldPos;
		MATH::GMATRIXF view_matrix, projectionMatrix;
		MATH::GMATRIXF matrices[MAX_SUBMESH_PER_DRAW];
		H2B::ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
	};
	struct MyStruct
	{
		char version[4];
		unsigned vertexCount;
		unsigned indexCount;
		unsigned materialCount;
		unsigned meshCount;
		std::vector<H2B::VERTEX> vertices;
		std::vector<unsigned> indices;
		std::vector<H2B::MATERIAL> materials;
		std::vector<H2B::BATCH> batches;
		std::vector<H2B::MESH> meshes;
	};
	//Array that stores data from the models
	// TODO: Part 4g
public:

	SHADER_MODEL_DATA modelData;
	H2B::Parser parse;

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		parse.Parse("../Blacksmith.h2b");
		win = _win;
		vlk = _vlk;
		for (size_t i = 0; i < 1 /*Array Size*/; i++)
		{
			//set array values :)
		}
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// TODO: Part 2a
		proxy.Create();
		GInput.Create(win);
		GController.Create();
		proxy.IdentityF(worldMatrix);
		proxy.IdentityF(viewMatrix);

		MATH::GVECTORF eye1 = { 0.75f, 0.25f, -1.5f, 1 };
		MATH::GVECTORF at1 = { 0.15f, 0.75f, 0, 1 };
		MATH::GVECTORF up1 = { 0, 1, 0, 1 };
		proxy.LookAtLHF(eye1, at1, up1, viewMatrix);
		proxy.InverseF(viewMatrix, worldCamera);
		float aspectRatio;
		proxy.IdentityF(perspectiveLeftMtrx);
		vlk.GetAspectRatio(aspectRatio);
		proxy.ProjectionVulkanLHF(1.134, aspectRatio, 0.1f, 100, perspectiveLeftMtrx);
		//proxy.MultiplyMatrixF(matrixOfView2, perspectiveLeftMtrx, viewAndPerspective);

		MATH::GVECTORF direction = { -1, -1, 2, 1 };

		MATH::GVECTORF color = { 0.9f, 0.9f, 1.0f, 1.0f };

		proxy2.NormalizeF(direction, direction);
		// TODO: Part 2b

		modelData.matrices[1] = worldMatrix;
		modelData.view_matrix = viewMatrix;
		modelData.projectionMatrix = perspectiveLeftMtrx;
		modelData.sunColor = color;
		modelData.sunDir = direction;
		modelData.materials[0] = parse.materials[0].attrib;
		modelData.camWorldPos = worldCamera.row4;
		//modelData.materials[0] = parse.materials[0].attrib;

		// TODO: Part 4g
		// TODO: part 3b

		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);

		// TODO: Part 1c
		// Create Vertex Buffer


		logoVertex fsLogo[12766];

		for (unsigned i = 0; i < 12766; i++) //3885
		{
			//fsLogo[i].x = FSLogo_vertices[i].pos.x;
			//fsLogo[i].y = FSLogo_vertices[i].pos.y;
			//fsLogo[i].z = FSLogo_vertices[i].pos.z;

			//fsLogo[i].UVx = FSLogo_vertices[i].uvw.x;
			//fsLogo[i].UVy = FSLogo_vertices[i].uvw.y;
			//fsLogo[i].UVz = FSLogo_vertices[i].uvw.z;

			//fsLogo[i].x_norm = FSLogo_vertices[i].nrm.x;
			//fsLogo[i].y_norm = FSLogo_vertices[i].nrm.y;
			//fsLogo[i].z_norm = FSLogo_vertices[i].nrm.z;

			/*fsLogo[i].x = Blacksmith_vertices[i].pos.x;
			fsLogo[i].y = Blacksmith_vertices[i].pos.y;
			fsLogo[i].z = Blacksmith_vertices[i].pos.z;

			fsLogo[i].UVx = Blacksmith_vertices[i].uvw.x;
			fsLogo[i].UVy = Blacksmith_vertices[i].uvw.y;
			fsLogo[i].UVz = Blacksmith_vertices[i].uvw.z;

			fsLogo[i].x_norm = Blacksmith_vertices[i].nrm.x;
			fsLogo[i].y_norm = Blacksmith_vertices[i].nrm.y;
			fsLogo[i].z_norm = Blacksmith_vertices[i].nrm.z;*/

			fsLogo[i].x = parse.vertices[i].pos.x;
			fsLogo[i].y = parse.vertices[i].pos.y;
			fsLogo[i].z = parse.vertices[i].pos.z;

			fsLogo[i].UVx = parse.vertices[i].uvw.x;
			fsLogo[i].UVy = parse.vertices[i].uvw.y;
			fsLogo[i].UVz = parse.vertices[i].uvw.z;

			fsLogo[i].x_norm = parse.vertices[i].nrm.x;
			fsLogo[i].y_norm = parse.vertices[i].nrm.y;
			fsLogo[i].z_norm = parse.vertices[i].nrm.z;
		}

		/*for (size_t i = 0; i < FSLogo_materialcount; i++)
		{
			modelData.materials[i] = FSLogo_materials[i].attrib;
		}*/
		for (size_t i = 0; i < parse.materialCount; i++)
		{
			modelData.materials[i] = parse.materials[i].attrib;
		}
		// Transfer triangle data to the vertex buffer. (staging would be prefered here)

		//GvkHelper::create_buffer(physicalDevice, device, sizeof(FSLogo_vertices),
		//	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		//	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		//GvkHelper::write_to_buffer(device, vertexData, FSLogo_vertices, sizeof(FSLogo_vertices));
		//// TODO: Part 1g
		//GvkHelper::create_buffer(physicalDevice, device, sizeof(FSLogo_indices),
		//	VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		//	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle2, &vertexData2);
		//GvkHelper::write_to_buffer(device, vertexData2, FSLogo_indices, sizeof(FSLogo_indices));
		//GvkHelper::create_buffer(physicalDevice, device, sizeof(parse.vertices),
		//	VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		//	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		//GvkHelper::write_to_buffer(device, vertexData, Blacksmith_vertices, sizeof(parse.vertices));
		//// TODO: Part 1g
		//GvkHelper::create_buffer(physicalDevice, device, sizeof(parse.indices),
		//	VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
		//	VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle2, &vertexData2);
		//GvkHelper::write_to_buffer(device, vertexData2, Blacksmith_indices, sizeof(parse.indices));
		GvkHelper::create_buffer(physicalDevice, device, sizeof(Blacksmith_vertices),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		GvkHelper::write_to_buffer(device, vertexData, Blacksmith_vertices, sizeof(Blacksmith_vertices));
		// TODO: Part 1g
		GvkHelper::create_buffer(physicalDevice, device, sizeof(Blacksmith_indices),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle2, &vertexData2);
		GvkHelper::write_to_buffer(device, vertexData2, Blacksmith_indices, sizeof(Blacksmith_indices));
		// TODO: Part 2d
		unsigned max_frames = 0;
		// to avoid per-frame resource sharing issues we give each "in-flight" frame its own buffer
		vlk.GetSwapchainImageCount(max_frames);
		handleStorage.resize(max_frames);
		dataStorage.resize(max_frames);
		for (int i = 0; i < max_frames; ++i) {

			GvkHelper::create_buffer(physicalDevice, device, sizeof(SHADER_MODEL_DATA),
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &handleStorage[i], &dataStorage[i]);
			GvkHelper::write_to_buffer(device, dataStorage[i], &modelData, sizeof(SHADER_MODEL_DATA));
		}

		/***************** SHADER INTIALIZATION ******************/
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		//shaderc_compile_options_set_invert_y(options, true); // TODO: Part 2i
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		// Create Vertex Shader
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexShaderSource, strlen(vertexShaderSource),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vertexShader);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		result = shaderc_compile_into_spv( // compile
			compiler, pixelShaderSource, strlen(pixelShaderSource),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &pixelShader);
		shaderc_result_release(result); // done
		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		/***************** PIPELINE INTIALIZATION ******************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vertexShader;
		stage_create_info[0].pName = "main";
		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = pixelShader;
		stage_create_info[1].pName = "main";
		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info.primitiveRestartEnable = false;
		// TODO: Part 1e
		// Vertex Input State
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(logoVertex);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription vertex_attribute_description[3] = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 },
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12 },
			{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, 24 }//uv, normal, etc....
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 3;
		input_vertex_info.pVertexAttributeDescriptions = vertex_attribute_description;
		// Viewport State (we still need to set this up even though we will overwrite the values)
		VkViewport viewport = {
			0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
		};
		VkRect2D scissor = { {0, 0}, {width, height} };
		VkPipelineViewportStateCreateInfo viewport_create_info = {};
		viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info.viewportCount = 1;
		viewport_create_info.pViewports = &viewport;
		viewport_create_info.scissorCount = 1;
		viewport_create_info.pScissors = &scissor;
		// Rasterizer State
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
		rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_create_info.lineWidth = 1.0f;
		rasterization_create_info.cullMode = VK_CULL_MODE_NONE;
		rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_create_info.depthClampEnable = VK_FALSE;
		rasterization_create_info.depthBiasEnable = VK_FALSE;
		rasterization_create_info.depthBiasClamp = 0.0f;
		rasterization_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_create_info.depthBiasSlopeFactor = 0.0f;
		// Multisampling State
		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
		multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info.sampleShadingEnable = VK_FALSE;
		multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info.minSampleShading = 1.0f;
		multisample_create_info.pSampleMask = VK_NULL_HANDLE;
		multisample_create_info.alphaToCoverageEnable = VK_FALSE;
		multisample_create_info.alphaToOneEnable = VK_FALSE;
		// Depth-Stencil State
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
		depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
		depth_stencil_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_create_info.minDepthBounds = 0.0f;
		depth_stencil_create_info.maxDepthBounds = 1.0f;
		depth_stencil_create_info.stencilTestEnable = VK_FALSE;
		// Color Blending Attachment & State
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
		color_blend_attachment_state.colorWriteMask = 0xF;
		color_blend_attachment_state.blendEnable = VK_FALSE;
		color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable = VK_FALSE;
		color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment_state;
		color_blend_create_info.blendConstants[0] = 0.0f;
		color_blend_create_info.blendConstants[1] = 0.0f;
		color_blend_create_info.blendConstants[2] = 0.0f;
		color_blend_create_info.blendConstants[3] = 0.0f;
		// Dynamic State 
		VkDynamicState dynamic_state[2] = {
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount = 2;
		dynamic_create_info.pDynamicStates = dynamic_state;

		// TODO: Part 2e
		VkDescriptorSetLayoutBinding Descriptor_layout_binding = {};
		Descriptor_layout_binding.binding = 0;
		Descriptor_layout_binding.descriptorCount = 1;
		Descriptor_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		Descriptor_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		Descriptor_layout_binding.pImmutableSamplers = nullptr;

		VkDescriptorSetLayoutCreateInfo Descriptor_create_info = {};
		Descriptor_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		Descriptor_create_info.flags = 0;
		Descriptor_create_info.bindingCount = 1;
		Descriptor_create_info.pBindings = &Descriptor_layout_binding;
		Descriptor_create_info.pNext = nullptr;


		VkResult r = vkCreateDescriptorSetLayout(device, &Descriptor_create_info, nullptr, &layoutOfDescriptor);
		// TODO: Part 2f
			// TODO: Part 4f
		// TODO: Part 2g
		VkDescriptorPoolCreateInfo Descriptor_pool_info = {};
		Descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		VkDescriptorPoolSize Descriptor_pool_size = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, max_frames };
		Descriptor_pool_info.poolSizeCount = 1;
		Descriptor_pool_info.pPoolSizes = &Descriptor_pool_size;
		Descriptor_pool_info.maxSets = max_frames;
		Descriptor_pool_info.flags = 0;
		Descriptor_pool_info.pNext = nullptr;
		vkCreateDescriptorPool(device, &Descriptor_pool_info, nullptr, &poolDescriptor);

		VkDescriptorSetAllocateInfo Descriptor_allocate_info = {};
		Descriptor_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		Descriptor_allocate_info.descriptorSetCount = 1;
		Descriptor_allocate_info.pSetLayouts = &layoutOfDescriptor;
		Descriptor_allocate_info.descriptorPool = poolDescriptor;
		Descriptor_allocate_info.pNext = nullptr;
		setDescriptor.resize(max_frames);
		for (int i = 0; i < max_frames; i++) {
			vkAllocateDescriptorSets(device, &Descriptor_allocate_info, &setDescriptor[i]);
		};

		VkWriteDescriptorSet write_descriptor = {};
		write_descriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_descriptor.descriptorCount = 1;
		write_descriptor.dstArrayElement = 0;
		write_descriptor.dstBinding = 0;
		write_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		for (int i = 0; i < max_frames; ++i) {
			write_descriptor.dstSet = setDescriptor[i];
			VkDescriptorBufferInfo dbinfo = {handleStorage[i], 0, VK_WHOLE_SIZE};
			write_descriptor.pBufferInfo = &dbinfo;
			vkUpdateDescriptorSets(device, 1, &write_descriptor, 0, nullptr);
		}
			// TODO: Part 4f
		// TODO: Part 2h
			// TODO: Part 4f
	
		// Descriptor pipeline layout
		//VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		//pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		//pipeline_layout_create_info.setLayoutCount = 0;
		//pipeline_layout_create_info.pSetLayouts = VK_NULL_HANDLE;
		//// TODO: Part 2e//might be copy and paste everything
		//// Descriptor pipeline layout,
		//pipeline_layout_create_info.setLayoutCount = 1;
		//pipeline_layout_create_info.pSetLayouts = &layoutOfDescriptor;
		// TODO: Part 3c
		VkPushConstantRange constRange = {};
		constRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		constRange.offset = 0;
		constRange.size = 128;
		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts = &layoutOfDescriptor;
		pipeline_layout_create_info.pushConstantRangeCount = 1; // TODO: Part 2d 
		pipeline_layout_create_info.pPushConstantRanges = &constRange; // TODO: Part 2d
		vkCreatePipelineLayout(device, &pipeline_layout_create_info,
			nullptr, &pipelineLayout);
	    // Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, 
			&pipeline_create_info, nullptr, &pipeline);

		/***************** CLEANUP / SHUTDOWN ******************/
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
		});
	}

	void Update()
	{
		for (int i = 0; i < MAX_SUBMESH_PER_DRAW; i++)
		{
			MATH::GMatrix::RotateYLocalF(modelData.matrices[i], 0.0003f, modelData.matrices[i]);
		}
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		GvkHelper::write_to_buffer(device, dataStorage[currentBuffer], &modelData, sizeof(SHADER_MODEL_DATA));
	}

	void Render()
	{
		// TODO: Part 2a
		//SHADER_VARS worldT;
		//worldT.view_Matrix = viewAndPerspective;

		modelData.matrices[0] = worldMatrix;
		// TODO: Part 4d
		// grab the current Vulkan commandBuffer
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);
		// what is the current client area dimensions?
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// setup the pipeline's dynamic settings
		VkViewport viewport = {
            0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
        };
        VkRect2D scissor = { {0, 0}, {width, height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		worldCamera = viewMatrix;
		float aspectRatio;
		proxy.IdentityF(perspectiveLeftMtrx);
		vlk.GetAspectRatio(aspectRatio);
		proxy.ProjectionVulkanLHF(1.134, aspectRatio, 0.1f, 100, perspectiveLeftMtrx);
		
		modelData.view_matrix = viewMatrix;

		GvkHelper::write_to_buffer(device, dataStorage[currentBuffer], &modelData, sizeof(SHADER_MODEL_DATA));
		// TODO: Part 2i
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &setDescriptor[currentBuffer], 0, nullptr);
		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		// TODO: Part 1h
		vkCmdBindIndexBuffer(commandBuffer, vertexHandle2, *offsets, VK_INDEX_TYPE_UINT32);
		// TODO: Part 4d
		// TODO: Part 3b
			// TODO: Part 3d
		//vkCmdDraw(commandBuffer, 3885, MAX_SUBMESH_PER_DRAW, 0, 0); // TODO: Part 1d, 1h
		/*for (unsigned int i = 0; i < FSLogo_meshcount; i++)
		{
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(unsigned), &i);
			vkCmdDrawIndexed(commandBuffer, FSLogo_meshes[i].indexCount, 1, FSLogo_meshes[i].indexOffset, 0, 0);
		}*/
		for (unsigned int i = 0; i < Blacksmith_meshcount; i++)
		{
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(unsigned), &i);
			vkCmdDrawIndexed(commandBuffer, Blacksmith_meshes[i].indexCount, 1, Blacksmith_meshes[i].indexOffset, 0, 0);
		}
	}
	void UpdateCamera()
	{

		static auto Timeplus = std::chrono::system_clock::now();
		auto Timeplus2 = std::chrono::system_clock::now();
		float delta = std::chrono::duration_cast<std::chrono::seconds>(Timeplus2 - Timeplus).count();
		//Timeplus = Timeplus2;
		// TODO: Part 4c
		proxy.InverseF(viewMatrix, worldCamera);
		//// TODO: Part 4d
		float totalY = 0;
		float totalX = 0;
		float totalZ = 0;
		float tP = 0;
		float tYAW = 0;
		const float camera_speed = 0.0003f;
		float aspectratio;
		float space_bar;
		float left_Shift;
		float rightT;
		float leftT;
		GInput.GetState(23, space_bar);
		GInput.GetState(14, left_Shift);
		GController.GetState(0, 7, rightT);
		GController.GetState(0, 6, leftT);
		totalY = space_bar - leftT + rightT - leftT;
		//// TODO: Part 4e
		float W;
		float S;
		float LSY;
		GInput.GetState(60, W);
		GInput.GetState(56, S);
		GController.GetState(0, 17, LSY);
		totalZ = W - S + LSY;

		float D;
		float A;
		float LSX;
		GInput.GetState(41, D);
		GInput.GetState(38, A);
		GController.GetState(0, 16, LSX);
		totalX = D - A + LSX;

		//// TODO: Part 4f
		float MYD;
		float MXD;
		float RSY;
		GInput.GetMouseDelta(MXD, MYD);
		GController.GetState(0, 19, RSY);
		// TODO: Part 4g
		float RSX;
		GController.GetState(0, 18, RSX);
		vlk.GetAspectRatio(aspectratio);
		//// TODO: Part 4c

		tP = 3.1344 * (-MYD / 20) / 600 + (-RSY / 30000) * (-3.14 * delta);
		tYAW = 3.1344 * aspectratio * (MXD / 20) / 800 + (RSX / 30000) * (-3.14 * delta);
		totalY = totalY * camera_speed * delta;
		proxy.RotationYawPitchRollF(tYAW, tP, 0, cameraMtrxOut);
		proxy.MultiplyMatrixF(worldCamera, cameraMtrxOut, worldCamera);
		GW::MATH::GVECTORF Y = { 0.0f,totalY,0.0f,1.0f };
		GW::MATH::GVECTORF XZ = { totalX * camera_speed * delta ,0.0,totalZ * camera_speed * delta, 1.0f };
		proxy.TranslateGlobalF(worldCamera, Y, worldCamera);
		proxy.TranslateLocalF(worldCamera, XZ, worldCamera);
		proxy.InverseF(worldCamera, viewMatrix);
	}; 

	

private:
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		// TODO: Part 1g
		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);
		
		vkDestroyBuffer(device, vertexHandle2, nullptr);
		vkFreeMemory(device, vertexData2, nullptr);
		
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);
		// TODO: Part 2d
		vkDestroyDescriptorSetLayout(device, layoutOfDescriptor, nullptr);
		vkDestroyDescriptorPool(device, poolDescriptor, nullptr);
		// free the uniform buffer and its handle
		for (int i = 0; i < dataStorage.size(); ++i) {
			vkDestroyBuffer(device, handleStorage[i], nullptr);
			vkFreeMemory(device, dataStorage[i], nullptr);
		}
		// TODO: Part 2d
		// TODO: Part 2e
		// TODO: part 2f
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
	}
};
