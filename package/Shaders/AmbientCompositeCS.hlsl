#include "Common/Color.hlsl"
#include "Common/DeferredShared.hlsli"
#include "Common/FrameBuffer.hlsl"
#include "Common/GBuffer.hlsli"
#include "Common/VR.hlsli"

Texture2D<unorm half3> AlbedoTexture : register(t0);
Texture2D<unorm half3> NormalRoughnessTexture : register(t1);

#if defined(SKYLIGHTING)
#	include "Skylighting/Skylighting.hlsli"
Texture2D<unorm float> DepthTexture : register(t2);
Texture3D<sh2> SkylightingProbeArray : register(t3);
#endif

#if defined(SSGI)
Texture2D<half4> SSGITexture : register(t4);
#endif

RWTexture2D<half3> MainRW : register(u0);
#if defined(SSGI)
RWTexture2D<half3> DiffuseAmbientRW : register(u1);
#endif

[numthreads(8, 8, 1)] void main(uint3 dispatchID
								: SV_DispatchThreadID) {
	half2 uv = half2(dispatchID.xy + 0.5) * BufferDim.zw;
	uint eyeIndex = GetEyeIndexFromTexCoord(uv);

	half3 normalGlossiness = NormalRoughnessTexture[dispatchID.xy];
	half3 normalVS = DecodeNormal(normalGlossiness.xy);

	half3 diffuseColor = MainRW[dispatchID.xy];
	half3 albedo = AlbedoTexture[dispatchID.xy];

	half3 normalWS = normalize(mul(CameraViewInverse[eyeIndex], half4(normalVS, 0)).xyz);

	half3 directionalAmbientColor = mul(DirectionalAmbient, half4(normalWS, 1.0));

	half3 ambient = albedo * directionalAmbientColor;

	diffuseColor = sRGB2Lin(diffuseColor);

	ambient = sRGB2Lin(max(0, ambient));  // Fixes black blobs on the world map
	albedo = sRGB2Lin(albedo);

#if defined(SKYLIGHTING)
	float rawDepth = DepthTexture[dispatchID.xy];
	float4 positionCS = float4(2 * float2(uv.x, -uv.y + 1) - 1, rawDepth, 1);
	float4 positionMS = mul(CameraViewProjInverse[0], positionCS);
	positionMS.xyz = positionMS.xyz / positionMS.w;

	sh2 skylighting = sampleSkylighting(SkylightingProbeArray, positionMS.xyz, normalWS);
	half skylightingDiffuse = shHallucinateZH3Irradiance(skylighting, normalWS);
	skylightingDiffuse = lerp(1, saturate(skylightingDiffuse * 3), 0.75);

	// fadeout
	const float fadeDist = 0.9;
	float fadeFactor = saturate((length(positionMS.xyz) * 2 / SKYLIGHTING_ARRAY_SIZE.x - fadeDist) / (1 - fadeDist));
	skylightingDiffuse = lerp(skylightingDiffuse, 1, fadeFactor);

	ambient *= skylightingDiffuse;
#endif

#if defined(SSGI)
	half4 ssgiDiffuse = SSGITexture[dispatchID.xy];
	ssgiDiffuse.rgb *= albedo;

	ambient *= ssgiDiffuse.a;

	DiffuseAmbientRW[dispatchID.xy] = ambient + ssgiDiffuse.rgb;

#	if defined(INTERIOR)
	diffuseColor *= ssgiDiffuse.a;
#	endif
	diffuseColor += ssgiDiffuse.rgb;
#endif

	ambient = Lin2sRGB(ambient);
	diffuseColor = Lin2sRGB(diffuseColor);

	diffuseColor += ambient;

	MainRW[dispatchID.xy] = diffuseColor;
};