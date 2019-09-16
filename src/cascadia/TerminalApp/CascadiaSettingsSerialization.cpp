// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include <argb.h>
#include "CascadiaSettings.h"
#include "../../types/inc/utils.hpp"
#include "utils.h"
#include "JsonUtils.h"
#include <appmodel.h>
#include <shlobj.h>

// defaults.h is a file containing the default json settings in a std::string_view
#include "defaults.h"
// userDefault.h is like the above, but with a default template for the user's profiles.json.
#include "userDefaults.h"
// Both defaults.h and userDefaults.h are generated at build time into the
// "Generated Files" directory.

using namespace ::TerminalApp;
using namespace winrt::Microsoft::Terminal::TerminalControl;
using namespace winrt::TerminalApp;
using namespace ::Microsoft::Console;

static constexpr std::wstring_view SettingsFilename{ L"profiles.json" };
static constexpr std::wstring_view UnpackagedSettingsFolderName{ L"Microsoft\\Windows Terminal\\" };

static constexpr std::wstring_view DefaultsFilename{ L"defaults.json" };

static constexpr std::string_view ProfilesKey{ "profiles" };
static constexpr std::string_view KeybindingsKey{ "keybindings" };
static constexpr std::string_view GlobalsKey{ "globals" };
static constexpr std::string_view SchemesKey{ "schemes" };

static constexpr std::string_view Utf8Bom{ u8"\uFEFF" };

// Method Description:
// - Creates a CascadiaSettings from whatever's saved on disk, or instantiates
//      a new one with the default values. If we're running as a packaged app,
//      it will load the settings from our packaged localappdata. If we're
//      running as an unpackaged application, it will read it from the path
//      we've set under localappdata.
// - Loads both the settings from the defaults.json and the user's profiles.json
// Return Value:
// - a unique_ptr containing a new CascadiaSettings object.
std::unique_ptr<CascadiaSettings> CascadiaSettings::LoadAll()
{
    auto resultPtr = LoadDefaults();

    std::optional<std::string> fileData = _ReadUserSettings();
    const bool foundFile = fileData.has_value();
    // Make sure the file isn't totally empty. If it is, we'll treat the file
    // like it doesn't exist at all.
    const bool fileHasData = foundFile && !fileData.value().empty();
    if (fileHasData)
    {
        resultPtr->_LayerJsonString(fileData.value(), false);
    }
    else
    {
        // We didn't find the user settings. We'll need to create a file
        // to use as the user defaults.
        _WriteSettings(UserSettingsJson);
    }

    // If this throws, the app will catch it and use the default settings
    resultPtr->_ValidateSettings();

    return resultPtr;
}

// Function Description:
// - Creates a new CascadiaSettings object initialized with settings from the
//   hardcoded defaults.json.
// Arguments:
// - <none>
// Return Value:
// - a unique_ptr to a CascadiaSettings with the settings from defaults.json
std::unique_ptr<CascadiaSettings> CascadiaSettings::LoadDefaults()
{
    auto resultPtr = std::make_unique<CascadiaSettings>();

    // We already have the defaults in memory, because we stamp them into a
    // header as part of the build process. We don't need to bother with reading
    // them from a file (and the potential that could fail)
    resultPtr->_LayerJsonString(DefaultJson, true);

    return resultPtr;
}

