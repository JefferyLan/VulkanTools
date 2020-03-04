/*
 * Copyright (c) 2020 Valve Corporation
 * Copyright (c) 2020 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * The vkConfig2 program monitors and adjusts the Vulkan configuration
 * environment. These settings are wrapped in this class, which serves
 * as the "model" of the system.
 *
 * Author: Richard S. Wright Jr. <richard@lunarg.com>
 */

#ifdef _WIN32
#include <windows.h>
#endif

#include <QDir>
#include <QSettings>
#include <QTextStream>

#include "vulkanconfiguration.h"

// I am purposly not flagging these as explicit or implicit as this can be parsed from the location
// and future updates to layer locations will only require a smaller change.
#ifdef _WIN32
const int nSearchPaths = 6;
const char *szSearchPaths[nSearchPaths] = {
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Khronos\\Vulkan\\ExplicitLayers",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers",
        "HKEY_CURRENT_USER\\SOFTWARE\\Khronos\\Vulkan\\ExplicitLayers",
        "HKEY_CURRENT_USER\\SOFTWARE\\Khronos\\Vulkan\\ImplicitLayers",
        "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\...\\VulkanExplicitLayers",
        "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\Class\\...\\VulkanImplicitLayers" };
#else
const int nSearchPaths = 10;
const QString szSearchPaths[nSearchPaths] = {
        "/usr/local/etc/vulkan/explicit_layer.d",            // Not used on macOS, okay to just ignore
        "/usr/local/etc/vulkan/implicit_layer.d",            // Not used on macOS, okay to just ignore
        "/usr/local/share/vulkan/explicit_layer.d",
        "/usr/local/share/vulkan/implicit_layer.d",
        "/etc/vulkan/explicit_layer.d",
        "/etc/vulkan/implicit_layer.d",
        "/usr/share/vulkan/explicit_layer.d",
        "/usr/share/vulkan/implicit_layer.d",
        "$HOME/.local/share/vulkan/explicit_layer.d",
        "$HOME/.local/share/vulkan/implicit_layer.d"
};
#endif


// Single pointer to singleton configuration object
CVulkanConfiguration* CVulkanConfiguration::pMe = nullptr;



CVulkanConfiguration::CVulkanConfiguration()
    {
    implicitLayers.reserve(10);
    explicitLayers.reserve(10);
    bLogStdout = false;

/*    CProfileDef *pProfile = new CProfileDef();
    pProfile->profileName = "No Profile Active";
    profileList.push_back(pProfile);

    pProfile = new CProfileDef();
    pProfile->profileName = "Performance Tuning";
    profileList.push_back(pProfile);

    pProfile = new CProfileDef();
    pProfile->profileName = "API Usage Validation";
    profileList.push_back(pProfile);

    pProfile = new CProfileDef();
    pProfile->profileName = "Synchronization and Best Practices";
    profileList.push_back(pProfile);

    pProfile = new CProfileDef;
    pProfile->profileName = "Best Practices and Validation";
    profileList.push_back(pProfile);
*/
    // Default, no active profile
    nActiveProfile = -1;

    loadAdditionalSearchPaths();
    }


CVulkanConfiguration::~CVulkanConfiguration()
    {
    clearLists();
    qDeleteAll(profileList.begin(), profileList.end());
    profileList.clear();
    }

void CVulkanConfiguration::clearLists(void)
    {
    qDeleteAll(implicitLayers.begin(), implicitLayers.end());
    implicitLayers.clear();
    qDeleteAll(explicitLayers.begin(), explicitLayers.end());
    explicitLayers.clear();
    qDeleteAll(customLayers.begin(), customLayers.end());
    customLayers.clear();
    }


///////////////////////////////////////////////////////////////////////////////
/// This is for the local application settings, not the system Vulkan settings
void CVulkanConfiguration::LoadAppSettings(void)
    {
    // Load the launch app name from the last session
    QSettings settings;
    qsLaunchApplicationWPath = settings.value(VKCONFIG_KEY_LAUNCHAPP).toString();
    qsLaunchApplicatinArgs = settings.value(VKCONFIG_KEY_LAUNCHAPP_ARGS).toString();
    qsLaunchApplicationWorkingDir = settings.value(VKCONFIG_KEY_LAUNCHAPP_CWD).toString();
    qsLogFileWPath = settings.value(VKCONFIG_KEY_LOGFILE).toString();
    bLogStdout = settings.value(VKCONFIG_KEY_LOGSTDOUT).toBool();
    }


///////////////////////////////////////////////////////////////////////////////
/// This is for the local application settings, not the system Vulkan settings
void CVulkanConfiguration::SaveAppSettings(void)
    {
    QSettings settings;
    settings.setValue(VKCONFIG_KEY_LAUNCHAPP, qsLaunchApplicationWPath);
    settings.setValue(VKCONFIG_KEY_LAUNCHAPP_ARGS, qsLaunchApplicatinArgs);
    settings.setValue(VKCONFIG_KEY_LAUNCHAPP_CWD, qsLaunchApplicationWorkingDir);
    settings.setValue(VKCONFIG_KEY_LOGFILE, qsLogFileWPath);
    settings.setValue(VKCONFIG_KEY_LOGSTDOUT, bLogStdout);
    }


