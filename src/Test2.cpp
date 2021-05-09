#include "Gfx/Renderer.h"
#include "Utils/Utils.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

int main()
{
    spdlog::set_default_logger(
        spdlog::stdout_color_mt(std::string("logger"), spdlog::color_mode::always));
    spdlog::set_level(spdlog::level::debug);

    spdlog::info("Hello!");
    VaryZulu::Gfx::Renderer app;

    try {
        app.run();
    } catch (const std::exception& e) {
        spdlog::error("{}", e.what());
        return EXIT_FAILURE;
    }
    spdlog::info("Bye");

    return EXIT_SUCCESS;
}
#ifdef WIN32
#pragma warning(pop)
#endif
