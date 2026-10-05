#include <SDL3/SDL.h>

#include <glad/gl.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <cstdint>
#include <print>

namespace _slv_glwrap {

constexpr char const* VERT_SRC_ = R"(
#version 330 core
layout(location = 0) in vec2 a_pos;
uniform mat4 u_proj;
void main() {
	gl_Position = u_proj * vec4(a_pos, 0.0, 1.0);
}
)";

constexpr char const* FRAG_SRC_ = R"(
#version 330 core
out vec4 frag_color;
uniform vec4 u_color;
void main() {
	frag_color = u_color;
}
)";

GLuint compile_shader_(GLenum type, char const* src) {
	GLuint out = glCreateShader(type);
	glShaderSource(out, 1, &src, nullptr);
	glCompileShader(out);
	glGetShaderiv(out, 0, nullptr);
	return out;
}

GLuint create_program_(char const* vs_src, char const* fs_src) {
	GLuint vs  = compile_shader_(GL_VERTEX_SHADER, vs_src);
	GLuint fs  = compile_shader_(GL_FRAGMENT_SHADER, fs_src);
	GLuint out = glCreateProgram();

	glAttachShader(out, vs);
	glAttachShader(out, fs);

	glLinkProgram(out);

	glGetProgramiv(out, 0, nullptr);

	glDeleteShader(vs);
	glDeleteShader(fs);

	return out;
}

} // namespace _slv_glwrap

int main() {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::println("SDL_Init failed | what: {}", SDL_GetError());
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window = SDL_CreateWindow("SDL3 + OpenGL + ImGui Test", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if (!window) {
		std::println("SDL_CreateWindow failed | what: {}", SDL_GetError());
		return 1;
	}

	SDL_SetWindowMinimumSize(window, 640, 360);

	constexpr float ASPECT_RATIO = 16.f / 9.f;
	SDL_SetWindowAspectRatio(window, ASPECT_RATIO, ASPECT_RATIO);

	SDL_GLContext ctx = SDL_GL_CreateContext(window);

	if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
		std::println("glad failed to load GL");
		return 1;
	}

	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(window, ctx);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	ImGui::GetIO().IniFilename = nullptr;

	// setup ---------------------------------------------------------------------------------------------------------------------------

	constexpr float PROJECTION[16] = {
	        2.f / 1280.f, 0.f,          0.f,  0.f, //
	        0.f,          -2.f / 720.f, 0.f,  0.f, //
	        0.f,          0.f,          -1.f, 0.f, //
	        -1.f,         1.f,          0.f,  1.f, //
	};

	GLuint prog   = _slv_glwrap::create_program_(_slv_glwrap::VERT_SRC_, _slv_glwrap::FRAG_SRC_);
	GLint u_proj  = glGetUniformLocation(prog, "u_proj");
	GLint u_color = glGetUniformLocation(prog, "u_color");

	// a 300x300 square
	float const vertices[] = {
	        0.f,   0.f,   // top left
	        300.f, 0.f,   // top right
	        300.f, 300.f, // bottom right
	        0.f,   300.f, // bottom left
	};

	uint32_t const indices[] = {0, 1, 2, 2, 3, 0};

	GLuint vao, vbo, ebo;

	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), (void*)vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), (void*)indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	// ---------------------------------------------------------------------------------------------------------------------------------

	uint64_t next_frame_ns = SDL_GetTicksNS();

	bool running          = true;
	bool window_minimized = false;

	while (running) {
		SDL_Event event{};
		while (SDL_PollEvent(&event)) {
			ImGui_ImplSDL3_ProcessEvent(&event);
			switch (event.type) {
			case SDL_EVENT_QUIT:
				running = false;
				break;
			case SDL_EVENT_WINDOW_MINIMIZED:
				window_minimized = true;
				break;
			case SDL_EVENT_WINDOW_RESTORED:
				window_minimized = false;
				break;
			case SDL_EVENT_KEY_DOWN:
				if (event.key.key == SDLK_F11 && !event.key.repeat) {
					bool const fs = SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN;
					SDL_SetWindowFullscreen(window, !fs);
				}
				break;
			case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
				glViewport(0, 0, event.window.data1, event.window.data2);
				break;
			}
		}

		if (window_minimized) {
			constexpr uint64_t DELAY_NS = 100'000'000; // 100MS
			SDL_DelayNS(DELAY_NS);
			next_frame_ns = SDL_GetTicksNS();          // dont burst on restore
			continue;
		}

		// update ------------------------------------------------------------------------------------------------------------------

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();

		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2{5.f, 5.f});
		ImGui::Begin("##debug", nullptr,
		             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMouseInputs |
		                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

		ImGuiIO& io = ImGui::GetIO();
		ImGui::Text("%.0fFPS (%.3fMS)", io.Framerate, 1000.f / io.Framerate);

		ImGui::End();

		// render ------------------------------------------------------------------------------------------------------------------

		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(prog);
		glUniformMatrix4fv(u_proj, 1, GL_FALSE, PROJECTION);
		glUniform4f(u_color, 1.f, 0.f, 0.f, 1.f);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		SDL_GL_SwapWindow(window);

		// -------------------------------------------------------------------------------------------------------------------------

		constexpr uint8_t MAX_FPS   = 120;
		constexpr uint64_t FRAME_NS = 1'000'000'000 / MAX_FPS; // 1S / 120FPS

		next_frame_ns += FRAME_NS;
		uint64_t now_ns = SDL_GetTicksNS();

		if (now_ns < next_frame_ns) {
			SDL_DelayPrecise(next_frame_ns - now_ns);
		} else {
			next_frame_ns = now_ns; // fell behind, dont burst to catch up
		}
	}

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteProgram(prog);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();

	ImGui::DestroyContext();

	SDL_GL_DestroyContext(ctx);
	SDL_DestroyWindow(window);
	SDL_Quit();
}