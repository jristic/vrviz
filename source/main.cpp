/*
Attribution for use of ImGui example code: 

The MIT License (MIT)

Copyright (c) 2014 Omar Cornut

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifdef _MSC_VER
#pragma warning (disable: 4996)         // 'This function or variable may be unsafe': strcpy, strdup, sprintf, vsnprintf, sscanf, fopen
#include <Windows.h>
#include <Imm.h>
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"                  // for .png loading
#include "imgui.h"

// glew & glfw
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#ifdef _MSC_VER
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include <iostream>
#include <vector>
#include <math.h>

#include "shader.h"

static GLFWwindow* window;
static GLuint fontTex;
static bool mousePressed[2] = { false, false };
static ImVec2 mousePosScale(1.0f, 1.0f);
static int sourceWidth	= 300;
static int sourceHeight	= 150;
static int targetScale 	= 2;

// This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
// If text or lines are blurry when integrating ImGui in your engine:
// - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)
// - try adjusting ImGui::GetIO().PixelCenterOffset to 0.5f or 0.375f
static void ImImpl_RenderDrawLists(ImDrawList** const cmd_lists, int cmd_lists_count)
{
	if (cmd_lists_count == 0)
		return;

	// We are using the OpenGL fixed pipeline to make the example code simpler to read!
	// A probable faster way to render would be to collate all vertices from all cmd_lists into a single vertex buffer.
	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, vertex/texcoord/color pointers.
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_SCISSOR_TEST);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	// Setup texture
	glBindTexture(GL_TEXTURE_2D, fontTex);
	glEnable(GL_TEXTURE_2D);

	// Setup orthographic projection matrix
	const float width = ImGui::GetIO().DisplaySize.x;
	const float height = ImGui::GetIO().DisplaySize.y;
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, width, height, 0.0f, -1.0f, +1.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Render command lists
	for (int n = 0; n < cmd_lists_count; n++)
	{
		const ImDrawList* cmd_list = cmd_lists[n];
		const unsigned char* vtx_buffer = (const unsigned char*)cmd_list->vtx_buffer.begin();
		glVertexPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer));
		glTexCoordPointer(2, GL_FLOAT, sizeof(ImDrawVert), (void*)(vtx_buffer+8));
		glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(ImDrawVert), (void*)(vtx_buffer+16));

		int vtx_offset = 0;
		const ImDrawCmd* pcmd_end = cmd_list->commands.end();
		for (const ImDrawCmd* pcmd = cmd_list->commands.begin(); pcmd != pcmd_end; pcmd++)
		{
			glScissor((int)pcmd->clip_rect.x, (int)(height - pcmd->clip_rect.w), (int)(pcmd->clip_rect.z - pcmd->clip_rect.x), (int)(pcmd->clip_rect.w - pcmd->clip_rect.y));
			glDrawArrays(GL_TRIANGLES, vtx_offset, pcmd->vtx_count);
			vtx_offset += pcmd->vtx_count;
		}
	}
	glDisable(GL_SCISSOR_TEST);
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
}

// NB: ImGui already provide OS clipboard support for Windows so this isn't needed if you are using Windows only.
static const char* ImImpl_GetClipboardTextFn()
{
	return glfwGetClipboardString(window);
}

static void ImImpl_SetClipboardTextFn(const char* text)
{
	glfwSetClipboardString(window, text);
}

#ifdef _MSC_VER
// Notify OS Input Method Editor of text input position (e.g. when using
// Japanese/Chinese inputs, otherwise this isn't needed)
static void ImImpl_ImeSetInputScreenPosFn(int x, int y)
{
	HWND hwnd = glfwGetWin32Window(window);
	if (HIMC himc = ImmGetContext(hwnd))
	{
		COMPOSITIONFORM cf;
		cf.ptCurrentPos.x = x;
		cf.ptCurrentPos.y = y;
		cf.dwStyle = CFS_FORCE_POSITION;
		ImmSetCompositionWindow(himc, &cf);
	}
}
#endif

// GLFW callbacks to get events
static void glfw_error_callback(int error, const char* description)
{
	fputs(description, stderr);
}

static void glfw_mouse_button_callback(
	GLFWwindow* window,
	int button,
	int action,
	int mods)
{
	if (action == GLFW_PRESS && button >= 0 && button < 2)
		mousePressed[button] = true;
}

static void glfw_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheel = (yoffset != 0.0f) ? yoffset > 0.0f ? 1 : - 1 : 0;
	// Mouse wheel: -1,0,+1
}

static void glfw_key_callback(
	GLFWwindow* window,
	int key,
	int scancode,
	int action,
	int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (action == GLFW_PRESS)
		io.KeysDown[key] = true;
	if (action == GLFW_RELEASE)
		io.KeysDown[key] = false;
	io.KeyCtrl = (mods & GLFW_MOD_CONTROL) != 0;
	io.KeyShift = (mods & GLFW_MOD_SHIFT) != 0;
	if (action == GLFW_RELEASE && (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q))
		glfwSetWindowShouldClose(window, true);
}

static void glfw_char_callback(GLFWwindow* window, unsigned int c)
{
	if (c > 0 && c < 0x10000)
		ImGui::GetIO().AddInputCharacter((unsigned short)c);
}

// OpenGL code based on http://open.gl tutorials
void InitGL()
{
	glfwSetErrorCallback(glfw_error_callback);

	if (!glfwInit())
		exit(1);

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	window = glfwCreateWindow(
		sourceWidth * targetScale, 
		sourceHeight * targetScale,
		"vrviz",
		NULL,
		NULL);
	glfwMakeContextCurrent(window);
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwSetMouseButtonCallback(window, glfw_mouse_button_callback);
	glfwSetScrollCallback(window, glfw_scroll_callback);
	glfwSetCharCallback(window, glfw_char_callback);

	glewInit();
}

void InitImGui()
{
	int w, h;
	int fb_w, fb_h;
	glfwGetWindowSize(window, &w, &h);
	glfwGetFramebufferSize(window, &fb_w, &fb_h);
	mousePosScale.x = (float)fb_w / w;                  // Some screens e.g. Retina display have framebuffer size != from window size, and mouse inputs are given in window/screen coordinates.
	mousePosScale.y = (float)fb_h / h;

	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2((float)fb_w, (float)fb_h);  // Display size, in pixels. For clamping windows positions.
	io.DeltaTime = 1.0f/60.0f;                          // Time elapsed since last frame, in seconds (in this sample app we'll override this every frame because our timestep is variable)
	io.PixelCenterOffset = 0.0f;                        // Align OpenGL texels
	io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;             // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
	io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
	io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
	io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
	io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
	io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
	io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
	io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
	io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
	io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
	io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
	io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
	io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
	io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

	io.RenderDrawListsFn = ImImpl_RenderDrawLists;
	io.SetClipboardTextFn = ImImpl_SetClipboardTextFn;
	io.GetClipboardTextFn = ImImpl_GetClipboardTextFn;
#ifdef _MSC_VER
	io.ImeSetInputScreenPosFn = ImImpl_ImeSetInputScreenPosFn;
#endif

	// Load font texture
	glGenTextures(1, &fontTex);
	glBindTexture(GL_TEXTURE_2D, fontTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

#if 1
	// Default font (embedded in code)
	const void* png_data;
	unsigned int png_size;
	ImGui::GetDefaultFontData(NULL, NULL, &png_data, &png_size);
	int tex_x, tex_y, tex_comp;
	void* tex_data = stbi_load_from_memory((const unsigned char*)png_data, (int)png_size, &tex_x, &tex_y, &tex_comp, 0);
	IM_ASSERT(tex_data != NULL);
#else
	// Custom font from filesystem
	io.Font = new ImBitmapFont();
	io.Font->LoadFromFile("../../extra_fonts/mplus-2m-medium_18.fnt");
	IM_ASSERT(io.Font->IsLoaded());

	int tex_x, tex_y, tex_comp;
	void* tex_data = stbi_load("../../extra_fonts/mplus-2m-medium_18.png", &tex_x, &tex_y, &tex_comp, 0);
	IM_ASSERT(tex_data != NULL);
	
	// Automatically find white pixel from the texture we just loaded
	// (io.FontTexUvForWhite needs to contains UV coordinates pointing to a white pixel in order to render solid objects)
	for (int tex_data_off = 0; tex_data_off < tex_x*tex_y; tex_data_off++)
		if (((unsigned int*)tex_data)[tex_data_off] == 0xffffffff)
		{
			io.FontTexUvForWhite = ImVec2((float)(tex_data_off % tex_x)/(tex_x), (float)(tex_data_off / tex_x)/(tex_y));
			break;
		}
#endif

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_x, tex_y, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data);
	stbi_image_free(tex_data);
}

void UpdateImGui()
{
	ImGuiIO& io = ImGui::GetIO();

	// Setup timestep
	static double time = 0.0f;
	const double current_time =  glfwGetTime();
	io.DeltaTime = (float)(current_time - time);
	time = current_time;

	// Setup inputs
	// (we already got mouse wheel, keyboard keys & characters from glfw callbacks polled in glfwPollEvents())
	double mouse_x, mouse_y;
	glfwGetCursorPos(window, &mouse_x, &mouse_y);
	io.MousePos = ImVec2((float)mouse_x * mousePosScale.x, (float)mouse_y * mousePosScale.y);      // Mouse position, in pixels (set to -1,-1 if no mouse / on another screen, etc.)
	io.MouseDown[0] = mousePressed[0] || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) != 0;  // If a mouse press event came, always pass it, so we don't miss click-release events that are shorted than our frame.
	io.MouseDown[1] = mousePressed[1] || glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) != 0;

	// Start the frame
	ImGui::NewFrame();
}

void increment_score(int* digits, int num_digits) {
	for (int i = 0 ; i < num_digits ; ++i) {
		if (i == num_digits - 1 || digits[i] < digits[i+1]) {
			++digits[i];
			break;
		}
		else {
			digits[i] = 0;
		}
	}	
}

// Application code
int main(int argc, char** argv)
{
	const int NUM_DIGITS = 7;
	int digits[NUM_DIGITS] = {0};
	bool should_auto_increment = false;
	bool paused = false;
	// Init helpers
	InitGL();
	InitImGui();
	// Init shader
	GLuint shader;
	std::string errors;
	bool success = make_shader_program("line.vert", "line.frag", shader, errors);
	if (!success) {
		std::cerr << "failed to make shader\n" << errors;
		exit(1);
	}
	glUseProgram(shader);
	glUniform1f(glGetUniformLocation(shader, "aspect"),
		(float)sourceWidth/sourceHeight);
	glUseProgram(0);
	// Set up secondary framebuffer for rendering to texture
	GLuint frame_buffer = 0;
	glGenFramebuffers(1, &frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	// The texture we're going to render to
	GLuint rendered_texture;
	glGenTextures(1, &rendered_texture);
	glBindTexture(GL_TEXTURE_2D, rendered_texture);
	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, sourceWidth, sourceHeight, 0, GL_RGB,
		GL_UNSIGNED_BYTE, 0);
	// Use box filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		rendered_texture, 0); 
	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
	// Always check that our framebuffer is ok
	if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		printf("failed to setup framebuffer\n");
		exit(-1);
	}
	// Setup vertex buffers and shader for rendering texture to screen
	const GLfloat g_quad_vertex_buffer_data[] = {
	    -1.0f, -1.0f, 0.0f,
	    3.0f, -1.0f, 0.0f,
	    -1.0f,  3.0f, 0.0f,
	};
	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
	// Create and compile our GLSL program from the shaders
	GLuint quad_shader;
	success = make_shader_program("quad.vert", "quad.frag", quad_shader, errors);
	if (!success) {
		std::cerr << "failed to make shader\n" << errors;
		exit(1);
	}
	GLuint tex_id = glGetUniformLocation(quad_shader, "renderedTexture");
	glUseProgram(quad_shader);
	// Set our "renderedTexture" sampler to user Texture Unit 0
	glUniform1i(tex_id, 0);
	glUseProgram(0);
	// Init geometry
	const int NUM_SHAPES = 9;
	GLuint vertex_buffers[NUM_SHAPES];
	GLuint index_buffers[NUM_SHAPES];
	int index_counts[NUM_SHAPES] = {0};
	std::vector<GLfloat> dataf;
	std::vector<GLuint> datau;
	glGenBuffers(NUM_SHAPES, vertex_buffers);
	glGenBuffers(NUM_SHAPES, index_buffers);
	// Setup our shapes
	// 0 - line
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[0]);
	dataf = {
		-1.f,0.f,0.f,
		1.f,0.f,0.f};
	glBufferData(GL_ARRAY_BUFFER, dataf.size()*sizeof(GLfloat), &dataf[0],
		GL_STATIC_DRAW);
	// 1 - 3 pointed line
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[1]);
	dataf = {
		0.f,0.f,0.f,
		-1.f,0.f,0.f,
		0.5,0.866,0.f,
		0.5,-0.866,0.f};
	glBufferData(GL_ARRAY_BUFFER, dataf.size()*sizeof(GLfloat), &dataf[0],
		GL_STATIC_DRAW);
	// 2 - cross
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[2]);
	dataf = {
		0.f,0.f,0.f,
		-1.f,0.f,0.f,
		0.f,1.f,0.f,
		1.f,0.f,0.f,
		0.f,-1.f,0.f};
	glBufferData(GL_ARRAY_BUFFER, dataf.size()*sizeof(GLfloat), &dataf[0],
		GL_STATIC_DRAW);
	// 3 - fat line
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[3]);
	dataf = {
		-1.f,0.2f,0.f,
		1.f,0.2f,0.f,
		1.f,-0.2f,0.f,
		-1.f,-0.2f,0.f
	};
	glBufferData(GL_ARRAY_BUFFER, dataf.size()*sizeof(GLfloat), &dataf[0],
		GL_STATIC_DRAW);
	// 4 - fat 3-line
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[4]);
	dataf = {
		-1.f,		-0.2f,		0.f,
		-1.f,		0.2f,		0.f,
		-0.115f,	0.2f,		0.f,
		0.316f,		0.949f,		0.f,
		0.663f,		0.748f,		0.f,
		0.3f,		0.f,		0.f,
		0.663f,		-0.748f,	0.f,
		0.316f,		-0.949f,	0.f,
		-0.115f,	-0.2f,		0.f
	};
	glBufferData(GL_ARRAY_BUFFER, dataf.size()*sizeof(GLfloat), &dataf[0],
		GL_STATIC_DRAW);
	// 5 - fat cross
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[5]);
	dataf = {
		-1.f,0.2f,0.f,
		-0.2f,0.2f,0.f,
		-0.2f,1.f,0.f,
		0.2f,1.f,0.f,
		0.2f,0.2f,0.f,
		1.f,0.2f,0.f,
		1.f,-0.2f,0.f,
		0.2f,-0.2f,0.f,
		0.2f,-1.f,0.f,
		-0.2f,-1.f,0.f,
		-0.2f,-0.2f,0.f,
		-1.f,-0.2f,0.f
	};
	glBufferData(GL_ARRAY_BUFFER, dataf.size()*sizeof(GLfloat), &dataf[0],
		GL_STATIC_DRAW);
	// 6 - triangle
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[6]);
	dataf = {
		-1.f,-0.866f,0.f,
		1.f,-0.866f,0.f,
		0.f,0.866f,0.f,
	};
	glBufferData(GL_ARRAY_BUFFER, dataf.size()*sizeof(GLfloat), &dataf[0],
		GL_STATIC_DRAW);
	// 7 - square
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[7]);
	dataf = {
		-0.707f,	-0.707f,	0.f,
		0.707f,		-0.707f,	0.f,
		0.707f,		0.707f,		0.f,
		-0.707f,	0.707f,		0.f
	};
	glBufferData(GL_ARRAY_BUFFER, dataf.size()*sizeof(GLfloat), &dataf[0],
		GL_STATIC_DRAW);
	// 8 - pentagon
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[8]);
	dataf = {
		1.f,		0.f,		0.f,
		0.309f,		-0.951f,	0.f,
		-0.809f,	-0.588f,	0.f,
		-0.809f,	0.588f,		0.f,
		0.309f,		0.951f,		0.f
	};
	glBufferData(GL_ARRAY_BUFFER, dataf.size()*sizeof(GLfloat), &dataf[0],
		GL_STATIC_DRAW);
	/*
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[3]);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[4]);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[5]);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[6]);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[7]);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[8]);
	*/
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	// Index buffers
	// 0 - line
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[0]);
	datau = {0,1};
	index_counts[0] = datau.size();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, datau.size()*sizeof(GLuint), &datau[0],
		GL_STATIC_DRAW);
	// 1 - 3-line
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[1]);
	datau = {0,1, 0,2, 0,3};
	index_counts[1] = datau.size();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, datau.size()*sizeof(GLuint), &datau[0],
		GL_STATIC_DRAW);
	// 2 - cross
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[2]);
	datau = {0,1, 0,2, 0,3, 0,4};
	index_counts[2] = datau.size();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, datau.size()*sizeof(GLuint), &datau[0],
		GL_STATIC_DRAW);
	// 3 - fat line
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[3]);
	datau = {0,1, 1,2, 2,3, 3,0};
	index_counts[3] = datau.size();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, datau.size()*sizeof(GLuint), &datau[0],
		GL_STATIC_DRAW);
	// 4 - fat 3-line
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[4]);
	datau = {0,1, 1,2, 2,3, 3,4, 4,5, 5,6, 6,7, 7,8, 8,0};
	index_counts[4] = datau.size();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, datau.size()*sizeof(GLuint), &datau[0],
		GL_STATIC_DRAW);
	// 5 - fat cross
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[5]);
	datau = {0,1, 1,2, 2,3, 3,4, 4,5, 5,6, 6,7, 7,8, 8,9, 9,10, 10,11, 11,0};
	index_counts[5] = datau.size();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, datau.size()*sizeof(GLuint), &datau[0],
		GL_STATIC_DRAW);
	// 6 - triangle
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[6]);
	datau = {0,1, 1,2, 2,0};
	index_counts[6] = datau.size();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, datau.size()*sizeof(GLuint), &datau[0],
		GL_STATIC_DRAW);
	// 7 - square
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[7]);
	datau = {0,1, 1,2, 2,3, 3,0};
	index_counts[7] = datau.size();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, datau.size()*sizeof(GLuint), &datau[0],
		GL_STATIC_DRAW);
	// 8 - pentagon
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[8]);
	datau = {0,1, 1,2, 2,3, 3,4, 4,0};
	index_counts[8] = datau.size();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, datau.size()*sizeof(GLuint), &datau[0],
		GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	int frame_count = 0;

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		ImGuiIO& io = ImGui::GetIO();
		mousePressed[0] = mousePressed[1] = false;
		io.MouseWheel = 0;
		glfwPollEvents();
		UpdateImGui();

		bool shown = ImGui::Begin("Info");
		if (shown) {
			ImGui::InputInt("0", digits);
			ImGui::InputInt("1", digits+1);
			ImGui::InputInt("2", digits+2);
			ImGui::InputInt("3", digits+3);
			ImGui::InputInt("4", digits+4);
			ImGui::InputInt("5", digits+5);
			ImGui::InputInt("6", digits+6);
			if (ImGui::Button("increment")) {
				increment_score(digits, NUM_DIGITS);
			}
			ImGui::Checkbox("auto increment", &should_auto_increment);
			ImGui::Checkbox("paused", &paused);
		}
		ImGui::End();
		// Prevent overflow
		for (int& digit : digits)
			digit = std::max(std::min(digit, NUM_SHAPES-1), 0);
		// Auto increment
		if (should_auto_increment)
			if (frame_count && frame_count % 30 == 0)
				increment_score(digits, NUM_DIGITS);
		// Rendering
		// Render to texture
		glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
		glViewport(0, 0, sourceWidth, sourceHeight);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glUseProgram(shader);
		glUniform1i(glGetUniformLocation(shader, "frame"), frame_count);
		glUniform3f(glGetUniformLocation(shader, "color"), 1.f, 1.f, 0.f);
		for (int i = 0 ; i < NUM_DIGITS ; ++i)
		{
			const int type = digits[i];
			glUniform1i(glGetUniformLocation(shader, "index"), i);
			glUniform1i(glGetUniformLocation(shader, "digit"), digits[i]);
			glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[type]);
			glEnableClientState(GL_VERTEX_ARRAY);
			glVertexPointer(3, GL_FLOAT, 0, 0);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[type]);
			glDrawElements(GL_LINES, index_counts[type], GL_UNSIGNED_INT, 0);
		}

		// Switch to rendering to screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Render texture fullscreen
		glUseProgram(quad_shader);
		// Bind our texture in Texture Unit 0
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, rendered_texture);
		// Use quad buffer
		glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// Unbind resources
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		glUseProgram(0);

		// UI Rendering
		ImGui::Render();
		// Swap
		glfwSwapBuffers(window);
		if (!paused)
			++frame_count;
	}
	// Closing
	ImGui::Shutdown();
	glfwTerminate();
	return 0;
}