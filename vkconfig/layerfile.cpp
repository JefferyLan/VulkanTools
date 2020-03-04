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
 * Author: Richard S. Wright Jr. <richard@lunarg.com>
 *
 * Layer files are JSON documents, so a layer file object is derived
 * from QJsonDocument and is given several useful utility access methods
 * for querying and manipulating layer .json files.
 *
 */

#include "layerfile.h"

#include <QFile>
#include <QMessageBox>


CLayerFile::CLayerFile()
{

}

CLayerFile::~CLayerFile()
    {
    qDeleteAll(layerSettings.begin(), layerSettings.end());
    layerSettings.clear();
    }

///////////////////////////////////////////////////////////////////////////////
/// \brief CLayerFile::readLayerFile
/// \param qsFullPathToFile - Fully qualified path to the layer json file.
/// \return true on success, false on failure.
/// Reports errors via a message box. This might be a bad idea?
/// //////////////////////////////////////////////////////////////////////////
bool CLayerFile::readLayerFile(QString qsFullPathToFile, TLayerType layerKind)
{
    layerType = layerKind;           // Set layer type, no way to know this from the json file

    // Open the file, should be text. Read it into a
    // temporary string.
    QFile file(qsFullPathToFile);
     if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
         return false;
         QMessageBox msgBox;
         msgBox.setText(tr("Could not open layer file"));
         msgBox.exec();
         return false;
        }

     QString jsonText = file.readAll();
     file.close();

     //////////////////////////////////////////////////////
     // Convert the text to a JSON document & validate it.
     // It does need to be a valid json formatted file.
     QJsonDocument jsonDoc;
     QJsonParseError parseError;
     jsonDoc = QJsonDocument::fromJson(jsonText.toUtf8(), &parseError);
    if(parseError.error != QJsonParseError::NoError) {
         QMessageBox msgBox;
         msgBox.setText(parseError.errorString());
         msgBox.exec();
        return false;
        }

    // Make sure it's not empty
    if(jsonDoc.isNull() || jsonDoc.isEmpty()) {
        QMessageBox msgBox;
        msgBox.setText(tr("Json document is empty!"));
        msgBox.exec();
        return false;
        }

    // Populate key items about the layer
    QJsonObject jsonObject = jsonDoc.object();
    QJsonValue  jsonValue = jsonObject.value("file_format_version");
    file_format_version = jsonValue.toString();

    QJsonValue layerValue = jsonObject.value("layer");
    QJsonObject layerObject = layerValue.toObject();

    jsonValue = layerObject.value("name");
    name = jsonValue.toString();

    jsonValue = layerObject.value("type");
    type = jsonValue.toString();

    jsonValue = layerObject.value("library_path");
    library_path = jsonValue.toString();

    jsonValue = layerObject.value("api_version");
    api_version = jsonValue.toString();

    jsonValue = layerObject.value("implementation_version");
    implementation_version = jsonValue.toString();

    jsonValue = layerObject.value("description");
    description = jsonValue.toString();

    readLayerSettingsSchema();

    // The layer file is loaded
    return true;
}

////////////////////////////////////////////////////////////////////////////
/// \brief CLayerFile::readLayerSettingsSchema
/// \return
/// TBD... this is the layers settings. Must be populated from somewhere,
/// or queried directly from the layer DLL.
bool CLayerFile::readLayerSettingsSchema(void)
    {
    TLayerSettings *pSettings = new TLayerSettings;
    pSettings->settingsName = "Fake Setting";
    pSettings->settingsDesc = "This is a fake layer setting, to be figured out later";
    pSettings->settingsType = LAYER_SETTINGS_STRING;
    pSettings->settingsValue = "Some Setting String";
    layerSettings.push_back(pSettings);


    pSettings = new TLayerSettings;
    pSettings->settingsName = "ANOTHER Fake Setting";
    pSettings->settingsDesc = "This is ANOTHER fake layer setting, to be figured out later";
    pSettings->settingsType = LAYER_SETTINGS_STRING;
    pSettings->settingsValue = "Some OTHER Setting String";
    layerSettings.push_back(pSettings);

    return true;
    }
