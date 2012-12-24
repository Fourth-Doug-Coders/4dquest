/*
 * JSON Tiled Plugin
 * Copyright 2011, Porfírio José Pereira Ribeiro <porfirioribeiro@gmail.com>
 * Copyright 2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of Tiled.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "bqclientplugin.h"

#include "maptovariantconverter.h"
#include "varianttomapconverter.h"

#include "qjsonparser/json.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

using namespace BQClient;

BQClientPlugin::BQClientPlugin()
{
}

bool BQClientPlugin::write(const Tiled::Map *map, const QString &fileName)
{
    QFile file(fileName);
    QFile jsFile;

    if(fileName.right(5) == ".json") {
        jsFile.setFileName(fileName.left(fileName.length() - 4) + "js");
    }
    else {
        jsFile.setFileName(fileName + ".js");
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text) || !jsFile.open(QIODevice::WriteOnly | QIODevice::Text)){
        mError = tr("Could not open file for writing.");
        return false;
    }

    MapToVariantConverter converter;
    QVariant variant = converter.toVariant(map, QFileInfo(fileName).dir());

    JsonWriter writer;
    writer.setAutoFormatting(true);

    if (!writer.stringify(variant)) {
        // This can only happen due to coding error
        mError = writer.errorString();
        return false;
    }

    QTextStream out(&file);
    QTextStream jsOut(&jsFile);

    out << writer.result();
    jsOut << "var mapData = " << writer.result() << ";";

    out.flush();
    jsOut.flush();

    if (file.error() != QFile::NoError) {
        mError = tr("Error while writing file:\n%1").arg(file.errorString());
        return false;
    }

    return true;
}

QString BQClientPlugin::nameFilter() const
{
    return tr("BrowserQuest client files (*.json)");
}

QString BQClientPlugin::errorString() const
{
    return mError;
}

Q_EXPORT_PLUGIN2(BQClient, BQClientPlugin)
