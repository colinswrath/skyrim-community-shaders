#include "ExtendedMaterials.h"

#include "Util.h"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
	ExtendedMaterials::Settings,
	EnableParallax,
	EnableTerrain,
	EnableComplexMaterial,
	EnableShadows
)

void ExtendedMaterials::DataLoaded()
{
	if (&settings.EnableTerrain) {
		if (auto bLandSpecular = RE::INISettingCollection::GetSingleton()->GetSetting("bLandSpecular:Landscape"); bLandSpecular) {
			if (!bLandSpecular->data.b) {
				logger::info("[CPM] Changing bLandSpecular from {} to {} to support Terrain Parallax", bLandSpecular->data.b, true);
				bLandSpecular->data.b = true;
			}
		}
	}
}

void ExtendedMaterials::DrawSettings()
{
	if (ImGui::TreeNodeEx("Complex Material", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Enable Complex Material", (bool*)&settings.EnableComplexMaterial);
		if (auto _tt = Util::HoverTooltipWrapper()) {
			ImGui::Text(
				"Enables support for the Complex Material specification which makes use of the environment mask. "
				"This includes parallax, as well as more realistic metals and specular reflections. "
				"May lead to some warped textures on modded content which have an invalid alpha channel in their environment mask. ");
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Parallax", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Enable Parallax", (bool*)&settings.EnableParallax);
		if (auto _tt = Util::HoverTooltipWrapper()) {
			ImGui::Text("Enables parallax on standard meshes made for parallax.");
		}

		if (ImGui::Checkbox("Enable Terrain", (bool*)&settings.EnableTerrain)) {
			if (settings.EnableTerrain) {
				DataLoaded();
			}
		}
		if (auto _tt = Util::HoverTooltipWrapper()) {
			ImGui::Text(
				"Enables terrain parallax using the alpha channel of each landscape texture. "
				"Therefore, all landscape textures must support parallax for this effect to work properly. ");
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Approximate Soft Shadows", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Checkbox("Enable Shadows", (bool*)&settings.EnableShadows);
		if (auto _tt = Util::HoverTooltipWrapper()) {
			ImGui::Text(
				"Enables cheap soft shadows when using parallax. "
				"This applies to all directional and point lights. ");
		}

		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::TreePop();
	}
}

void ExtendedMaterials::Load(json& o_json)
{
	if (o_json[GetName()].is_object())
		settings = o_json[GetName()];

	Feature::Load(o_json);
}

void ExtendedMaterials::Save(json& o_json)
{
	o_json[GetName()] = settings;
}

void ExtendedMaterials::RestoreDefaultSettings()
{
	settings = {};
}

bool ExtendedMaterials::HasShaderDefine(RE::BSShader::Type shaderType)
{
	switch (shaderType) {
	case RE::BSShader::Type::Lighting:
		return true;
	default:
		return false;
	}
}