#include <SDL3/SDL.h>

#include <glad/gl.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <cstdint>
#include <print>

int main() {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::println("SDL_Init failed | what: {}\n", SDL_GetError());
		return 1;
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_Window* window = SDL_CreateWindow("SDL3 + OpenGL + ImGui Test", 1280, 720, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

	if (!window) {
		std::println("SDL_CreateWindow failed | what: {}\n", SDL_GetError());
		return 1;
	}

	SDL_SetWindowMinimumSize(window, 640, 360);

	constexpr float ASPECT_RATIO = 16.f / 9.f;
	SDL_SetWindowAspectRatio(window, ASPECT_RATIO, ASPECT_RATIO);

	SDL_GLContext ctx = SDL_GL_CreateContext(window);

	if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
		std::println("glad failed to load GL\n");
		return 1;
	}

	IMGUI_CHECKVERSION();

	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplSDL3_InitForOpenGL(window, ctx);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	ImGui::GetIO().IniFilename = nullptr;

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

		ImGui::Render();

		glClearColor(0.f, 0.f, 0.f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

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

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();

	ImGui::DestroyContext();

	SDL_GL_DestroyContext(ctx);
	SDL_DestroyWindow(window);
	SDL_Quit();
}