// Method Description:
// - Attempts to read the given data as a string of JSON, parse that JSON, and
//   then layer the settings from that JSON object on our current settings. See
//   CascadiaSettings::LayerJson for detauls on how the layering works.
// - Will ignore leading UTF-8 BOMs.
// - Additionally, will store the parsed JSON in this object, as either our
//   _defaultSettings or our _userSettings, depending on isDefaultSettings.
// Arguments:
// - fileData: the string to parse as JSON data
// - isDefaultSettings: if true, we should store the parsed JSON as our
//   defaultSettings. Otherwise, we'll store the parsed JSON as our user
//   settings.
// Return Value:
// - <none>
void CascadiaSettings::_LayerJsonString(std::string_view fileData, const bool isDefaultSettings)
{
    // Ignore UTF-8 BOM
    auto actualDataStart = fileData.data();
    if (fileData.compare(0, Utf8Bom.size(), Utf8Bom) == 0)
    {
        actualDataStart += Utf8Bom.size();
    }

    std::string errs; // This string will recieve any error text from failing to parse.
    std::unique_ptr<Json::CharReader> reader{ Json::CharReaderBuilder::CharReaderBuilder().newCharReader() };

    // Parse the json data into either our defaults or user settings. We'll keep
    // these original json values around for later, in case we need to parse
    // their raw contents again.
    Json::Value& root = isDefaultSettings ? _defaultSettings : _userSettings;
    // `parse` will return false if it fails.
    if (!reader->parse(actualDataStart, fileData.data() + fileData.size(), &root, &errs))
    {
        // This will be caught by App::_TryLoadSettings, who will display
        // the text to the user.
        throw winrt::hresult_error(WEB_E_INVALID_JSON_STRING, winrt::to_hstring(errs));
    }

    LayerJson(root);
}

// Method Description:
// - Serialize this settings structure, and save it to a file. The location of
//      the file changes depending whether we're running as a packaged
//      application or not.
// Arguments:
// - <none>
// Return Value:
// - <none>
void CascadiaSettings::SaveAll() const
{
    const auto json = ToJson();
    Json::StreamWriterBuilder wbuilder;
    // Use 4 spaces to indent instead of \t
    wbuilder.settings_["indentation"] = "    ";
    const auto serializedString = Json::writeString(wbuilder, json);

    _WriteSettings(serializedString);
}

// Method Description:
// - Serialize this object to a JsonObject.
// Arguments:
// - <none>
// Return Value:
// - a JsonObject which is an equivalent serialization of this object.
Json::Value CascadiaSettings::ToJson() const
{
    Json::Value root;

    Json::Value profilesArray;
    for (const auto& profile : _profiles)
    {
        profilesArray.append(profile.ToJson());
    }

    Json::Value schemesArray;
    const auto& colorSchemes = _globals.GetColorSchemes();
    for (auto& scheme : colorSchemes)
    {
        schemesArray.append(scheme.ToJson());
    }

    root[GlobalsKey.data()] = _globals.ToJson();
    root[ProfilesKey.data()] = profilesArray;
    root[SchemesKey.data()] = schemesArray;

    return root;
}

// Method Description:
// - Create a new instance of this class from a serialized JsonObject.
// Arguments:
// - json: an object which should be a serialization of a CascadiaSettings object.
// Return Value:
// - a new CascadiaSettings instance created from the values in `json`
std::unique_ptr<CascadiaSettings> CascadiaSettings::FromJson(const Json::Value& json)
{
    auto resultPtr = std::make_unique<CascadiaSettings>();
    resultPtr->LayerJson(json);
    return resultPtr;
}

// Method Description:
// - Layer values from the given json object on top of the existing properties
//   of this object. For any keys we're expecting to be able to parse in the
//   given object, we'll parse them and replace our settings with values from
//   the new json object. Properties that _aren't_ in the json object will _not_
//   be replaced.
// Arguments:
// - json: an object which should be a partial serialization of a CascadiaSettings object.
// Return Value:
// <none>
void CascadiaSettings::LayerJson(const Json::Value& json)
{
    if (auto globals{ json[GlobalsKey.data()] })
    {
        if (globals.isObject())
        {
            _globals.LayerJson(globals);
        }
    }
    else
    {
        // If there's no globals key in the root object, then try looking at the
        // root object for those properties instead, to gracefully upgrade.
        // This will attempt to do the legacy keybindings loading too
        _globals.LayerJson(json);
    }

    if (auto schemes{ json[SchemesKey.data()] })
    {
        for (auto schemeJson : schemes)
        {
            if (schemeJson.isObject())
            {
                _LayerOrCreateColorScheme(schemeJson);
            }
        }
    }

    for (auto profileJson : _GetProfilesJsonObject(json))
    {
        if (profileJson.isObject())
        {
            _LayerOrCreateProfile(profileJson);
        }
    }
}

