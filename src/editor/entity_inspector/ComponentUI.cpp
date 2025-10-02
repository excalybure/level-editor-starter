#include "ComponentUI.h"
#include <imgui.h>

namespace editor
{

bool ComponentUI::renderVec3Control( const char *label,
	math::Vec3f &value,
	const math::Vec3f &resetValue,
	float speed )
{
	bool changed = false;

	ImGui::PushID( label );

	// Label
	ImGui::Text( "%s", label );
	ImGui::SameLine();

	// X component (Red)
	ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.8f, 0.1f, 0.15f, 0.5f ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( 0.9f, 0.2f, 0.2f, 0.7f ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 1.0f, 0.3f, 0.3f, 0.9f ) );
	ImGui::PushItemWidth( 80.0f );
	if ( ImGui::DragFloat( "##X", &value.x, speed ) )
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::PopStyleColor( 3 );
	ImGui::SameLine();

	// Y component (Green)
	ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.1f, 0.8f, 0.15f, 0.5f ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( 0.2f, 0.9f, 0.2f, 0.7f ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 0.3f, 1.0f, 0.3f, 0.9f ) );
	ImGui::PushItemWidth( 80.0f );
	if ( ImGui::DragFloat( "##Y", &value.y, speed ) )
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::PopStyleColor( 3 );
	ImGui::SameLine();

	// Z component (Blue)
	ImGui::PushStyleColor( ImGuiCol_FrameBg, ImVec4( 0.1f, 0.15f, 0.8f, 0.5f ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgHovered, ImVec4( 0.2f, 0.2f, 0.9f, 0.7f ) );
	ImGui::PushStyleColor( ImGuiCol_FrameBgActive, ImVec4( 0.3f, 0.3f, 1.0f, 0.9f ) );
	ImGui::PushItemWidth( 80.0f );
	if ( ImGui::DragFloat( "##Z", &value.z, speed ) )
	{
		changed = true;
	}
	ImGui::PopItemWidth();
	ImGui::PopStyleColor( 3 );
	ImGui::SameLine();

	// Reset button
	if ( ImGui::Button( "Reset" ) )
	{
		value = resetValue;
		changed = true;
	}

	ImGui::PopID();

	return changed;
}

bool ComponentUI::renderFloatControl( const char *label,
	float &value,
	float min,
	float max )
{
	bool changed = false;

	ImGui::PushID( label );
	ImGui::Text( "%s", label );
	ImGui::SameLine();

	// Use DragFloat with bounds if both min and max are non-zero
	if ( min != 0.0f || max != 0.0f )
	{
		if ( ImGui::DragFloat( "##value", &value, 0.1f, min, max ) )
		{
			changed = true;
		}
	}
	else
	{
		// No bounds, free drag
		if ( ImGui::DragFloat( "##value", &value, 0.1f ) )
		{
			changed = true;
		}
	}

	ImGui::PopID();

	return changed;
}

} // namespace editor
