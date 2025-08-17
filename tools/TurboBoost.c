#include <stdio.h>
#include <string.h>

#include <windows.h>
#include <powrprof.h>
#pragma comment (lib, "powrprof")

static DWORD TurboBoostGet(void)
{
    GUID* scheme;
	PowerGetActiveScheme(NULL, &scheme);

    DWORD mode;
    DWORD size = sizeof(mode);
    DWORD result = PowerReadACValue(NULL, scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_PERF_BOOST_MODE, NULL, (void*)&mode, &size);

    LocalFree(scheme);

    return result == ERROR_SUCCESS ? mode : (DWORD)(~0U);
}

static BOOL TurboBoostEnable(void)
{
    GUID* scheme;
	PowerGetActiveScheme(NULL, &scheme);

    DWORD result = PowerWriteACValueIndex(NULL, scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_PERF_BOOST_MODE, PROCESSOR_PERF_BOOST_MODE_ENABLED);
    if (result == ERROR_SUCCESS) PowerSetActiveScheme(NULL, scheme);

    LocalFree(scheme);
    return result == ERROR_SUCCESS;
}

static BOOL TurboBoostDisable(void)
{
    GUID* scheme;
	PowerGetActiveScheme(NULL, &scheme);

    DWORD result = PowerWriteACValueIndex(NULL, scheme, &GUID_PROCESSOR_SETTINGS_SUBGROUP, &GUID_PROCESSOR_PERF_BOOST_MODE, PROCESSOR_PERF_BOOST_MODE_DISABLED);
    if (result == ERROR_SUCCESS) PowerSetActiveScheme(NULL, scheme);

    LocalFree(scheme);
    return result == ERROR_SUCCESS;
}

int main(int argc, char* argv[])
{
    DWORD mode = TurboBoostGet();

    printf("boost mode = ");

    switch (mode)
    {
    break; case PROCESSOR_PERF_BOOST_MODE_DISABLED:                           printf("disabled\n");
    break; case PROCESSOR_PERF_BOOST_MODE_ENABLED:                            printf("enabled\n");
    break; case PROCESSOR_PERF_BOOST_MODE_AGGRESSIVE:                         printf("aggressive\n");
    break; case PROCESSOR_PERF_BOOST_MODE_EFFICIENT_ENABLED:                  printf("efficient enabled\n");
    break; case PROCESSOR_PERF_BOOST_MODE_EFFICIENT_AGGRESSIVE:               printf("efficient aggressive\n");
    break; case PROCESSOR_PERF_BOOST_MODE_AGGRESSIVE_AT_GUARANTEED:           printf("aggressive at guaranteed\n");
    break; case PROCESSOR_PERF_BOOST_MODE_EFFICIENT_AGGRESSIVE_AT_GUARANTEED: printf("efficient agressive at guaranteed\n");
    break; default:                                                           printf("UNKNOWN\n");
    }

    if (argc > 1 && !strcmp(argv[1], "disable"))
    {
        printf("setting mode to DISABLED -> %s\n", TurboBoostDisable() ? "OK" : "ERROR");
    }
    else if (argc > 1 && !strcmp(argv[1], "enable"))
    {
        printf("setting mode to ENABLED -> %s\n", TurboBoostEnable() ? "OK" : "ERROR");
    }
}
