//@ SPDX-FileCopyrightText: 2024-present roundedrectangle
//@ SPDX-License-Identifier: GPL-3.0-or-later

#include "callaudio.h"
#include <QAudioDeviceInfo>

#include <pulse/pulseaudio.h>

#define DEBUG_MODULE CallAudio
#include "debuglog.h"

namespace {
    const QString SINK_PRIMARY("sink.primary");
}

QString CallAudio::getOutputDeviceName() {
    // QAudioDeviceInfo::defaultOutputDevice().deviceName() is "sink.null", so we check for other sinks ourselves
    // The preferred order of sinks is: sink.deep_buffer, sink.primary

    bool primaryFound = false;

    for (const QAudioDeviceInfo &info : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        const QString name = info.deviceName();

        if (name == "sink.deep_buffer")
            return name;
        else if (name == SINK_PRIMARY)
            primaryFound = true;
    }

    return primaryFound ? SINK_PRIMARY : QAudioDeviceInfo::defaultOutputDevice().deviceName();
}

QString CallAudio::getInputDeviceName() {
    // QAudioDeviceInfo::defaultInputDevice().deviceName() is "source.null", so we check for other sources ourselves
    // The preferred source is source.droid

    for (const QAudioDeviceInfo &info : QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        const QString name = info.deviceName();
        if (name == "source.droid")
            return name;
    }

    return QAudioDeviceInfo::defaultInputDevice().deviceName();
}

static void successCallback(pa_context *, int, void *) {}

void CallAudio::toggleSpeakerphone(const QString &deviceName, bool enabled) {
    pa_mainloop *mainloop = pa_mainloop_new();
    if (!mainloop) {
        WARN("Unable to create pa mainloop");
        return;
    }

    pa_mainloop_api *api = pa_mainloop_get_api(mainloop);
    pa_context *context = pa_context_new(api, "yaqtlib_CallAudio");
    if (!context) {
        WARN("Unable to create pa context");
        pa_mainloop_free(mainloop);
        return;
    }

    if (pa_context_connect(context, nullptr, PA_CONTEXT_NOAUTOSPAWN, nullptr) < 0) {
        WARN("Unable to connect to pa server");
        pa_context_unref(context);
        pa_mainloop_free(mainloop);
        return;
    }

    // Wait for the context to be connected
    pa_context_state_t state;
    while ((state = pa_context_get_state(context)) != PA_CONTEXT_READY) {
        if (!PA_CONTEXT_IS_GOOD(state)) {
            pa_context_disconnect(context);
            pa_context_unref(context);
            pa_mainloop_free(mainloop);
            return;
        }
        pa_mainloop_iterate(mainloop, 1, nullptr);
    }

    pa_operation *op = pa_context_set_sink_port_by_name(
        context,
        deviceName.toLatin1().constData(),
        enabled ? "output-speaker" : "output-earpiece",
        successCallback,
        nullptr
    );

    if (op) { // Wait for the port to be set
        while (pa_operation_get_state(op) == PA_OPERATION_RUNNING)
            pa_mainloop_iterate(mainloop, 1, nullptr);

        pa_operation_unref(op);
    }

    pa_context_disconnect(context);
    pa_context_unref(context);
    pa_mainloop_free(mainloop);
}

void CallAudio::toggleSpeakerphone(bool enabled) {
    // FIXME: this was not tested
    toggleSpeakerphone(getOutputDeviceName(), enabled);
}
