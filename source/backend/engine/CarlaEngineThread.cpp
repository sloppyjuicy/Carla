/*
 * Carla Plugin Host
 * Copyright (C) 2011-2014 Filipe Coelho <falktx@falktx.com>
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

#include "CarlaEngine.hpp"
#include "CarlaEngineThread.hpp"
#include "CarlaPlugin.hpp"

CARLA_BACKEND_START_NAMESPACE

// -----------------------------------------------------------------------

CarlaEngineThread::CarlaEngineThread(CarlaEngine* const engine)
    : CarlaThread("CarlaEngineThread"),
      fEngine(engine)
{
    CARLA_ASSERT(engine != nullptr);
    carla_debug("CarlaEngineThread::CarlaEngineThread(%p)", engine);
}

CarlaEngineThread::~CarlaEngineThread()
{
    carla_debug("CarlaEngineThread::~CarlaEngineThread()");
}

// -----------------------------------------------------------------------

void CarlaEngineThread::run()
{
    CARLA_SAFE_ASSERT_RETURN(fEngine != nullptr,);
    CARLA_ASSERT(fEngine->isRunning());
    carla_debug("CarlaEngineThread::run()");

    bool oscRegisted, needsSingleThread;
    float value;

    for (; fEngine->isRunning() && ! shouldExit();)
    {
#ifdef BUILD_BRIDGE
        oscRegisted = fEngine->isOscBridgeRegistered();
#else
        oscRegisted = fEngine->isOscControlRegistered();
#endif

        for (uint i=0, count = fEngine->getCurrentPluginCount(); i < count; ++i)
        {
            CarlaPlugin* const plugin(fEngine->getPluginUnchecked(i));

            CARLA_SAFE_ASSERT_CONTINUE(plugin != nullptr && plugin->isEnabled());
            CARLA_SAFE_ASSERT_UINT2(i == plugin->getId(), i, plugin->getId());

            needsSingleThread = (plugin->getHints() & PLUGIN_NEEDS_SINGLE_THREAD);

            // -----------------------------------------------------------
            // Process postponed events

            if (oscRegisted || ! needsSingleThread)
            {
                if (! needsSingleThread)
                    plugin->postRtEventsRun();

                // -------------------------------------------------------
                // Update parameter outputs

                for (uint32_t j=0, pcount=plugin->getParameterCount(); j < pcount; ++j)
                {
                    if (! plugin->isParameterOutput(j))
                        continue;

                    value = plugin->getParameterValue(j);

                    // Update UI
                    if (! needsSingleThread)
                        plugin->uiParameterChange(j, value);

                    // Update OSC engine client
                    if (oscRegisted)
                    {
#ifdef BUILD_BRIDGE
                        fEngine->oscSend_bridge_parameter_value(j, value);
#else
                        fEngine->oscSend_control_set_parameter_value(i, static_cast<int32_t>(j), value);
#endif
                    }
                }

#ifndef BUILD_BRIDGE
                // ---------------------------------------------------
                // Update OSC control client peaks

                if (oscRegisted)
                    fEngine->oscSend_control_set_peaks(i);
#endif
            }
        }

        fEngine->idleOsc();
        carla_msleep(oscRegisted ? 30 : 50);
    }
}

// -----------------------------------------------------------------------

CARLA_BACKEND_END_NAMESPACE
