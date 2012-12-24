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

#include "maptovariantconverter.h"

#include "map.h"
#include "mapobject.h"
#include "objectgroup.h"
#include "properties.h"
#include "tile.h"
#include "tilelayer.h"
#include "tileset.h"
#include "qdebug.h"

using namespace Tiled;
using namespace BQClient;

QVariant MapToVariantConverter::toVariant(const Map *map, const QDir &mapDir)
{
    mMapDir = mapDir;
    mGidMapper.clear();

    mapVariant["height"] = map->height();
    mapVariant["width"] = map->width();
    mapVariant["tilesize"] = map->tileWidth();

    for(uint i = 0; i < map->height() * map->width(); i++) {
        layersOfTiles << 0;
    }

    uint firstGid = 1;
    foreach(Tileset *tileset, map->tilesets()) {
        processTileset(tileset, firstGid);
        mGidMapper.insert(firstGid, tileset);
        firstGid += tileset->tileCount();
    }

    foreach (const Layer *layer, map->layers()) {
        const TileLayer *tileLayer = dynamic_cast<const TileLayer*>(layer);
        const ObjectGroup *objectGroup = dynamic_cast<const ObjectGroup *>(layer);

        if(tileLayer != 0)
            processLayer(tileLayer);
        if(objectGroup != 0)
            processObjectGroup(objectGroup);
    }

    mapVariant["data"] = layersOfTiles;
    mapVariant["collisions"] = collisionTiles;
    mapVariant["doors"] = doors;
    mapVariant["musicAreas"] = musicAreas;
    mapVariant["checkpoints"] = checkpoints;

    return mapVariant;
}

void MapToVariantConverter::processTileset(const Tileset *tileset, const uint firstGid)
{
    if(tileset->name() == "tilesheet") {
        QVariantMap animated;
        QVariantList high;
        uint gid = firstGid;

        for(int i = 0; i < tileset->tileCount(); ++i) {
            const Tile *tile = tileset->tileAt(i);
            const Properties tileProps = tile->properties();
            if(tileProps.contains("length") || tileProps.contains("delay")) {
                animated[QString::number(gid)] = animPropsToVariant(tileProps);
            }
            if(tileProps.contains("v")) {
                high << gid;
            }
            if(tileProps.contains("c")) {
                collisionGids << gid;
            }
            gid++;
        }
        mapVariant["animated"] = animated;
        mapVariant["high"] = high;
    }
}

void MapToVariantConverter::processObjectGroup(const ObjectGroup *objectGroup)
{
    if(objectGroup->name() == "doors") {
        foreach (const MapObject *object, objectGroup->objects()) {
            QVariantMap objectVariant;
            objectVariant["x"] = object->x();
            objectVariant["y"] = object->y();

            objectVariant["p"] = object->type() == "portal" ? 1 : 0;

            Properties::const_iterator it = object->properties().constBegin();
            Properties::const_iterator it_end = object->properties().constEnd();
            for (; it != it_end; ++it) {
                bool isInt;
                int val = it.value().toInt(&isInt);
                if(isInt)
                    objectVariant["t" + it.key()] = val;
                else
                    objectVariant["t" + it.key()] = it.value();
            }

            doors << objectVariant;
        }
    }
    else if(objectGroup->name() == "music") {
        foreach(const MapObject *object, objectGroup->objects()) {
            QVariantMap objectVariant;

            objectVariant["x"] = object->x();
            objectVariant["y"] = object->y();
            objectVariant["w"] = object->width();
            objectVariant["h"] = object->height();
            objectVariant["id"] = object->properties()["id"];
            musicAreas << objectVariant;
        }
    }
    else if(objectGroup->name() == "checkpoints") {
        uint count = 0;

        foreach(const MapObject *object, objectGroup->objects()) {
            QVariantMap objectVariant;

            objectVariant["id"] = ++count;
            objectVariant["x"] = object->x();
            objectVariant["y"] = object->y();
            objectVariant["w"] = object->width();
            objectVariant["h"] = object->height();

            checkpoints << objectVariant;
        }
    }
}

