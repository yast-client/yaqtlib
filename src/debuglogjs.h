#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QLoggingCategory>

#ifndef JS_DEBUG_ROOT_MODULE
#define JS_DEBUG_ROOT_MODULE "yaqtlib.JS"
#endif

class DebugLogJS : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
public:

    DebugLogJS(QObject* parent = Q_NULLPTR) : QObject(parent), category(JS_DEBUG_ROOT_MODULE) {
        enabled = category.isDebugEnabled();
    }
    static QObject* createSingleton(QQmlEngine*, QJSEngine*) { return new DebugLogJS(); }
    bool isEnabled() const { return enabled; }
    void setEnabled(bool value) {
        if (enabled != value) {
            enabled = value;
            Q_EMIT enabledChanged();
        }
    }
Q_SIGNALS:
    void enabledChanged();
private:
    bool enabled;
    const QLoggingCategory category;
};