// Method Description:
// - Given a partial json serialization of a Profile object, either layers that
//   json on a matching Profile we already have, or creates a new Profile
//   object from those settings.
// Arguments:
// - json: an object which may be a partial serialization of a Profile object.
// Return Value:
// - <none>
void CascadiaSettings::_LayerOrCreateProfile(const Json::Value& profileJson)
{
    // Layer the json on top of an existing profile, if we have one:
    auto pProfile = _FindMatchingProfile(profileJson);
    if (pProfile)
    {
        pProfile->LayerJson(profileJson);
    }
    else
    {
        auto profile = Profile::FromJson(profileJson);
        _profiles.emplace_back(profile);
    }
}

// Method Description:
// - Finds a profile from our list of profiles that matches the given json
//   object. Uses Profile::ShouldBeLayered to determine if the Json::Value is a
//   match or not. This method should be used to find a profile to layer the
//   given settings upon.
// - Returns nullptr if no such match exists.
// Arguments:
// - json: an object which may be a partial serialization of a Profile object.
// Return Value:
// - a Profile that can be layered with the given json object, iff such a
//   profile exists.
Profile* CascadiaSettings::_FindMatchingProfile(const Json::Value& profileJson)
{
    for (auto& profile : _profiles)
    {
        if (profile.ShouldBeLayered(profileJson))
        {
            // HERE BE DRAGONS: Returning a pointer to a type in the vector is
            // maybe not the _safest_ thing, but we have a mind to make Profile
            // and ColorScheme winrt types in the future, so this will be safer
            // then.
            return &profile;
        }
    }
    return nullptr;
}

// Method Description:
// - Given a partial json serialization of a ColorScheme object, either layers that
//   json on a matching ColorScheme we already have, or creates a new ColorScheme
//   object from those settings.
// Arguments:
// - json: an object which should be a partial serialization of a ColorScheme object.
// Return Value:
// - <none>
void CascadiaSettings::_LayerOrCreateColorScheme(const Json::Value& schemeJson)
{
    // Layer the json on top of an existing profile, if we have one:
    auto pScheme = _FindMatchingColorScheme(schemeJson);
    if (pScheme)
    {
        pScheme->LayerJson(schemeJson);
    }
    else
    {
        auto scheme = ColorScheme::FromJson(schemeJson);
        _globals.GetColorSchemes().emplace_back(scheme);
    }
}

// Method Description:
// - Finds a color scheme from our list of color schemes that matches the given
//   json object. Uses ColorScheme::ShouldBeLayered to determine if the
//   Json::Value is a match or not. This method should be used to find a color
//   scheme to layer the given settings upon.
// - Returns nullptr if no such match exists.
// Arguments:
// - json: an object which should be a partial serialization of a ColorScheme object.
// Return Value:
// - a ColorScheme that can be layered with the given json object, iff such a
//   color scheme exists.
ColorScheme* CascadiaSettings::_FindMatchingColorScheme(const Json::Value& schemeJson)
{
    for (auto& scheme : _globals.GetColorSchemes())
    {
        if (scheme.ShouldBeLayered(schemeJson))
        {
            // HERE BE DRAGONS: Returning a pointer to a type in the vector is
            // maybe not the _safest_ thing, but we have a mind to make Profile
            // and ColorScheme winrt types in the future, so this will be safer
            // then.
            return &scheme;
        }
    }
    return nullptr;
}

