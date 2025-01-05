#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include <SDL2/SDL.h>

/* Steamworks interface versions */

#define CHEATS_STEAMCLIENT "SteamClient017"
#define CHEATS_STEAMUSERSTATS "STEAMUSERSTATS_INTERFACE_VERSION011"
/* Shared object file name */

#if defined(_WIN32)
#define STEAM_LIBRARY "steam_api64.dll"
#elif defined(__APPLE__)
#define STEAM_LIBRARY "libsteam_api.dylib"
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__HAIKU__) || defined(__DragonFly__)
#define STEAM_LIBRARY "libsteam_api.so"
#else
#error STEAM_LIBRARY: Unrecognized platform!
#endif

/* DLL, Entry Points */

struct ISteamClient;
struct ISteamUserStats;
struct CallbackMsg_t
{
    int32_t m_hSteamUser;
    int32_t m_iCallback;
    uint8_t* m_pubParam;
    int32_t m_cubParam;
};
struct SteamAPICallCompleted_t
{
    uint64_t m_hAsyncCall;
    int32_t m_iCallback;
    uint32_t m_cubParam;
};

#define FUNC_LIST \
    FOREACH_FUNC(uint8_t, SteamAPI_Init, (void)) \
    FOREACH_FUNC(void, SteamAPI_Shutdown, (void)) \
    FOREACH_FUNC(void, SteamAPI_RunCallbacks, (void)) \
    FOREACH_FUNC(struct ISteamClient*, SteamInternal_CreateInterface, (const char*)) \
    FOREACH_FUNC(int32_t, SteamAPI_GetHSteamUser, (void)) \
    FOREACH_FUNC(int32_t, SteamAPI_GetHSteamPipe, (void)) \
    FOREACH_FUNC(struct ISteamUserStats*, SteamAPI_ISteamClient_GetISteamUserStats, ( \
        struct ISteamClient*, \
        int32_t, \
        int32_t, \
        const char* \
    )) \
    FOREACH_FUNC(uint32_t, SteamAPI_ISteamUserStats_GetNumAchievements, (struct ISteamUserStats*)) \
    FOREACH_FUNC(const char*, SteamAPI_ISteamUserStats_GetAchievementName, (struct ISteamUserStats*, uint32_t)) \
    FOREACH_FUNC(uint8_t, SteamAPI_ISteamUserStats_RequestCurrentStats, (struct ISteamUserStats*)) \
    FOREACH_FUNC(uint8_t, SteamAPI_ISteamUserStats_StoreStats, (struct ISteamUserStats*)) \
    FOREACH_FUNC(uint8_t, SteamAPI_ISteamUserStats_GetStatInt32, ( \
        struct ISteamUserStats*, \
        const char*, \
        int32_t* \
    )) \
    FOREACH_FUNC(uint8_t, SteamAPI_ISteamUserStats_SetStatInt32, ( \
        struct ISteamUserStats*, \
        const char*, \
        int32_t \
    )) \
    FOREACH_FUNC(uint8_t, SteamAPI_ISteamUserStats_SetAchievement, ( \
        struct ISteamUserStats*, \
        const char* \
    )) \
    FOREACH_FUNC(void, SteamAPI_ManualDispatch_Init, (void)) \
    FOREACH_FUNC(void, SteamAPI_ManualDispatch_RunFrame, (int32_t)) \
    FOREACH_FUNC(uint8_t, SteamAPI_ManualDispatch_GetNextCallback, (int32_t, struct CallbackMsg_t*)) \
    FOREACH_FUNC(void, SteamAPI_ManualDispatch_FreeLastCallback, (int32_t)) \
    FOREACH_FUNC(uint8_t, SteamAPI_ManualDispatch_GetAPICallResult, ( \
        int32_t, \
        uint64_t, \
        void*, \
        int32_t, \
        int32_t, \
        uint8_t* \
    ))


static void* libHandle = NULL;
static struct ISteamUserStats* steamUserStats = NULL;

#define FOREACH_FUNC(rettype, name, params) static rettype (*name) params = NULL;
FUNC_LIST
#undef FOREACH_FUNC

/* Clean up after ourselves... */

