/*
 * Dear ImGui for DPF
 * Copyright (C) 2021 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "DearImGui.hpp"

#ifndef DGL_NO_SHARED_RESOURCES
# include "src/Resources.hpp"
#endif

#ifndef IMGUI_SKIP_IMPLEMENTATION
# define IMGUI_DPF_BACKEND
# include "DearImGui/imgui.cpp"
# include "DearImGui/imgui_demo.cpp"
# include "DearImGui/imgui_draw.cpp"
# include "DearImGui/imgui_tables.cpp"
# include "DearImGui/imgui_widgets.cpp"
# ifdef DGL_USE_OPENGL3
#  include "DearImGui/imgui_impl_opengl3.cpp"
# else
#  include "DearImGui/imgui_impl_opengl2.cpp"
# endif
#else
# ifdef DGL_USE_OPENGL3
#  include "DearImGui/imgui_impl_opengl3.h"
# else
#  include "DearImGui/imgui_impl_opengl2.h"
# endif
#endif

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

template <class BaseWidget>
struct ImGuiWidget<BaseWidget>::PrivateData {
    ImGuiWidget<BaseWidget>* const self;
    ImGuiContext* context;
    double scaleFactor;

    explicit PrivateData(ImGuiWidget<BaseWidget>* const s)
        : self(s),
          context(nullptr),
          scaleFactor(s->getTopLevelWidget()->getScaleFactor())
    {
        IMGUI_CHECKVERSION();
        context = ImGui::CreateContext();
        ImGui::SetCurrentContext(context);

        ImGuiIO& io(ImGui::GetIO());
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.DisplaySize.x = self->getWidth();
        io.DisplaySize.y = self->getHeight();
        io.DisplayFramebufferScale = ImVec2(scaleFactor, scaleFactor);
        io.FontGlobalScale = scaleFactor;
        io.IniFilename = nullptr;

#ifndef DGL_NO_SHARED_RESOURCES
        using namespace dpf_resources;
        ImFontConfig fc;
        fc.FontDataOwnedByAtlas = false;
        fc.OversampleH = 1;
        fc.OversampleV = 1;
        fc.PixelSnapH = 1;
        io.Fonts->AddFontFromMemoryTTF((void*)dejavusans_ttf, dejavusans_ttf_size, 13.0f * scaleFactor, &fc);
        io.Fonts->Build();
        // ImGui::GetStyle().ScaleAllSizes(scaleFactor);
#endif

        io.KeyMap[ImGuiKey_Tab] = '\t';
        io.KeyMap[ImGuiKey_LeftArrow] = 0xff + kKeyLeft - kKeyF1;
        io.KeyMap[ImGuiKey_RightArrow] = 0xff + kKeyRight - kKeyF1;
        io.KeyMap[ImGuiKey_UpArrow] = 0xff + kKeyUp - kKeyF1;
        io.KeyMap[ImGuiKey_DownArrow] = 0xff + kKeyDown - kKeyF1;
        io.KeyMap[ImGuiKey_PageUp] = 0xff + kKeyPageUp - kKeyF1;
        io.KeyMap[ImGuiKey_PageDown] = 0xff + kKeyPageDown - kKeyF1;
        io.KeyMap[ImGuiKey_Home] = 0xff + kKeyHome - kKeyF1;
        io.KeyMap[ImGuiKey_End] = 0xff + kKeyEnd - kKeyF1;
        io.KeyMap[ImGuiKey_Insert] = 0xff + kKeyInsert - kKeyF1;
        io.KeyMap[ImGuiKey_Delete] = kKeyDelete;
        io.KeyMap[ImGuiKey_Backspace] = kKeyBackspace;
        io.KeyMap[ImGuiKey_Space] = ' ';
        io.KeyMap[ImGuiKey_Enter] = '\r';
        io.KeyMap[ImGuiKey_Escape] = kKeyEscape;
        // io.KeyMap[ImGuiKey_KeyPadEnter] = '\n';
        io.KeyMap[ImGuiKey_A] = 'a';
        io.KeyMap[ImGuiKey_C] = 'c';
        io.KeyMap[ImGuiKey_V] = 'v';
        io.KeyMap[ImGuiKey_X] = 'x';
        io.KeyMap[ImGuiKey_Y] = 'y';
        io.KeyMap[ImGuiKey_Z] = 'z';

#ifdef DGL_USE_OPENGL3
        ImGui_ImplOpenGL3_Init();
#else
        ImGui_ImplOpenGL2_Init();
#endif
    }

    ~PrivateData()
    {
        ImGui::SetCurrentContext(context);
#ifdef DGL_USE_OPENGL3
        ImGui_ImplOpenGL3_Shutdown();
#else
        ImGui_ImplOpenGL2_Shutdown();
#endif
        ImGui::DestroyContext(context);
    }

    float getDisplayX() const noexcept;
    float getDisplayY() const noexcept;

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

template <class BaseWidget>
void ImGuiWidget<BaseWidget>::idleCallback()
{
    BaseWidget::repaint();
}

template <class BaseWidget>
void ImGuiWidget<BaseWidget>::onDisplay()
{
    ImGui::SetCurrentContext(pData->context);

#ifdef DGL_USE_OPENGL3
    ImGui_ImplOpenGL3_NewFrame();
#else
    ImGui_ImplOpenGL2_NewFrame();
#endif

    ImGui::NewFrame();
    onImGuiDisplay();
    ImGui::Render();

    /*
    const GraphicsContext& gc(getGraphicsContext());
    Color::fromHTML("#373").setFor(gc);
    Rectangle<double> r(0, 0, getWidth(), getHeight());
    r.draw(gc);
    */

    if (ImDrawData* const data = ImGui::GetDrawData())
    {
        data->DisplayPos.x = -pData->getDisplayX();
        data->DisplayPos.y = pData->getDisplayY();
#ifdef DGL_USE_OPENGL3
        ImGui_ImplOpenGL3_RenderDrawData(data);
#else
        ImGui_ImplOpenGL2_RenderDrawData(data);
#endif
    }
}