// Function Description:
// - Returns true if we're running in a packaged context.
//   If we are, we want to change our settings path slightly.
// Arguments:
// - <none>
// Return Value:
// - true iff we're running in a packaged context.
bool CascadiaSettings::_IsPackaged()
{
    UINT32 length = 0;
    LONG rc = GetCurrentPackageFullName(&length, NULL);
    return rc != APPMODEL_ERROR_NO_PACKAGE;
}

// Method Description:
// - Writes the given content in UTF-8 to our settings file using the Win32 APIS's.
//   Will overwrite any existing content in the file.
// Arguments:
// - content: the given string of content to write to the file.
// Return Value:
// - <none>
//   This can throw an exception if we fail to open the file for writing, or we
//      fail to write the file
void CascadiaSettings::_WriteSettings(const std::string_view content)
{
    auto pathToSettingsFile{ CascadiaSettings::GetSettingsPath() };

    wil::unique_hfile hOut{ CreateFileW(pathToSettingsFile.c_str(),
                                        GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL) };
    if (!hOut)
    {
        THROW_LAST_ERROR();
    }
    THROW_LAST_ERROR_IF(!WriteFile(hOut.get(), content.data(), gsl::narrow<DWORD>(content.size()), 0, 0));
}

// Method Description:
// - Reads the content in UTF-8 encoding of our settings file using the Win32 APIs
// Arguments:
// - <none>
// Return Value:
// - an optional with the content of the file if we were able to open it,
//      otherwise the optional will be empty.
//   If the file exists, but we fail to read it, this can throw an exception
//      from reading the file
std::optional<std::string> CascadiaSettings::_ReadUserSettings()
{
    const auto pathToSettingsFile{ CascadiaSettings::GetSettingsPath() };
    wil::unique_hfile hFile{ CreateFileW(pathToSettingsFile.c_str(),
                                         GENERIC_READ,
                                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                                         nullptr,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         nullptr) };

    if (!hFile)
    {
        // GH#1770 - Now that we're _not_ roaming our settings, do a quick check
        // to see if there's a file in the Roaming App data folder. If there is
        // a file there, but not in the LocalAppData, it's likely the user is
        // upgrading from a version of the terminal from before this change.
        // We'll try moving the file from the Roaming app data folder to the
        // local appdata folder.

        const auto pathToRoamingSettingsFile{ CascadiaSettings::GetSettingsPath(true) };
        wil::unique_hfile hRoamingFile{ CreateFileW(pathToRoamingSettingsFile.c_str(),
                                                    GENERIC_READ,
                                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                                    nullptr,
                                                    OPEN_EXISTING,
                                                    FILE_ATTRIBUTE_NORMAL,
                                                    nullptr) };

        if (hRoamingFile)
        {
            // Close the file handle, move it, and re-open the file in its new location.
            hRoamingFile.reset();

            // Note: We're unsure if this is unsafe. Theoretically it's possible
            // that two instances of the app will try and move the settings file
            // simultaneously. We don't know what might happen in that scenario,
            // but we're also not sure how to safely lock the file to prevent
            // that from ocurring.
            THROW_LAST_ERROR_IF(!MoveFile(pathToRoamingSettingsFile.c_str(),
                                          pathToSettingsFile.c_str()));

            hFile.reset(CreateFileW(pathToSettingsFile.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    nullptr,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    nullptr));

            // hFile shouldn't be INVALID. That's unexpected - We just moved the
            // file, we should be able to open it. Throw the error so we can get
            // some information here.
            THROW_LAST_ERROR_IF(!hFile);
        }
        else
        {
            // If the roaming file didn't exist, and the local file doesn't exist,
            //      that's fine. Just log the error and return nullopt - we'll
            //      create the defaults.
            LOG_LAST_ERROR();
            return std::nullopt;
        }
    }

    return _ReadFile(hFile.get());
}

