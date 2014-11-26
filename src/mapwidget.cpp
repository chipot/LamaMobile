#include <cmath>
#include <QPainter>
#include "mapwidget.h"
#include "mapwidgetprivate.h"

MapWidgetPrivate::MapWidgetPrivate(MapWidget *ptr)
    : q_ptr(ptr),
      _quadtree(QRectF(-180.0, -90.0, 360.0, 180.0)),
      _center(2.3488000, 48.8534100),
      _scale(7)
{
}

MapWidgetPrivate::~MapWidgetPrivate()
{
}

void MapWidgetPrivate::displayChanged()
{
    Q_Q(MapWidget);
    _changed = true;
    q->update();
}

void MapWidgetPrivate::addTile(const MapTile &tile)
{
    if (!tile.pixmap.isNull() && tile.pixmap.width() > 0 && tile.pixmap.height() > 0)
    {
        QRectF rect;

        QPointF coords = MapWidgetPrivate::coordsFromPos(tile.pos, tile.scale);
        QSizeF size = MapWidgetPrivate::tileSize(tile.pos, tile.scale);

        rect.setX(coords.x());
        rect.setY(coords.y());
        rect.setWidth(size.width());
        rect.setHeight(size.height());

        _quadtree.remove(rect);
        _quadtree.insert(rect, tile);

        displayChanged();
    }
}

void MapWidgetPrivate::paint(QPainter *painter)
{
    Q_Q(const MapWidget);

    if (painter && q->width() > 0 && q->height() > 0 && _scale > 0)
    {
        if (_changed)
            generateCache();
        painter->drawPixmap(0, 0, _cache);
    }
}

quint8 MapWidgetPrivate::getMapScale() const
{
    return _scale;
}

void MapWidgetPrivate::setMapScale(quint8 scale)
{
    if (scale != _scale)
    {
        Q_Q(MapWidget);
        _scale = scale;
        q->mapScaleChanged();
        displayChanged();
    }
}

const QPointF &MapWidgetPrivate::getMapCenter() const
{
    return _center;
}

void MapWidgetPrivate::setMapCenter(const QPointF &center)
{
    if (_center != center)
    {
        Q_Q(MapWidget);
        _center = center;
        q->mapCenterChanged();
        displayChanged();
    }
}

void MapWidgetPrivate::generateCache()
{
    Q_Q(const MapWidget);

    QPixmap pix(q->width(), q->height());
    QPainter painter(&pix);
    painter.fillRect(0, 0, pix.width(), pix.height(), Qt::white);

    QRectF viewRect;
    QSizeF viewSize;

    QPoint centerPos = MapWidgetPrivate::posFromCoords(_center, _scale);
    QSizeF centerTileSize = MapWidgetPrivate::tileSize(centerPos, _scale);
    QPointF centerScale = QPointF(centerTileSize.width() / 256.0, centerTileSize.height() / 256.0);

    viewSize.setWidth((q->width() + 512) * centerScale.x());
    viewSize.setHeight((q->height() + 512) * centerScale.y());
    viewRect.setX(_center.x() - (viewSize.width() / 2.0));
    viewRect.setY(_center.y() - (viewSize.height() / 2.0));
    viewRect.setSize(viewSize);

    QList<QuadtreeObject<MapTile> *> tiles = _quadtree.query(viewRect);

    foreach (QuadtreeObject<MapTile> *tile, tiles)
    {
        const MapTile &obj = tile->object;
        if (obj.scale == _scale)
        {
            QPoint pos;
            QPointF tileCoords;

            QSizeF tileSize = MapWidgetPrivate::tileSize(obj.pos, _scale);
            QPointF tileScale = QPointF(tileSize.width() / 256.0, tileSize.height() / 256.0);

            tileCoords = MapWidgetPrivate::coordsFromPos(obj.pos, _scale);
            pos.setX((int)((tileCoords.x() - _center.x()) / tileScale.x() + q->width() / 2));
            pos.setY((int)((_center.y() - tileCoords.y()) / tileScale.y() + q->height() / 2));

            painter.drawPixmap(pos, obj.pixmap);
        }
    }

    _cache = pix;
    _changed = false;
}

QPoint MapWidgetPrivate::posFromCoords(const QPointF &coords, quint8 scale)
{
    // x = longitude
    // y = latitude

    QPoint pos;
    pos.setX((int)((coords.x() + 180.0) / 360.0 * pow(2.0, scale)));
    pos.setY((int)((1.0 - log(tan(coords.y() * M_PI / 180.0) + 1.0 / cos(coords.y() * M_PI / 180.0)) / M_PI) / 2.0 * pow(2.0, scale)));

    return pos;
}

QPointF MapWidgetPrivate::coordsFromPos(const QPoint &pos, quint8 scale)
{
    // x = longitude
    // y = latitude

    qreal n = M_PI - 2.0 * M_PI * pos.y() / pow(2.0, scale);

    QPointF coords;
    coords.setX(pos.x() / pow(2.0, scale) * 360.0 - 180);
    coords.setY(180.0 / M_PI * atan(0.5 * (exp(n) - exp(-n))));

    return coords;
}

QSizeF MapWidgetPrivate::tileSize(const QPoint &pos, quint8 scale)
{
    // x = longitude
    // y = latitude

    QPointF current = MapWidgetPrivate::coordsFromPos(pos, scale);
    QPointF next = MapWidgetPrivate::coordsFromPos(QPoint(pos.x() + 1, pos.y() + 1), scale);

    QSizeF size;
    size.setWidth(qAbs(current.x() - next.x()));
    size.setHeight(qAbs(current.y() - next.y()));

    return size;
}

MapTile::MapTile()
{
}

MapTile::MapTile(quint8 mscale, const QPoint &mpos, const QPixmap &mpixmap)
    : scale(mscale), pos(mpos), pixmap(mpixmap)
{
}

MapTile::MapTile(const MapTile &other)
    : scale(other.scale), pos(other.pos), pixmap(other.pixmap)
{
}

MapTile &MapTile::operator=(const MapTile &other)
{
    scale = other.scale;
    pos = other.pos;
    pixmap = other.pixmap;
    return *this;
}

MapTile::~MapTile()
{
}

MapWidget::MapWidget(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      d_ptr(new MapWidgetPrivate(this))
{
}

MapWidget::~MapWidget()
{
    delete d_ptr;
}

void MapWidget::addTile(const MapTile &tile)
{
    Q_D(MapWidget);
    d->addTile(tile);
}

void MapWidget::paint(QPainter *painter)
{
    Q_D(MapWidget);
    d->paint(painter);
}

quint8 MapWidget::getMapScale() const
{
    Q_D(const MapWidget);
    return d->getMapScale();
}

void MapWidget::setMapScale(quint8 scale)
{
    Q_D(MapWidget);
    d->setMapScale(qBound<quint8>(1, scale, 18));
}

const QPointF &MapWidget::getMapCenter() const
{
    Q_D(const MapWidget);
    return d->getMapCenter();
}

void MapWidget::setMapCenter(const QPointF &center)
{
    Q_D(MapWidget);
    d->setMapCenter(center);
}

void MapWidget::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if (newGeometry.size() != oldGeometry.size())
    {
        Q_D(MapWidget);
        d->displayChanged();
    }
    QQuickPaintedItem::geometryChanged(newGeometry, oldGeometry);
}