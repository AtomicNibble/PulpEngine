#pragma once

#ifndef X_LVL_SETTINGS_H_
#define X_LVL_SETTINGS_H_

X_NAMESPACE_BEGIN(level)

struct Settings
{
    Settings();

    bool noPatches;
};

extern Settings gSettings;

X_NAMESPACE_END

#endif // !X_LVL_SETTINGS_H_