/////////////////////////////////////////////////////////////////////////////
/// \brief CVulkanConfiguration::loadAdditionalSearchPaths
/// We may have additional paths where we want to search for layers.
/// Load the list of paths here.
void CVulkanConfiguration::loadAdditionalSearchPaths(void)
    {
    // If the file doesn't exist, then the count is zero...
    nAdditionalSearchPathCount = 0;
    QFile file(VKCONFIG_CUSTOM_LAYER_PATHS);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream stream(&file);

    // Okay, there must be data...
    nAdditionalSearchPathCount = 0;

    while(!stream.atEnd()) {
        additionalSearchPaths << stream.readLine();
        nAdditionalSearchPathCount++;
        }

    file.close();   // Just to be explicit, should close anyway when it goes out of scope
    }


/////////////////////////////////////////////////////////////////////////////
/// \brief saveAdditionalSearchPaths
/// We may have additional paths where we want to search for layers.
/// Save the list of paths here.
void CVulkanConfiguration::saveAdditionalSearchPaths(void)
    {
    if(nAdditionalSearchPathCount == 0)
        return;

    QFile file(VKCONFIG_CUSTOM_LAYER_PATHS);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    // Okay, there must be data...
    QTextStream output(&file);
    for(uint32_t i = 0; i < nAdditionalSearchPathCount; i++)
        output << additionalSearchPaths[i] << "\n";

    file.close();   // Just to be explicit, should close anyway when it goes out of scope
    }



///////////////////////////////////////////////////////////////////////////////
void CVulkanConfiguration::reLoadLayerConfiguration(void)
{
    // This is called initially, but also when custom search paths are set, so
    // we need to clear out the old data and just do a clean refresh
    clearLists();


#ifdef _WIN32
    for(int i = 0; i < nSearchPaths; i++) {
        wchar_t keyName[128];
        memset(keyName, 0, 128*sizeof(wchar_t));
        QString qsFullRegistryPath =   "SOFTWARE\\Khronos\\Vulkan\\ExplicitLayers";//  szSearchPaths[i];
        qsFullRegistryPath.toWCharArray(keyName);
        HKEY hKey;

    LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyName, 0, KEY_READ, &hKey);
    bool bExistsAndSuccess (lRes == ERROR_SUCCESS);
    bool bDoesNotExistsSpecifically (lRes == ERROR_FILE_NOT_FOUND);

    std::wstring strValueOfBinDir;
    std::wstring strKeyDefaultValue;

    //    strValue = strDefaultValue;
 /*   WCHAR szBuffer[512];
    DWORD dwBufferSize = sizeof(szBuffer);
    ULONG nError;
    nError = RegQueryValueExW(hKey, strValueName.c_str(), 0, NULL, (LPBYTE)szBuffer, &dwBufferSize);
    if (ERROR_SUCCESS == nError)
    {
        strValue = szBuffer;
    }
    */
//    GetStringRegKey(hKey, L"BinDir", strValueOfBinDir, L"bad");
//    GetStringRegKey(hKey, L"", strKeyDefaultValue, L"bad");


 //       CLayerFile* pLayerFile = new CLayerFile();
 //       pLayerFile->readLayerKeys(szSearchPaths[i], LAYER_TYPE_EXPLICIT);

    }


#endif

#ifndef _WIN32  // All the non-Windows layer locations

    // Standard layer paths
    for(uint32_t i = 0; i < nSearchPaths; i++) {
        // Does the path exist
        QDir dir(szSearchPaths[i]);
        if(!dir.exists())
            continue;

        TLayerType type = (szSearchPaths[i].contains("implicit")) ? LAYER_TYPE_IMPLICIT : LAYER_TYPE_EXPLICIT;
        if(type == LAYER_TYPE_IMPLICIT)
            loadLayersFromPath(szSearchPaths[i], implicitLayers, type);
        else
            loadLayersFromPath(szSearchPaths[i], explicitLayers, type);
        }

    // Any custom paths? All layers from all paths are appended together here
    for(uint32_t i = 0; i < nAdditionalSearchPathCount; i++)
        loadLayersFromPath(additionalSearchPaths[i], customLayers, LAYER_TYPE_CUSTOM);

#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
/// \brief CVulkanConfiguration::loadLayersFromPath
/// \param szPath
/// \param layerList
/// Search a folder and load up all the layers found there.
void CVulkanConfiguration::loadLayersFromPath(const QString &qsPath, QVector<CLayerFile *>& layerList, TLayerType type)
    {
    // Does the path exist
    QDir dir(qsPath);
    if(!dir.exists())
        return;

    // Path exists, let's loop through all the .json files
    QFileInfoList fileList = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
    for(int iFile = 0; iFile < fileList.size(); iFile++) {
        CLayerFile *pLayerFile = new CLayerFile();
        if(pLayerFile->readLayerFile(fileList[iFile].filePath(), type)) {
            // Look for duplicates - Path name AND name must be the same TBD
            for(int i = 0; i < layerList.size(); i++)
                if(layerList[i]->library_path == pLayerFile->library_path &&
                        layerList[i]->name == pLayerFile->name) {
                    delete pLayerFile;
                    pLayerFile = nullptr;
                    break;
                    }

            if(pLayerFile != nullptr)
                layerList.push_back(pLayerFile);
            }
        }
    }



void CVulkanConfiguration::loadProfiles(void)
    {



    }



void CVulkanConfiguration::saveProfiles(void)
    {



    }