// Method Description:
// - Reads the content in UTF-8 encoding of the given file using the Win32 APIs
// Arguments:
// - <none>
// Return Value:
// - an optional with the content of the file if we were able to read it. If we
//   fail to read it, this can throw an exception from reading the file
std::optional<std::string> CascadiaSettings::_ReadFile(HANDLE hFile)
{
    // fileSize is in bytes
    const auto fileSize = GetFileSize(hFile, nullptr);
    THROW_LAST_ERROR_IF(fileSize == INVALID_FILE_SIZE);

    auto utf8buffer = std::make_unique<char[]>(fileSize);

    DWORD bytesRead = 0;
    THROW_LAST_ERROR_IF(!ReadFile(hFile, utf8buffer.get(), fileSize, &bytesRead, nullptr));

    // convert buffer to UTF-8 string
    std::string utf8string(utf8buffer.get(), fileSize);

    return { utf8string };
}

// function Description:
// - Returns the full path to the settings file, either within the application
//   package, or in its unpackaged location. This path is under the "Local
//   AppData" folder, so it _doesn't_ roam to other machines.
// - If the application is unpackaged,
//   the file will end up under e.g. C:\Users\admin\AppData\Local\Microsoft\Windows Terminal\profiles.json
// Arguments:
// - <none>
// Return Value:
// - the full path to the settings file
std::wstring CascadiaSettings::GetSettingsPath(const bool useRoamingPath)
{
    wil::unique_cotaskmem_string localAppDataFolder;
    // KF_FLAG_FORCE_APP_DATA_REDIRECTION, when engaged, causes SHGet... to return
    // the new AppModel paths (Packages/xxx/RoamingState, etc.) for standard path requests.
    // Using this flag allows us to avoid Windows.Storage.ApplicationData completely.
    const auto knowFolderId = useRoamingPath ? FOLDERID_RoamingAppData : FOLDERID_LocalAppData;
    if (FAILED(SHGetKnownFolderPath(knowFolderId, KF_FLAG_FORCE_APP_DATA_REDIRECTION, 0, &localAppDataFolder)))
    {
        THROW_LAST_ERROR();
    }

    std::filesystem::path parentDirectoryForSettingsFile{ localAppDataFolder.get() };

    if (!_IsPackaged())
    {
        parentDirectoryForSettingsFile /= UnpackagedSettingsFolderName;
    }

    // Create the directory if it doesn't exist
    std::filesystem::create_directories(parentDirectoryForSettingsFile);

    return parentDirectoryForSettingsFile / SettingsFilename;
}

std::wstring CascadiaSettings::GetDefaultSettingsPath()
{
    // Both of these posts suggest getting the path to the exe, then removing
    // the exe's name to get the package root:
    // * https://blogs.msdn.microsoft.com/appconsult/2017/06/23/accessing-to-the-files-in-the-installation-folder-in-a-desktop-bridge-application/
    // * https://blogs.msdn.microsoft.com/appconsult/2017/03/06/handling-data-in-a-converted-desktop-app-with-the-desktop-bridge/
    //
    // This would break if we ever moved our exe out of the package root.
    // HOWEVER, if we try to look for a defaults.json that's simply in the same
    // directory as the exe, that will work for unpackaged scenarios as well. So
    // let's try that.

    HMODULE hModule = GetModuleHandle(nullptr);
    THROW_LAST_ERROR_IF(hModule == nullptr);

    std::wstring exePathString;
    THROW_IF_FAILED(wil::GetModuleFileNameW(hModule, exePathString));

    const std::filesystem::path exePath{ exePathString };
    const std::filesystem::path rootDir = exePath.parent_path();
    return rootDir / DefaultsFilename;
}

// Function Description:
// - Gets the object in the given JSON object under the "profiles" key. Returns
//   null if there's no "profiles" key.
// Arguments:
// - json: the json object to get the profiles from.
// Return Value:
// - the Json::Value representing the profiles property from the given object
const Json::Value& CascadiaSettings::_GetProfilesJsonObject(const Json::Value& json)
{
    return json[JsonKey(ProfilesKey)];
}
