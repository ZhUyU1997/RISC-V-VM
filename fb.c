#include <pthread.h>
#include <SDL2/SDL.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

static SDL_Window *gWindow = NULL;
static SDL_Renderer *gRender = NULL;
static SDL_Texture *gTexture = NULL;

static unsigned int *frambuffer_fb;
static unsigned int frambuffer_len;

static void frambuffer_init()
{
    SDL_Init(SDL_INIT_VIDEO);
    gWindow = SDL_CreateWindow("SDL", 100, 100, 640, 480, SDL_WINDOW_SHOWN);
    gRender = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    gTexture = SDL_CreateTexture(gRender, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);
}

static void frambuffer_close()
{
    SDL_DestroyRenderer(gRender);
    SDL_DestroyTexture(gTexture);
    SDL_DestroyWindow(gWindow);
    SDL_Quit();
}

void *framebuffer_function(void *ptr)
{
    void *pix;
    int pitch;
    SDL_PixelFormat *format;
    frambuffer_init();

    while (1)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond,&mutex);
        SDL_LockTexture(gTexture, NULL, &pix, &pitch);
        // //为了生成颜色,使用rgba8888的格式
        // format = SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888);
        // Uint32 color = SDL_MapRGBA(format, 0xff, 0, 0, 0xff / 3);
        for (int i = 0; i < frambuffer_len / 4; i++)
            ((Uint32 *)pix)[i] = frambuffer_fb[i];
        SDL_UnlockTexture(gTexture);

        SDL_RenderCopy(gRender, gTexture, NULL, NULL);
        SDL_RenderPresent(gRender);
        pthread_mutex_unlock(&mutex);
    }

    SDL_Delay(5 * 1000);
    frambuffer_close();
}

void framebuffer_present(unsigned int *fb, unsigned int len)
{
    frambuffer_fb = fb;
    frambuffer_len= len;
    pthread_mutex_lock(&mutex);
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
}