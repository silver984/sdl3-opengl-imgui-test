#include <SDL3/SDL.h>

#include <glad/gl.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <cstdint>
#include <print>
#include <string_view>
#include <vector>

namespace solum::_gl_wrap {

constexpr std::string_view VERTEX_SHADER_ = R"(#version 330 core
layout(location = 0) in vec2 a_pos;
uniform mat3 u_proj;
void main() {
	vec3 pos = u_proj * vec3(a_pos, 1.0);
	gl_Position = vec4(pos.xy, 0.0, 1.0);
})";

constexpr std::string_view FRAGMENT_SHADER_ = R"(#version 330 core
out vec4 frag_color;
uniform vec4 u_color;
void main() {
	frag_color = u_color;
})";

uint32_t compile_shader_(uint32_t type, std::string_view src) {
	uint32_t out     = glCreateShader(type);
	char const* cstr = src.data();

	glShaderSource(out, 1, &cstr, nullptr);
	glCompileShader(out);

	int32_t result{};
	glGetShaderiv(out, GL_COMPILE_STATUS, &result);

	if (result == GL_FALSE) {
		int32_t len{};
		glGetShaderiv(out, GL_INFO_LOG_LENGTH, &len);
		char* msg = (char*)alloca(len * sizeof(char));
		glGetShaderInfoLog(out, len, &len, msg);
		std::println("Failed to compile shader | what: {}", msg);
		return 0;
	}

	return out;
}

uint32_t create_program_(std::string_view vs_src, std::string_view fs_src) {
	uint32_t out = glCreateProgram();
	uint32_t vs  = compile_shader_(GL_VERTEX_SHADER, vs_src);
	uint32_t fs  = compile_shader_(GL_FRAGMENT_SHADER, fs_src);

	glAttachShader(out, vs);
	glAttachShader(out, fs);

	glLinkProgram(out);

	int32_t result{};
	glGetProgramiv(out, GL_LINK_STATUS, &result);

	if (result == GL_FALSE) {
		int32_t len{};
		glGetProgramiv(out, GL_INFO_LOG_LENGTH, &len);
		char* msg = (char*)alloca(len * sizeof(char));
		glGetProgramInfoLog(out, len, &len, msg);
		std::println("Failed to link program | what: {}", msg);
		return 0;
	};

	glDeleteShader(vs);
	glDeleteShader(fs);

	return out;
}

} // namespace solum::_gl_wrap

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

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(window, ctx);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	ImGui::GetIO().IniFilename = nullptr;

	// setup ---------------------------------------------------------------------------------------------------------------------------

	uint32_t program = solum::_gl_wrap::create_program_(solum::_gl_wrap::VERTEX_SHADER_, solum::_gl_wrap::FRAGMENT_SHADER_);
	int32_t u_proj   = glGetUniformLocation(program, "u_proj");
	int32_t u_color  = glGetUniformLocation(program, "u_color");

	constexpr float PROJECTION[] = {
	        2.f / 1280.f, 0.f,          0.f, //
	        0.f,          -2.f / 720.f, 0.f, //
	        -1.f,         1.f,          1.f, //
	};

	std::vector<float> all_vertices;
	std::vector<uint32_t> all_indices;

	constexpr float SQUARE_VERTICES[] = {
	        0.f,   0.f,   //
	        300.f, 0.f,   //
	        300.f, 300.f, //
	        0.f,   300.f, //
	};

	constexpr uint32_t SQUARE_INDICES[] = {
	        0, 1, 2, //
	        2, 3, 0, //
	};

	constexpr float TRIANGLE_VERTICES[] = {
	        450.f, 0.f,   //
	        600.f, 300.f, //
	        300.f, 300.f, //
	};

	constexpr uint32_t TRIANGLE_INDICES[] = {4, 5, 6};

	all_vertices.append_range(SQUARE_VERTICES);
	all_vertices.append_range(TRIANGLE_VERTICES);

	all_indices.append_range(SQUARE_INDICES);
	all_indices.append_range(TRIANGLE_INDICES);

	uint32_t vao{}, vbo{}, ebo{};

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, all_vertices.size() * sizeof(float), all_vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, all_indices.size() * sizeof(uint32_t), all_indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

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
				int32_t w = event.window.data1;
				int32_t h = event.window.data2;

				int32_t vw = w;
				int32_t vh = (int32_t)(w / ASPECT_RATIO);

				if (vh > h) {
					vh = h;
					vw = (int32_t)(h * ASPECT_RATIO);
				}

				glViewport((w - vw) / 2, (h - vh) / 2, vw, vh);
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

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program);
		glUniformMatrix3fv(u_proj, 1, GL_FALSE, PROJECTION);
		glUniform4f(u_color, 1.f, 1.f, 1.f, 1.f);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, all_indices.size(), GL_UNSIGNED_INT, nullptr);
		glBindVertexArray(0);

		glUseProgram(0);

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

	// cleanup -------------------------------------------------------------------------------------------------------------------------

	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);

	glDeleteProgram(program);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();

	ImGui::DestroyContext();

	SDL_GL_DestroyContext(ctx);
	SDL_DestroyWindow(window);
	SDL_Quit();
}