template <class BaseWidget>
bool ImGuiWidget<BaseWidget>::onKeyboard(const Widget::KeyboardEvent& event)
{
    if (BaseWidget::onKeyboard(event))
        return true;

    ImGui::SetCurrentContext(pData->context);

    ImGuiIO& io(ImGui::GetIO());

    io.KeyCtrl  = event.mod & kModifierControl;
    io.KeyShift = event.mod & kModifierShift;
    io.KeyAlt   = event.mod & kModifierAlt;
    io.KeySuper = event.mod & kModifierSuper;

    d_stdout("onKeyboard %u %u", event.key, event.keycode);

    if (event.key <= kKeyDelete)
        io.KeysDown[event.key] = event.press;
    else if (event.key >= kKeyF1 && event.key <= kKeyPause)
        io.KeysDown[0xff + event.key - kKeyF1] = event.press;

    return io.WantCaptureKeyboard;
}

template <class BaseWidget>
bool ImGuiWidget<BaseWidget>::onCharacterInput(const Widget::CharacterInputEvent& event)
{
    if (BaseWidget::onCharacterInput(event))
        return true;

    ImGuiIO& io(ImGui::GetIO());

    switch (event.character)
    {
    case kKeyBackspace:
    case kKeyEscape:
    case kKeyDelete:
    case '\n':
    case '\r':
    case '\t':
        break;
    default:
        d_stdout("input %u %u %lu '%s'", event.keycode, event.character, std::strlen(event.string), event.string);
        io.AddInputCharactersUTF8(event.string);
        break;
    }

    return io.WantCaptureKeyboard;
}

template <class BaseWidget>
bool ImGuiWidget<BaseWidget>::onMouse(const Widget::MouseEvent& event)
{
    if (BaseWidget::onMouse(event))
        return true;

    ImGui::SetCurrentContext(pData->context);

    ImGuiIO& io(ImGui::GetIO());

    switch (event.button)
    {
    case 1:
        io.MouseDown[0] = event.press;
        break;
    case 2:
        io.MouseDown[2] = event.press;
        break;
    case 3:
        io.MouseDown[1] = event.press;
        break;
    }

    return io.WantCaptureMouse;
}

