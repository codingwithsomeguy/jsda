#include <jerryscript.h>
#include <SDL.h>


const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

// TODO: get rid of this, manage window with the JS lifecycle
SDL_Window *window = NULL;


static jerry_value_t drawPixelHandler(
                const jerry_call_info_t *call_info_p,
                const jerry_value_t arguments[],
                const jerry_length_t argument_count) {

    if (argument_count == 3) {
        uint32_t x = jerry_value_as_uint32(arguments[0]);
        uint32_t y = jerry_value_as_uint32(arguments[1]);
        uint32_t urgb = jerry_value_as_uint32(arguments[2]);

        if (window == NULL) {
            SDL_Log("drawPixelHandler: SDL Window was undefined, ignoring draw");
            return jerry_create_undefined();
        }

        if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT) {
            SDL_Log("drawPixelHandler: ignoring draw past screen bounds");
            return jerry_create_undefined();
        }

        SDL_Surface *surface = SDL_GetWindowSurface(window);
        if (surface == NULL) {
            SDL_Log("Surface Error: %s", SDL_GetError());
            return jerry_create_undefined();
        }

        SDL_LockSurface(surface);
        // TODO: get the pixel format
        const Uint8 pixelSize = (Uint8)(surface->pitch / surface->w);
        Uint8 *pixel = surface->pixels + (y * surface->pitch) + (x * sizeof(Uint8) * pixelSize);
        pixel[0] = (urgb & 0x00FF0000) >> 16;
        pixel[1] = (urgb & 0x0000FF00) >> 8;
        pixel[2] = (urgb & 0x000000FF);

        SDL_UnlockSurface(surface);
        SDL_UpdateWindowSurface(window);
    } else {
        SDL_Log("drawPixelHandler: not enough args %d", argument_count);
    }

    return jerry_create_undefined();
}

static jerry_value_t sleepHandler(
                const jerry_call_info_t *call_info_p,
                const jerry_value_t arguments[],
                const jerry_length_t argument_count) {
    //SDL_Log("sleepHandler: invoked");
    if (argument_count == 1) {
        uint32_t t = jerry_value_as_uint32(arguments[0]);
        SDL_Delay(t);
    } else {
        SDL_Log("sleepHandler: ignoring empty sleep time");
    }
    return jerry_create_undefined();
}

int registerAddons(void) {
    jerry_value_t globalObject = jerry_get_global_object();
    jerry_value_t propertyNameDrawPixel = jerry_create_string((const jerry_char_t *) "drawPixel");
    jerry_value_t propertyValueFunc = jerry_create_external_function(drawPixelHandler);
    jerry_value_t setResult = jerry_set_property(globalObject, propertyNameDrawPixel, propertyValueFunc);
    if (jerry_value_is_error(setResult)) {
        SDL_Log("couldn't add the drawPixel obj");
        return 1;
    }

    jerry_release_value(setResult);
    jerry_release_value(propertyValueFunc);
    jerry_release_value(propertyNameDrawPixel);
    jerry_release_value(globalObject);


    globalObject = jerry_get_global_object();
    jerry_value_t propertyNameSleep = jerry_create_string((const jerry_char_t *) "sleep");
    propertyValueFunc = jerry_create_external_function(sleepHandler);
    setResult = jerry_set_property(globalObject, propertyNameSleep, propertyValueFunc);
    if (jerry_value_is_error(setResult)) {
        SDL_Log("couldn't add the sleep obj");
        return 1;
    }

    jerry_release_value(setResult);
    jerry_release_value(propertyValueFunc);
    jerry_release_value(propertyNameSleep);
    jerry_release_value(globalObject);

    return 0;
}

// TODO: properly handle cleanup on failures
// TODO: add SDL input
// TODO: move screen init to a custom function
// TODO: remove the screen/window global
// TODO: add GL
// TODO: draw on the renderer or use a double-buffer
int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow(
        "JSDA Graphics",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        0);
        //SDL_WINDOW_OPENGL);
    if (window == NULL) {
        SDL_Log("Win Error: %s", SDL_GetError());
        return 1;
    }

    jerry_init(JERRY_INIT_EMPTY);

    if (registerAddons() != 0) {
        SDL_Log("failed to registerAddons");
        return 1;
    }


    // trivial load file
    FILE *in = fopen("jsdatest.js", "rb");
    fseek(in, 0, SEEK_END);
    // naughty
    long endPos = ftell(in);
    const jerry_length_t script_size = endPos;
    rewind(in);
    // naughty
    jerry_char_t *script = malloc(endPos + 1);
    // naughty - try one read
    fread(script, 1, endPos, in);
    fclose(in);
    script[endPos] = '\0';

    jerry_value_t parsedCode = jerry_parse(script, script_size, NULL);
    if (!jerry_value_is_error(parsedCode)) {
        jerry_value_t ret_value = jerry_run(parsedCode);
        jerry_release_value(ret_value);
    }

    jerry_release_value(parsedCode);
    jerry_cleanup();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