void MapToVariantConverter::processLayer(const TileLayer *tileLayer)
{
    QVariantList specialTiles;
    uint tileIdx = 0;

    // Mega loop through all tiles in the layer
    for(int y = 0; y < tileLayer->height(); ++y) {
        for(int x = 0; x < tileLayer->width(); ++x) {

            // "blocking" and "plateau" are non-graphical layers, which is why they get special treatment
            if(tileLayer->name() == "blocking" || tileLayer->name() == "plateau") {
                if(!tileLayer->cellAt(x, y).isEmpty())
                    specialTiles << (y * tileLayer->width() + x);
            }

            // all other layers -- create 2D array of layer GIDs
            else if(tileLayer->name() != "entities" && tileLayer->isVisible()) {
                int gid = mGidMapper.cellToGid(tileLayer->cellAt(x, y));

                if(gid != 0) {
                    if(layersOfTiles[tileIdx] == 0) {
                        layersOfTiles.replace(tileIdx, gid);
                    }
                    else if(layersOfTiles[tileIdx].type() == QVariant::List) {
                        QVariantList gids = QVariantList(layersOfTiles[tileIdx].toList());
                        gids << gid;
                        layersOfTiles.replace(tileIdx, gids);
                    }
                    else {
                        QVariantList gids = QVariantList();
                        gids << layersOfTiles[tileIdx].toInt() << gid;
                        layersOfTiles.replace(tileIdx, gids);
                    }

                    if(collisionGids.contains(gid))
                        collisionTiles << tileIdx;

                }
                tileIdx++;
            }
        }
    }

    if(!specialTiles.isEmpty())
        mapVariant[tileLayer->name()] = specialTiles;
}

QVariant MapToVariantConverter::animPropsToVariant(const Properties &properties)
{
    QVariantMap animProps;

    Properties::const_iterator it = properties.constBegin();
    Properties::const_iterator it_end = properties.constEnd();
    for (; it != it_end; ++it) {
        if(it.key() == "length")
            animProps["l"] = it.value().toUInt();
        if(it.key() == "delay")
            animProps["d"] = it.value().toUInt();
    }

    return animProps;
}

QVariant MapToVariantConverter::toVariant(const Properties &properties)
{
    QVariantMap variantMap;

    Properties::const_iterator it = properties.constBegin();
    Properties::const_iterator it_end = properties.constEnd();
    for (; it != it_end; ++it)
        variantMap[it.key()] = it.value();

    return variantMap;
}

// TODO: Unduplicate this class since it's used also in mapwriter.cpp
class TileToPixelCoordinates
{
public:
    TileToPixelCoordinates(Map *map)
    {
        if (map->orientation() == Map::Isometric) {
            // Isometric needs special handling, since the pixel values are
            // based solely on the tile height.
            mMultiplierX = map->tileHeight();
            mMultiplierY = map->tileHeight();
        } else {
            mMultiplierX = map->tileWidth();
            mMultiplierY = map->tileHeight();
        }
    }

    QPoint operator() (qreal x, qreal y) const
    {
        return QPoint(qRound(x * mMultiplierX),
                      qRound(y * mMultiplierY));
    }

private:
    int mMultiplierX;
    int mMultiplierY;
};

void MapToVariantConverter::addLayerAttributes(QVariantMap &layerVariant,
                                               const Layer *layer)
{
    layerVariant["name"] = layer->name();
    layerVariant["width"] = layer->width();
    layerVariant["height"] = layer->height();
    layerVariant["x"] = layer->x();
    layerVariant["y"] = layer->y();
    layerVariant["visible"] = layer->isVisible();
    layerVariant["opacity"] = layer->opacity();

    const Properties &properties = layer->properties();
    if (!properties.isEmpty())
        layerVariant["properties"] = toVariant(properties);
}