static void ClearPointers(void)
{
    SDL_UnloadObject(libHandle);
    libHandle = NULL;
    steamUserStats = NULL;
#define FOREACH_FUNC(rettype, name, params) name = NULL;
    FUNC_LIST
#undef FOREACH_FUNC
}

/* NETWORK API Implementation */

static int32_t steamPipe = 0;

int32_t STEAM_init(void)
{
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__HAIKU__) || defined(__DragonFly__)
    return 0;
#endif
    struct ISteamClient *steamClient;
    int32_t steamUser;

    libHandle = SDL_LoadObject( STEAM_LIBRARY );
    if (!libHandle)
    {
        printf( STEAM_LIBRARY " not found!\n" );
        return 0;
    }

#define FOREACH_FUNC(rettype, name, params) \
    name = (rettype (*) params) (intptr_t) SDL_LoadFunction(libHandle, #name); \
    if (!name) \
    { \
        printf( STEAM_LIBRARY " symbol " #name " not found!\n" ); \
        ClearPointers(); \
        return 0; \
    }
    FUNC_LIST
#undef FOREACH_FUNC

    if (!SteamAPI_Init())
    {
        printf( "Steamworks not initialized!\n" );
        ClearPointers();
        return 0;
    }
    SteamAPI_ManualDispatch_Init();
    steamClient = SteamInternal_CreateInterface( CHEATS_STEAMCLIENT );
    steamUser = SteamAPI_GetHSteamUser();
    steamPipe = SteamAPI_GetHSteamPipe();
    if (!steamClient || !steamUser || !steamPipe)
    {
        SteamAPI_Shutdown();
        printf( CHEATS_STEAMCLIENT " not created!\n" );
        ClearPointers();
        return 0;
    }
    steamUserStats = SteamAPI_ISteamClient_GetISteamUserStats(
        steamClient,
        steamUser,
        steamPipe,
        CHEATS_STEAMUSERSTATS
    );
    if (!steamUserStats)
    {
        SteamAPI_Shutdown();
        printf( CHEATS_STEAMUSERSTATS " not created!\n" );
        ClearPointers();
        return 0;
    }
    SteamAPI_ISteamUserStats_RequestCurrentStats( steamUserStats );

    return 1;
}

void STEAM_shutdown(void)
{
    if (libHandle)
    {
        SteamAPI_Shutdown();
        ClearPointers();
    }
}

void STEAM_update(void)
{
    if ( !libHandle )
    {
        return;
    }

    SteamAPI_ManualDispatch_RunFrame( steamPipe );
    struct CallbackMsg_t callback;
    SDL_zero( callback );
    while ( SteamAPI_ManualDispatch_GetNextCallback(steamPipe, &callback) )
    {
        SteamAPI_ManualDispatch_FreeLastCallback(steamPipe);
    }
}

void STEAM_unlockAchievement(const char *name)
{
    if ( libHandle )
    {
        SteamAPI_ISteamUserStats_SetAchievement(
            steamUserStats,
            name
        );
        SteamAPI_ISteamUserStats_StoreStats(steamUserStats);
    }
}

void *STEAM_loopUpdate(void *idc) // run in thread
{
    while( 1 )
    {
        SDL_Delay( 1000/60 );
        //printf("I am the thread and I am RUNNING!\n");
        STEAM_update();
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    printf("steam_get 🄯 2024 Buggemgames\n\nUse at your own risk!\n", argc);
    pthread_t callbackLoop;
    if( !STEAM_init() )
    {
        return 1;
    }
    pthread_create(&callbackLoop, NULL, STEAM_loopUpdate, NULL);

    uint32_t numAchievements = SteamAPI_ISteamUserStats_GetNumAchievements( steamUserStats );
    for ( uint32_t i = 0; i < numAchievements; ++i )
    {
        const char *achName = SteamAPI_ISteamUserStats_GetAchievementName( steamUserStats, i );
        if( achName )
        {
            printf( "%s\n", achName );
            STEAM_unlockAchievement( achName );
        }
    }
    pthread_exit(NULL);
}

// Kill Bill
int WinMain() {
    char *fakeargv[] = {
        "./steam_get.exe"
    };
    main(1, fakeargv);
}
