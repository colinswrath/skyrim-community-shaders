#pragma once

#include <eastl/set.h>
#include <unordered_set>

#include <d3d12.h>

#include <FidelityFX/host/backends/dx12/ffx_dx12.h>

#include <FidelityFX/host/ffx_fsr3.h>
#include <FidelityFX/host/ffx_interface.h>

#include <FidelityFX/host/ffx_brixelizer.h>
#include <FidelityFX/host/ffx_brixelizergi.h>

#include "Buffer.h"
#include "State.h"

class Raytracing
{
public:
	static Raytracing* GetSingleton()
	{
		static Raytracing singleton;
		return &singleton;
	}

	// FidelityFX Brixelizer information
	FfxBrixelizerContextDescription initializationParameters = {};
	FfxBrixelizerContext brixelizerContext = {};
	FfxBrixelizerBakedUpdateDescription brixelizerBakedUpdateDesc = {};

	struct BrixelizerBufferInfo
	{
		uint32_t index;
		const ID3D12Resource* buffer;
	};

	////const cauldron::Texture* m_pSdfAtlas = nullptr;
	////const cauldron::Buffer* m_pBrickAABBs = nullptr;
	////const cauldron::Buffer* m_pCascadeAABBTrees[FFX_BRIXELIZER_MAX_CASCADES] = {};
	////const cauldron::Buffer* m_pCascadeBrickMaps[FFX_BRIXELIZER_MAX_CASCADES] = {};
	////const cauldron::Buffer* m_pGpuScratchBuffer = nullptr;
	//std::vector<BrixelizerInstanceInfo> m_Instances = {};
	//std::vector<BrixelizerBufferInfo> m_Buffers = {};

	winrt::com_ptr<IDXGIAdapter3> dxgiAdapter3;
	winrt::com_ptr<ID3D12Device> d3d12Device;
	winrt::com_ptr<ID3D12CommandQueue> commandQueue;
	winrt::com_ptr<ID3D12CommandAllocator> commandAllocator;
	winrt::com_ptr<ID3D12GraphicsCommandList> commandList;

	void InitD3D12();

	void OpenSharedHandles();

	struct Vertex
	{
		float4 position;
	};

	struct BufferData
	{
		winrt::com_ptr<ID3D12Resource> buffer;
		bool registered = false;
		uint width;
		uint index;
	};

	BufferData AllocateBuffer(const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData);

	eastl::hash_set<RE::BSGeometry*> geometries;

	eastl::hash_map<ID3D11Buffer*, BufferData> vertexBuffers;
	eastl::hash_map<ID3D11Buffer*, BufferData> indexBuffers;

	uint GetBufferIndex(BufferData& a_bufferData);

	void UpdateGeometry(RE::BSGeometry* a_geometry);
	void RemoveGeometry(RE::BSGeometry* a_geometry);

	void RegisterVertexBuffer(const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer);
	void RegisterIndexBuffer(const D3D11_BUFFER_DESC* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Buffer** ppBuffer);

	struct RenderTargetDataD3D12
	{
		// D3D12 Resource
		winrt::com_ptr<ID3D12Resource> d3d12Resource;
	};

	RenderTargetDataD3D12 renderTargetsD3D12[RE::RENDER_TARGET::kTOTAL];
	RenderTargetDataD3D12 ConvertD3D11TextureToD3D12(RE::BSGraphics::RenderTargetData* rtData);

	struct BSLightingShader_SetupGeometry
	{
		static void thunk(RE::BSShader* This, RE::BSRenderPass* Pass, uint32_t RenderFlags)
		{
			func(This, Pass, RenderFlags);
			GetSingleton()->UpdateGeometry(Pass->geometry);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	struct NiNode_Destroy
	{
		static void thunk(RE::NiNode* This)
		{
			GetSingleton()->RemoveGeometry((RE::BSGeometry*)This);
			func(This);
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	static void InstallHooks()
	{
		stl::write_vfunc<0x6, BSLightingShader_SetupGeometry>(RE::VTABLE_BSLightingShader[0]);

		//	stl::detour_thunk<NiNode_Destroy>(REL::RelocationID(68937, 70288));

		logger::info("[Raytracing] Installed hooks");
	}
};