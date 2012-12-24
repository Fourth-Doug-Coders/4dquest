/*
 * BrowserQuest Client Tiled Plugin
 * Copyright 2012, Jeff Lang <jeffplang@gmail.com>
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

#ifndef BQCLIENT_H
#define BQCLIENT_H

#include "bqclient_global.h"

#include "mapwriterinterface.h"

#include <QObject>

namespace Tiled {
class Map;
}

namespace BQClient {

class BQCLIENTSHARED_EXPORT BQClientPlugin
        : public QObject
        , public Tiled::MapWriterInterface
{
    Q_OBJECT
    Q_INTERFACES(Tiled::MapWriterInterface)

public:
    BQClientPlugin();

    // MapWriterInterface
    bool write(const Tiled::Map *map, const QString &fileName);

    // Both interfaces
    QString nameFilter() const;
    QString errorString() const;

private:
    QString mError;
};

} // namespace Json

#endif // BQCLIENTPLUGIN_H