template <class BaseWidget>
bool ImGuiWidget<BaseWidget>::onMotion(const Widget::MotionEvent& event)
{
    if (BaseWidget::onMotion(event))
        return true;

    ImGui::SetCurrentContext(pData->context);

    ImGuiIO& io(ImGui::GetIO());
    io.MousePos.x = event.pos.getX();
    io.MousePos.y = event.pos.getY();
    return false;
}

template <class BaseWidget>
bool ImGuiWidget<BaseWidget>::onScroll(const Widget::ScrollEvent& event)
{
    if (BaseWidget::onScroll(event))
        return true;

    ImGui::SetCurrentContext(pData->context);

    ImGuiIO& io(ImGui::GetIO());
    io.MouseWheel += event.delta.getY();
    io.MouseWheelH += event.delta.getX();
    return io.WantCaptureMouse;
}

template <class BaseWidget>
void ImGuiWidget<BaseWidget>::onResize(const Widget::ResizeEvent& event)
{
    BaseWidget::onResize(event);

    ImGui::SetCurrentContext(pData->context);

    ImGuiIO& io(ImGui::GetIO());
    io.DisplaySize.x = event.size.getWidth();
    io.DisplaySize.y = event.size.getHeight();
}

// --------------------------------------------------------------------------------------------------------------------
// ImGuiSubWidget

template <>
float ImGuiWidget<SubWidget>::PrivateData::getDisplayX() const noexcept
{
    return self->getAbsoluteX();
}

template <>
float ImGuiWidget<SubWidget>::PrivateData::getDisplayY() const noexcept
{
    return self->getWindow().getHeight() - self->getAbsoluteY() - self->getHeight();
}

template <>
ImGuiWidget<SubWidget>::ImGuiWidget(Widget* const parent)
    : SubWidget(parent),
      pData(new PrivateData(this))
{
    getWindow().addIdleCallback(this, 1000 / 60); // 60 fps
}

template <>
ImGuiWidget<SubWidget>::~ImGuiWidget()
{
    getWindow().removeIdleCallback(this);
    delete pData;
}

template class ImGuiWidget<SubWidget>;

// --------------------------------------------------------------------------------------------------------------------
// ImGuiTopLevelWidget

template <>
float ImGuiWidget<TopLevelWidget>::PrivateData::getDisplayX() const noexcept
{
    return 0.0f;
}

template <>
float ImGuiWidget<TopLevelWidget>::PrivateData::getDisplayY() const noexcept
{
    return 0.0f;
}

template <>
ImGuiWidget<TopLevelWidget>::ImGuiWidget(Window& windowToMapTo)
    : TopLevelWidget(windowToMapTo),
      pData(new PrivateData(this))
{
    addIdleCallback(this, 1000 / 60); // 60 fps
}

template <>
ImGuiWidget<TopLevelWidget>::~ImGuiWidget()
{
    removeIdleCallback(this);
    delete pData;
}

template class ImGuiWidget<TopLevelWidget>;

// --------------------------------------------------------------------------------------------------------------------
// ImGuiStandaloneWindow

template <>
float ImGuiWidget<StandaloneWindow>::PrivateData::getDisplayX() const noexcept
{
    return 0.0f;
}

template <>
float ImGuiWidget<StandaloneWindow>::PrivateData::getDisplayY() const noexcept
{
    return 0.0f;
}

template <>
ImGuiWidget<StandaloneWindow>::ImGuiWidget(Application& app)
    : StandaloneWindow(app),
      pData(new PrivateData(this))
{
    Window::addIdleCallback(this, 1000 / 60); // 60 fps
}

template <>
ImGuiWidget<StandaloneWindow>::ImGuiWidget(Application& app, Window& transientParentWindow)
    : StandaloneWindow(app, transientParentWindow),
      pData(new PrivateData(this))
{
    Window::addIdleCallback(this, 1000 / 60); // 60 fps
}

template <>
ImGuiWidget<StandaloneWindow>::~ImGuiWidget()
{
    Window::removeIdleCallback(this);
    delete pData;
}

template class ImGuiWidget<StandaloneWindow>;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
