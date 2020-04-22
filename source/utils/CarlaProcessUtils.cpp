/*
 * Carla process utils
 * Copyright (C) 2019-2020 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#include "CarlaProcessUtils.hpp"

#ifdef CARLA_OS_LINUX
# include <sys/prctl.h>
#endif

// --------------------------------------------------------------------------------------------------------------------
// process functions

void carla_setProcessName(const char* const name) noexcept
{
    CARLA_SAFE_ASSERT_RETURN(name != nullptr && name[0] != '\0',);

#ifdef CARLA_OS_LINUX
    ::prctl(PR_SET_NAME, name, 0, 0, 0);
#endif
}

void carla_terminateProcessOnParentExit(const bool kill) noexcept
{
#ifdef CARLA_OS_LINUX
    //
    ::prctl(PR_SET_PDEATHSIG, kill ? SIGKILL : SIGTERM);
    // TODO, osx version too, see https://stackoverflow.com/questions/284325/how-to-make-child-process-die-after-parent-exits
#endif

    // maybe unused
    return; (void)kill;
}

// --------------------------------------------------------------------------------------------------------------------
// process utility classes

ScopedAbortCatcher::ScopedAbortCatcher()
{
    s_triggered = false;
#ifndef CARLA_OS_WIN
    s_oldsig = ::setjmp(s_env) == 0
             ? std::signal(SIGABRT, sig_handler)
             : nullptr;
#endif
}

ScopedAbortCatcher::~ScopedAbortCatcher()
{
#ifndef CARLA_OS_WIN
    if (! s_triggered)
        std::signal(SIGABRT, s_oldsig);
#endif
}

bool ScopedAbortCatcher::s_triggered = false;

#ifndef CARLA_OS_WIN
jmp_buf ScopedAbortCatcher::s_env;
sig_t ScopedAbortCatcher::s_oldsig;

void ScopedAbortCatcher::sig_handler(const int signum)
{
    CARLA_SAFE_ASSERT_INT2_RETURN(signum == SIGABRT, signum, SIGABRT,);

    s_triggered = true;
    std::signal(signum, s_oldsig);
    std::longjmp(s_env, 1);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

CarlaSignalRestorer::CarlaSignalRestorer()
{
#ifndef CARLA_OS_WIN
    carla_zeroStructs(sigs, 16);

    for (int i=0; i < 16; ++i)
        ::sigaction(i+1, nullptr, &sigs[i]);
#endif
}

CarlaSignalRestorer::~CarlaSignalRestorer()
{
#ifndef CARLA_OS_WIN
    for (int i=0; i < 16; ++i)
        ::sigaction(i+1, &sigs[i], nullptr);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
