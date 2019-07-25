from . import _PythonMagick

class Image(_PythonMagick.Image):
    pass

class Blob(_PythonMagick.Blob):
    def __init__(self,*args):
        if len(args)==1 and type(args[0])==type(''):
            _PythonMagick.Blob.__init__(self)
            self.update(args[0])
        else:
            _PythonMagick.Blob.__init__(self,*args)
    data=property(_PythonMagick.get_blob_data,_PythonMagick.Blob.update)

class Color(_PythonMagick.Color):
   pass

class ColorspaceType(_PythonMagick.ColorspaceType):
    pass

class CompositeOperator(_PythonMagick.CompositeOperator):
   pass

class CompressionType(_PythonMagick.CompressionType):
   pass

class Coordinate(_PythonMagick.Coordinate):
   pass

class DecorationType(_PythonMagick.DecorationType):
   pass

class DrawableAffine(_PythonMagick.DrawableAffine):
   pass

class DrawableArc(_PythonMagick.DrawableArc):
   pass

class DrawableBezier(_PythonMagick.DrawableBezier):
   pass

class DrawableCircle(_PythonMagick.DrawableCircle):
   pass

class DrawableClipPath(_PythonMagick.DrawableClipPath):
   pass

class DrawableColor(_PythonMagick.DrawableColor):
   pass

class DrawableCompositeImage(_PythonMagick.DrawableCompositeImage):
   pass

class DrawableDashArray(_PythonMagick.DrawableDashArray):
   pass

class DrawableDashOffset(_PythonMagick.DrawableDashOffset):
   pass

class DrawableEllipse(_PythonMagick.DrawableEllipse):
   pass

class DrawableFillColor(_PythonMagick.DrawableFillColor):
   pass

class DrawableFillOpacity(_PythonMagick.DrawableFillOpacity):
   pass

class DrawableFillRule(_PythonMagick.DrawableFillRule):
   pass

class DrawableFont(_PythonMagick.DrawableFont):
   pass

class DrawableGravity(_PythonMagick.DrawableGravity):
   pass

class DrawableLine(_PythonMagick.DrawableLine):
   pass

class DrawableMatte(_PythonMagick.DrawableMatte):
   pass

class DrawableMiterLimit(_PythonMagick.DrawableMiterLimit):
   pass

class DrawablePath(_PythonMagick.DrawablePath):
   pass

class DrawablePoint(_PythonMagick.DrawablePoint):
   pass

class DrawablePointSize(_PythonMagick.DrawablePointSize):
   pass

class DrawablePolygon(_PythonMagick.DrawablePolygon):
   pass

class DrawablePolyline(_PythonMagick.DrawablePolyline):
   pass

class DrawablePopClipPath(_PythonMagick.DrawablePopClipPath):
   pass

class DrawablePopGraphicContext(_PythonMagick.DrawablePopGraphicContext):
   pass

class DrawablePopPattern(_PythonMagick.DrawablePopPattern):
   pass

class DrawablePushClipPath(_PythonMagick.DrawablePushClipPath):
   pass

class DrawablePushGraphicContext(_PythonMagick.DrawablePushGraphicContext):
   pass

class DrawablePushPattern(_PythonMagick.DrawablePushPattern):
   pass

class DrawableRectangle(_PythonMagick.DrawableRectangle):
   pass

class DrawableRotation(_PythonMagick.DrawableRotation):
   pass

class DrawableRoundRectangle(_PythonMagick.DrawableRoundRectangle):
   pass

class DrawableScaling(_PythonMagick.DrawableScaling):
   pass

class DrawableSkewX(_PythonMagick.DrawableSkewX):
   pass

class DrawableSkewY(_PythonMagick.DrawableSkewY):
   pass

class DrawableStrokeAntialias(_PythonMagick.DrawableStrokeAntialias):
   pass

class DrawableStrokeColor(_PythonMagick.DrawableStrokeColor):
   pass

class DrawableStrokeLineCap(_PythonMagick.DrawableStrokeLineCap):
   pass

class DrawableStrokeLineJoin(_PythonMagick.DrawableStrokeLineJoin):
   pass

class DrawableStrokeOpacity(_PythonMagick.DrawableStrokeOpacity):
   pass

class DrawableStrokeWidth(_PythonMagick.DrawableStrokeWidth):
   pass

class DrawableText(_PythonMagick.DrawableText):
   pass

class DrawableTextAntialias(_PythonMagick.DrawableTextAntialias):
   pass

class DrawableTextDecoration(_PythonMagick.DrawableTextDecoration):
   pass

class DrawableTextUnderColor(_PythonMagick.DrawableTextUnderColor):
   pass

class DrawableTranslation(_PythonMagick.DrawableTranslation):
   pass

class DrawableViewbox(_PythonMagick.DrawableViewbox):
   pass

class Exception(_PythonMagick.Exception):
   pass

class FilterTypes(_PythonMagick.FilterTypes):
   pass

class Geometry(_PythonMagick.Geometry):
   pass

class GravityType(_PythonMagick.GravityType):
   pass

class PathArcAbs(_PythonMagick.PathArcAbs):
   pass

class PathArcArgs(_PythonMagick.PathArcArgs):
   pass

class PathArcRel(_PythonMagick.PathArcRel):
   pass

class PathClosePath(_PythonMagick.PathClosePath):
   pass

class PathCurvetoAbs(_PythonMagick.PathCurvetoAbs):
   pass

class PathCurvetoArgs(_PythonMagick.PathCurvetoArgs):
   pass

class PathCurvetoRel(_PythonMagick.PathCurvetoRel):
   pass

class PathLinetoAbs(_PythonMagick.PathLinetoAbs):
   pass

class PathLinetoHorizontalAbs(_PythonMagick.PathLinetoHorizontalAbs):
   pass

class PathLinetoHorizontalRel(_PythonMagick.PathLinetoHorizontalRel):
   pass

class PathLinetoRel(_PythonMagick.PathLinetoRel):
   pass

class PathLinetoVerticalAbs(_PythonMagick.PathLinetoVerticalAbs):
   pass

class PathLinetoVerticalRel(_PythonMagick.PathLinetoVerticalRel):
   pass

class PathMovetoAbs(_PythonMagick.PathMovetoAbs):
   pass

class PathMovetoRel(_PythonMagick.PathMovetoRel):
   pass

class PathQuadraticCurvetoAbs(_PythonMagick.PathQuadraticCurvetoAbs):
   pass

class PathQuadraticCurvetoArgs(_PythonMagick.PathQuadraticCurvetoArgs):
   pass

class PathQuadraticCurvetoRel(_PythonMagick.PathQuadraticCurvetoRel):
   pass

class PathSmoothCurvetoAbs(_PythonMagick.PathSmoothCurvetoAbs):
   pass

class PathSmoothCurvetoRel(_PythonMagick.PathSmoothCurvetoRel):
   pass

class PathSmoothQuadraticCurvetoAbs(_PythonMagick.PathSmoothQuadraticCurvetoAbs):
   pass

class PathSmoothQuadraticCurvetoRel(_PythonMagick.PathSmoothQuadraticCurvetoRel):
   pass

class Pixels(_PythonMagick.Pixels):
   pass

class TypeMetric(_PythonMagick.TypeMetric):
   pass

class VPath(_PythonMagick.VPath):
   pass
