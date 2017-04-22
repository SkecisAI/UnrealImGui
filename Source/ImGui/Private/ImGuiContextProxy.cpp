// Distributed under the MIT License (MIT) (see accompanying LICENSE file)

#include "ImGuiPrivatePCH.h"

#include "ImGuiContextProxy.h"


static constexpr float DEFAULT_CANVAS_WIDTH = 3840.f;
static constexpr float DEFAULT_CANVAS_HEIGHT = 2160.f;

FImGuiContextProxy::FImGuiContextProxy()
{
	ImGuiIO& IO = ImGui::GetIO();

	// Use pre-defined canvas size.
	IO.DisplaySize.x = DEFAULT_CANVAS_WIDTH;
	IO.DisplaySize.y = DEFAULT_CANVAS_HEIGHT;

	// Load texture atlas.
	unsigned char* Pixels;
	IO.Fonts->GetTexDataAsRGBA32(&Pixels, nullptr, nullptr);

	// Initialize key mapping, so context can correctly interpret input state.
	ImGuiInterops::SetUnrealKeyMap(IO);

	// Begin frame to complete context initialization (this is to avoid problems with other systems calling to ImGui
	// during startup).
	BeginFrame();
}

FImGuiContextProxy::~FImGuiContextProxy()
{
	ImGui::Shutdown();
}

void FImGuiContextProxy::Tick(float DeltaSeconds, const FImGuiInputState* InputState)
{
	if (bIsFrameStarted)
	{
		// Broadcast draw event to allow listeners to draw their controls to this context.
		if (DrawEvent.IsBound())
		{
			DrawEvent.Broadcast();
		}

		// Ending frame will produce render output that we capture and store for later use. This also puts context to
		// state in which it does not allow to draw controls, so we want to immediately start a new frame.
		EndFrame();
	}

	// Begin a new frame and set the context back to a state in which it allows to draw controls.
	BeginFrame(DeltaSeconds, InputState);
}

void FImGuiContextProxy::BeginFrame(float DeltaTime, const FImGuiInputState* InputState)
{
	if (!bIsFrameStarted)
	{
		ImGuiIO& IO = ImGui::GetIO();
		IO.DeltaTime = DeltaTime;

		if (InputState)
		{
			ImGuiInterops::CopyInput(IO, *InputState);
		}

		ImGui::NewFrame();

		bIsFrameStarted = true;
	}
}

void FImGuiContextProxy::EndFrame()
{
	if (bIsFrameStarted)
	{
		// Prepare draw data (after this call we cannot draw to this context until we start a new frame).
		ImGui::Render();

		// Update our draw data, so we can use them later during Slate rendering while ImGui is in the middle of the
		// next frame.
		UpdateDrawData(ImGui::GetDrawData());

		bIsFrameStarted = false;
	}
}

void FImGuiContextProxy::UpdateDrawData(ImDrawData* DrawData)
{
	if (DrawData && DrawData->CmdListsCount > 0)
	{
		DrawLists.SetNum(DrawData->CmdListsCount, false);

		for (int Index = 0; Index < DrawData->CmdListsCount; Index++)
		{
			DrawLists[Index].TransferDrawData(*DrawData->CmdLists[Index]);
		}
	}
	else
	{
		// If we are not rendering then this might be a good moment to empty the array.
		DrawLists.Empty();
	}
}
