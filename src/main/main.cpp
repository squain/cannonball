/***************************************************************************
    Cannonball Main Entry Point.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

// Version information
#define CANNONBALL_VERSION "0.35"
#define CANNONBALL_YEAR    "2022"

#include <cstring>
#include <iostream>

// SDL Library
#include <SDL.h>

// SDL Specific Code
#include "sdl2/timer.hpp"
#include "sdl2/input.hpp"

#include "video.hpp"

#include "romloader.hpp"
#include "trackloader.hpp"
#include "stdint.hpp"
#include "main.hpp"
#include "engine/outrun.hpp"
#include "frontend/config.hpp"
#include "frontend/menu.hpp"

#include "engine/oinputs.hpp"
#include "engine/ooutputs.hpp"
#include "engine/omusic.hpp"

// Direct X Haptic Support.
// Fine to include on non-windows builds as dummy functions used.
#include "directx/ffeedback.hpp"

// ------------------------------------------------------------------------------------------------
// Initialize Shared Variables
// ------------------------------------------------------------------------------------------------
using namespace cannonball;

int    cannonball::state       = STATE_BOOT;
double cannonball::frame_ms    = 0;
int    cannonball::frame       = 0;
bool   cannonball::tick_frame  = true;
int    cannonball::fps_counter = 0;

// ------------------------------------------------------------------------------------------------
// Main Variables and Pointers
// ------------------------------------------------------------------------------------------------
Audio cannonball::audio;
Menu* menu;
bool pause_engine;


// ------------------------------------------------------------------------------------------------

static void quit_func(int code)
{
    audio.stop_audio();
    input.close_joy();
    forcefeedback::close();
    delete menu;
    SDL_Quit();
    exit(code);
}

static void process_events(void)
{
    SDL_Event event;

    // Grab all events from the queue.
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_KEYDOWN:
                // Handle key presses.
                if (event.key.keysym.sym == SDLK_ESCAPE)
                    state = STATE_QUIT;
                else
                    input.handle_key_down(&event.key.keysym);
                break;

            case SDL_KEYUP:
                input.handle_key_up(&event.key.keysym);
                break;

            case SDL_JOYAXISMOTION:
                input.handle_joy_axis(&event.jaxis);
                break;

            case SDL_JOYBUTTONDOWN:
                input.handle_joy_down(&event.jbutton);
                break;

            case SDL_JOYBUTTONUP:
                input.handle_joy_up(&event.jbutton);
                break;

            case SDL_CONTROLLERAXISMOTION:
                input.handle_controller_axis(&event.caxis);
                break;

            case SDL_CONTROLLERBUTTONDOWN:
                input.handle_controller_down(&event.cbutton);
                break;

            case SDL_CONTROLLERBUTTONUP:
                input.handle_controller_up(&event.cbutton);
                break;

            case SDL_JOYHATMOTION:
                input.handle_joy_hat(&event.jhat);
                break;

            case SDL_JOYDEVICEADDED:
                input.open_joy();
                break;

            case SDL_JOYDEVICEREMOVED:
                input.close_joy();
                break;

            case SDL_QUIT:
                // Handle quit requests (like Ctrl-c).
                state = STATE_QUIT;
                break;
        }
    }
}

static void tick()
{
    frame++;

    // Non standard FPS: Determine whether to tick certain logic for the current frame.
    if (config.fps == 60)
        tick_frame = frame & 1;
    else if (config.fps == 120)
        tick_frame = (frame & 3) == 1;

    process_events();

    if (tick_frame)
    {
        oinputs.tick();           // Do Controls
        oinputs.do_gear();        // Digital Gear
    }
     
    switch (state)
    {
        case STATE_GAME:
        {
            if (tick_frame)
            {
                if (input.has_pressed(Input::TIMER)) outrun.freeze_timer = !outrun.freeze_timer;
                if (input.has_pressed(Input::PAUSE)) pause_engine = !pause_engine;
                if (input.has_pressed(Input::MENU))  state = STATE_INIT_MENU;
            }

            if (!pause_engine || input.has_pressed(Input::STEP))
            {
                outrun.tick(tick_frame);
                if (tick_frame) input.frame_done();
                osoundint.tick();
            }
            else
            {                
                if (tick_frame) input.frame_done();
            }
        }
        break;

        case STATE_INIT_GAME:
            if (config.engine.jap && !roms.load_japanese_roms())
            {
                state = STATE_QUIT;
            }
            else
            {
                tick_frame = true;
                pause_engine = false;
                outrun.init();
                state = STATE_GAME;
            }
            break;

        case STATE_MENU:
            menu->tick();
            input.frame_done();
            osoundint.tick();
            break;

        case STATE_INIT_MENU:
            oinputs.init();
            outrun.outputs->init();
            menu->init();
            state = STATE_MENU;
            break;
    }

    // Map OutRun outputs to CannonBall devices (SmartyPi Interface / Controller Rumble)
    outrun.outputs->writeDigitalToConsole();
    if (tick_frame)
    {
         input.set_rumble(outrun.outputs->is_set(OOutputs::D_MOTOR), config.controls.rumble);
    }
}

static void main_loop()
{
    // FPS Counter (If Enabled)
    Timer fps_count;
    int frame = 0;
    fps_count.start();

    // General Frame Timing
    bool vsync = config.video.vsync == 1 && video.supports_vsync();
    Timer frame_time;
    int t;                              // Actual timing of tick in ms as measured by SDL (ms)
    double deltatime  = 0;              // Time we want an entire frame to take (ms)
    int deltaintegral = 0;              // Integer version of above

    while (state != STATE_QUIT)
    {
        frame_time.start();
        // Tick Engine
        tick();

        // Draw SDL Video
        video.prepare_frame();
        video.render_frame();

        // Fill SDL Audio Buffer For Callback
        audio.tick();
        
        // Calculate Timings. Cap Frame Rate. Note this might be trumped by V-Sync
        if (!vsync)
        {
            deltatime += (frame_ms * audio.adjust_speed());
            deltaintegral = (int)deltatime;
            t = frame_time.get_ticks();
            
            if (t < deltatime)
                SDL_Delay((Uint32)(deltatime - t));

            deltatime -= deltaintegral;
        }

        if (config.video.fps_count)
        {
            frame++;
            // One second has elapsed
            if (fps_count.get_ticks() >= 1000)
            {
                fps_counter = frame;
                frame       = 0;
                fps_count.start();
            }
        }
    }

    quit_func(0);
}

static void print_version()
{
    std::cout << "Cannonball " << CANNONBALL_VERSION << std::endl;
    std::cout << "An Enhanced OutRun Engine" << std::endl;
    std::cout << "Copyright Chris White " << CANNONBALL_YEAR << std::endl;
}

static void print_usage(const char* program_name)
{
    print_version();
    std::cout << std::endl;
    std::cout << "Usage: " << program_name << " [OPTIONS]" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help              Show this help message and exit" << std::endl;
    std::cout << "  -v, --version           Show version information and exit" << std::endl;
    std::cout << "  -c, --config <file>     Path to config.xml file" << std::endl;
    std::cout << "                          (default: config.xml in current directory)" << std::endl;
    std::cout << "  -t, --track <file>      Load custom track data from LayOut Editor" << std::endl;
    std::cout << std::endl;
    std::cout << "Required Files:" << std::endl;
    std::cout << "  roms/          Directory containing OutRun Rev B ROM files" << std::endl;
    std::cout << "  res/           Directory with tilemap.bin, tilepatch.bin," << std::endl;
    std::cout << "                 and gamecontrollerdb.txt" << std::endl;
    std::cout << "  config.xml     Configuration file (created on first run)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << std::endl;
    std::cout << "      Run with default settings" << std::endl;
    std::cout << std::endl;
    std::cout << "  " << program_name << " --config /path/to/config.xml" << std::endl;
    std::cout << "      Run with a specific configuration file" << std::endl;
    std::cout << std::endl;
    std::cout << "  " << program_name << " --track mytrack.bin" << std::endl;
    std::cout << "      Load a custom track from LayOut Editor" << std::endl;
    std::cout << std::endl;
    std::cout << "In-Game Controls:" << std::endl;
    std::cout << "  ESC            Quit game" << std::endl;
    std::cout << "  F1             Pause" << std::endl;
    std::cout << "  F2             Frame step (when paused)" << std::endl;
    std::cout << "  F3             Toggle menu" << std::endl;
    std::cout << std::endl;
    std::cout << "For more information, visit:" << std::endl;
    std::cout << "  https://github.com/djyt/cannonball" << std::endl;
}

// Command line parser supporting both short and long options.
// Returns true if everything is ok to proceed with launching the engine.
static bool parse_command_line(int argc, char* argv[])
{
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0 ||
            strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "/?") == 0)
        {
            print_usage(argv[0]);
            exit(0);
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0 ||
                 strcmp(argv[i], "-version") == 0)
        {
            print_version();
            exit(0);
        }
        else if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0 ||
                  strcmp(argv[i], "-cfgfile") == 0) && i + 1 < argc)
        {
            config.set_config_file(argv[++i]);
        }
        else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--track") == 0 ||
                  strcmp(argv[i], "-file") == 0) && i + 1 < argc)
        {
            if (!trackloader.set_layout_track(argv[++i]))
            {
                std::cerr << "Error: Failed to load track file: " << argv[i] << std::endl;
                return false;
            }
        }
        else if (argv[i][0] == '-')
        {
            std::cerr << "Error: Unknown option: " << argv[i] << std::endl;
            std::cerr << "Try '" << argv[0] << " --help' for more information." << std::endl;
            return false;
        }
    }
    return true;
}

int main(int argc, char* argv[])
{
    // Parse command line arguments (config file location, LayOut data) 
    bool ok = parse_command_line(argc, argv);

    if (ok)
    {
        config.load(); // Load config.XML file
        ok = roms.load_revb_roms(config.sound.fix_samples);
    }
    if (!ok)
    {
        quit_func(1);
        return 0;
    }

    // Load gamecontrollerdb.txt mappings
    if (SDL_GameControllerAddMappingsFromFile((config.data.res_path + "gamecontrollerdb.txt").c_str()) == -1)
        std::cout << "Unable to load controller mapping" << std::endl;

    // Initialize timer and video systems
    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC) == -1)
    {
        std::cerr << "SDL Initialization Failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Load patched widescreen tilemaps
    if (!omusic.load_widescreen_map(config.data.res_path))
        std::cout << "Unable to load widescreen tilemaps" << std::endl;

    // Initialize SDL Video
    config.set_fps(config.video.fps);
    if (!video.init(&roms, &config.video))
        quit_func(1);

    // Initialize SDL Audio
    audio.init();

    state = config.menu.enabled ? STATE_INIT_MENU : STATE_INIT_GAME;

    // Initalize SDL Controls
    input.init(config.controls.pad_id,
               config.controls.keyconfig, config.controls.padconfig, 
               config.controls.analog,    config.controls.axis, config.controls.invert, config.controls.asettings);

    if (config.controls.haptic) 
        config.controls.haptic = forcefeedback::init(config.controls.max_force, config.controls.min_force, config.controls.force_duration);
        
    // Populate menus
    menu = new Menu();
    menu->populate();
    main_loop();  // Loop until we quit the app

    // Never Reached
    return 0;
}
