#include "trackline.h"
#include "waypoint.h"
#include <QPainter>
#include <QJsonObject>
#include <QJsonArray>
#include <QStandardItem>
#include <QDebug>
#include "autonomousvehicleproject.h"

TrackLine::TrackLine(MissionItem *parent) :GeoGraphicsMissionItem(parent)
{

}

QRectF TrackLine::boundingRect() const
{
    return childrenBoundingRect();
}

void TrackLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    auto children = waypoints();
    if (children.length() > 1)
    {
        painter->save();

        QPen p;
        p.setColor(Qt::red);
        p.setCosmetic(true);
        p.setWidth(3);
        painter->setPen(p);
        painter->drawPath(shape());

//         auto first = children.begin();
//         auto second = first;
//         second++;
//         while(second != children.end())
//         {
//             painter->drawLine((*first)->pos(),(*second)->pos());
//             first++;
//             second++;
//         }

        painter->restore();

    }

}

QPainterPath TrackLine::shape() const
{
    auto children = waypoints();
    if (children.length() > 1)
    {
        auto i = children.begin();
        QPainterPath ret((*i)->pos());
        i++;
        while(i != children.end())
        {
            ret.lineTo((*i)->pos());
            i++;
        }
        QPainterPathStroker pps;
        pps.setWidth(5);
        return pps.createStroke(ret);

    }
    return QGraphicsItem::shape();
}

Waypoint * TrackLine::createWaypoint()
{
    int i = childMissionItems().size();
    QString wplabel = "waypoint"+QString::number(i);
    Waypoint *wp = createMissionItem<Waypoint>(wplabel);

    wp->setFlag(QGraphicsItem::ItemIsMovable);
    wp->setFlag(QGraphicsItem::ItemIsSelectable);
    wp->setFlag(QGraphicsItem::ItemSendsGeometryChanges);
    return wp;
}

Waypoint * TrackLine::addWaypoint(const QGeoCoordinate &location)
{
    Waypoint *wp = createWaypoint();
    wp->setLocation(location);
    emit trackLineUpdated();
    update();
    return wp;
}

void TrackLine::removeWaypoint(Waypoint* wp)
{
    autonomousVehicleProject()->deleteItem(wp);
}


QList<Waypoint *> TrackLine::waypoints() const
{
    QList<Waypoint *> ret;
    auto children = childMissionItems();
    for(auto child: children)
    {
        Waypoint *wp = qobject_cast<Waypoint*>(child);
            if(wp)
                ret.append(wp);
    }
    return ret;
}

QList<QList<QGeoCoordinate> > TrackLine::getLines() const
{
    QList<QList<QGeoCoordinate> > ret;
    ret.append(QList<QGeoCoordinate>());
    for(auto wp:waypoints())
        ret.back().append(wp->location());
    return ret;
}


void TrackLine::write(QJsonObject &json) const
{
    json["type"] = "TrackLine";
    QJsonArray wpArray;
    auto children = childItems();
    for(auto child: children)
    {
        if(child->type() == GeoGraphicsItem::WaypointType)
        {
            Waypoint *wp = qgraphicsitem_cast<Waypoint*>(child);
            QJsonObject wpObject;
            wp->write(wpObject);
            wpArray.append(wpObject);
        }
    }
    json["waypoints"] = wpArray;
}

void TrackLine::writeToMissionPlan(QJsonArray& navArray) const
{
    QJsonObject navItem;
    QJsonObject pathObject;
    writeBehaviorsToMissionPlanObject(pathObject);
    QJsonArray pathNavArray;
    auto children = childItems();
    for(auto child: children)
    {
        if(child->type() == GeoGraphicsItem::WaypointType)
        {
            Waypoint *wp = qgraphicsitem_cast<Waypoint*>(child);
            wp->writeToMissionPlan(pathNavArray);
        }
    }
    pathObject["nav"] = pathNavArray;
    navItem["path"] = pathObject;
    navArray.append(navItem);
}

void TrackLine::read(const QJsonObject &json)
{
    QJsonArray waypointsArray = json["waypoints"].toArray();
    for(int wpIndex = 0; wpIndex < waypointsArray.size(); wpIndex++)
    {
        QJsonObject wpObject = waypointsArray[wpIndex].toObject();
        if(wpIndex == 0)
        {
            QGeoCoordinate position(wpObject["latitude"].toDouble(),wpObject["longitude"].toDouble());
            setPos(geoToPixel(position,autonomousVehicleProject()));
        }
        Waypoint *wp = createWaypoint();
        wp->read(wpObject);
    }

}

void TrackLine::updateProjectedPoints()
{
    for(auto wp: waypoints())
        wp->updateProjectedPoints();
}

bool TrackLine::canAcceptChildType(const std::string& childType) const
{
    return childType == "Waypoint";
}
