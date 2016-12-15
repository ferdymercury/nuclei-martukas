#ifndef TransitionItem_H
#define TransitionItem_H

#include <stdint.h>
#include <QFont>
#include <QPen>
#include "ClickableItem.h"
#include "Transition.h"

class QGraphicsItem;
class ActiveGraphicsItemGroup;
class QGraphicsLineItem;
class QGraphicsTextItem;
class QGraphicsPolygonItem;
class QGraphicsRectItem;
class GraphicsHighlightItem;
class QGraphicsScene;

class TransitionItem : public ClickableItem
{
public:

  TransitionItem();
  TransitionItem(Transition transition, SchemeVisualSettings vis, QGraphicsScene *scene);

  virtual ~TransitionItem();

  void updateArrow(double arrowDestY, double max_intensity);
  double minimalXDistance() const;
  /**
     * Distance between origin and right edge of the bounding rect
     */
  double widthFromOrigin() const;

  QPen pen() const;

  QGraphicsLineItem* arrow {nullptr};
  QGraphicsTextItem* text {nullptr};
  QGraphicsPolygonItem* arrowhead {nullptr};
  QGraphicsPolygonItem* arrowbase {nullptr};
  QGraphicsRectItem* clickarea {nullptr};
  GraphicsHighlightItem* highlightHelper {nullptr};
  double mindist {0.0};
  QPen m_pen;

  Transition transition_;

private:
  static const double textAngle;
  static const double arrowHeadLength;
  static const double arrowBaseLength;
  static const double arrowHeadWidth;
  static const double arrowBaseWidth;
  static const double highlightWidth;
  static const double maxwidth;

  static const QPolygonF arrowHeadShape, arrowBaseShape;
  static QPolygonF initArrowHead(double width);
  static QPolygonF initArrowBase(double width);
};

#endif // TransitionItem_H
