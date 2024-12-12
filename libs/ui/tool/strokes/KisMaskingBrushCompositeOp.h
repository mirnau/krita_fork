/*
 *  SPDX-FileCopyrightText: 2017 Dmitry Kazakov <dimula73@gmail.com>
 *  SPDX-FileCopyrightText: 2021 Deif Lou <ginoba@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KISMASKINGBRUSHCOMPOSITEOP_H
#define KISMASKINGBRUSHCOMPOSITEOP_H

#include <type_traits>

#ifdef HAVE_OPENEXR
#include "half.h"
#endif

#include <KoColorSpaceTraits.h>
#include <KoGrayColorSpaceTraits.h>
#include <KoColorSpaceMaths.h>
#include <KoCompositeOpFunctions.h>
#include <kritaui_export.h>

#include "KisMaskingBrushCompositeOpBase.h"

enum KisMaskingBrushCompositeFuncTypes
{
    KIS_MASKING_BRUSH_COMPOSITE_MULT,
    KIS_MASKING_BRUSH_COMPOSITE_DARKEN,
    KIS_MASKING_BRUSH_COMPOSITE_OVERLAY,
    KIS_MASKING_BRUSH_COMPOSITE_DODGE,
    KIS_MASKING_BRUSH_COMPOSITE_BURN,
    KIS_MASKING_BRUSH_COMPOSITE_LINEAR_BURN,
    KIS_MASKING_BRUSH_COMPOSITE_LINEAR_DODGE,
    KIS_MASKING_BRUSH_COMPOSITE_HARD_MIX_PHOTOSHOP,
    KIS_MASKING_BRUSH_COMPOSITE_HARD_MIX_SOFTER_PHOTOSHOP,
    KIS_MASKING_BRUSH_COMPOSITE_SUBTRACT,
    KIS_MASKING_BRUSH_COMPOSITE_HEIGHT,
    KIS_MASKING_BRUSH_COMPOSITE_LINEAR_HEIGHT,
    KIS_MASKING_BRUSH_COMPOSITE_HEIGHT_PHOTOSHOP,
    KIS_MASKING_BRUSH_COMPOSITE_LINEAR_HEIGHT_PHOTOSHOP
};

namespace KisMaskingBrushCompositeDetail
{

template <typename channels_type>
struct StrengthCompositeFunctionBase
{
    const channels_type strength;
    StrengthCompositeFunctionBase(qreal strength)
        : strength(KoColorSpaceMaths<qreal, channels_type>::scaleToA(strength))
    {}
};

template <typename channels_type, int composite_function, bool use_strength, bool use_soft_texturing>
struct CompositeFunction;

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_MULT, false, false>
{
    channels_type apply(channels_type src, channels_type dst)
    {
        return cfMultiply(src, dst);
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_MULT, true, false>
    : public StrengthCompositeFunctionBase<channels_type>
{
    CompositeFunction(qreal strength) : StrengthCompositeFunctionBase<channels_type>(strength) {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        return Arithmetic::mul(src, dst, StrengthCompositeFunctionBase<channels_type>::strength);
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_MULT, true, true>
    : public StrengthCompositeFunctionBase<channels_type>
{
    const channels_type invertedStrength;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , invertedStrength(Arithmetic::inv(StrengthCompositeFunctionBase<channels_type>::strength))
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        return Arithmetic::mul(Arithmetic::unionShapeOpacity(src, invertedStrength), dst);
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_DARKEN, false, false>
{
    channels_type apply(channels_type src, channels_type dst)
    {
        return cfDarkenOnly(src, dst);
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_DARKEN, true, false>
    : public StrengthCompositeFunctionBase<channels_type>
{
    CompositeFunction(qreal strength) : StrengthCompositeFunctionBase<channels_type>(strength) {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        return cfDarkenOnly(src, Arithmetic::mul(dst, StrengthCompositeFunctionBase<channels_type>::strength));
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_DARKEN, true, true>
    : public StrengthCompositeFunctionBase<channels_type>
{
    const channels_type invertedStrength;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , invertedStrength(Arithmetic::inv(StrengthCompositeFunctionBase<channels_type>::strength))
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        return cfDarkenOnly(Arithmetic::unionShapeOpacity(src, invertedStrength), dst);
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_OVERLAY, false, false>
{
    channels_type apply(channels_type src, channels_type dst)
    {
        return CFOverlay<channels_type>::composeChannel(src, dst);
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_OVERLAY, true, false>
    : public StrengthCompositeFunctionBase<channels_type>
{
    CompositeFunction(qreal strength) : StrengthCompositeFunctionBase<channels_type>(strength) {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        return CFOverlay<channels_type>::composeChannel(src, Arithmetic::mul(dst, StrengthCompositeFunctionBase<channels_type>::strength));
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_OVERLAY, true, true>
    : public StrengthCompositeFunctionBase<channels_type>
{
    const channels_type invertedStrength;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , invertedStrength(Arithmetic::inv(StrengthCompositeFunctionBase<channels_type>::strength))
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        return CFOverlay<channels_type>::composeChannel(Arithmetic::unionShapeOpacity(src, invertedStrength), dst);
    }
};

/**
 * A special Color Dodge variant for alpha channel.
 *
 * The meaning of alpha channel is a bit different from the one in color.
 * Color dodge can quickly make the values higher than 1 or less than 0 so,
 * contrary to the color values case, we should clamp to the unit range
 */

template<class T>
inline T colorDodgeAlphaHelper(T src, T dst)
{
    using composite_type = typename KoColorSpaceMathsTraits<T>::compositetype;
    using namespace Arithmetic;
    // Handle the case where the denominator is 0.
    // When src is 1 then the denominator (1 - src) becomes 0, and to avoid
    // dividing by 0 we treat the denominator as an infinitely small number,
    // so the result of the formula would approach infinity.
    // For alpha values, the result should be clamped to the unit range,
    // contrary to the color version, where the values should be clamped to
    // the min/max range.
    // Another special case is when both numerator and denominator are 0. In
    // this case we also treat the denominator as an infinitely small number,
    // and the numerator can remain as 0, so dividing 0 over a number (no matter
    // how small it is) gives 0.
    if (isUnitValue<T>(src)) {
        return isZeroValue<T>(dst) ? zeroValue<T>() : unitValue<T>();
    }
    return qBound(composite_type(KoColorSpaceMathsTraits<T>::zeroValue),
                  div(dst, inv(src)),
                  composite_type(KoColorSpaceMathsTraits<T>::unitValue));
}

// Integer version of color dodge alpha
template<class T>
inline typename std::enable_if<std::numeric_limits<T>::is_integer, T>::type
colorDodgeAlpha(T src, T dst)
{
    return colorDodgeAlphaHelper(src, dst);
}

// Floating point version of color dodge alpha
template<class T>
inline typename std::enable_if<!std::numeric_limits<T>::is_integer, T>::type
colorDodgeAlpha(T src, T dst)
{
    const T result = colorDodgeAlphaHelper(src, dst);
    // Constantly dividing by small numbers can quickly make the result
    // become infinity or NaN, so we check that and correct (kind of clamping)
    return std::isfinite(result) ? result : KoColorSpaceMathsTraits<T>::unitValue;
}

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_DODGE, false, false>
{
    channels_type apply(channels_type src, channels_type dst)
    {
        return colorDodgeAlpha(src, dst);
    }
};

template <typename channels_type, bool use_soft_texturing>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_DODGE, true, use_soft_texturing>
    : public StrengthCompositeFunctionBase<channels_type>
{
    CompositeFunction(qreal strength) : StrengthCompositeFunctionBase<channels_type>(strength) {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        if constexpr (use_soft_texturing) {
            return colorDodgeAlpha(Arithmetic::mul(src, StrengthCompositeFunctionBase<channels_type>::strength), dst);
        } else {
            return colorDodgeAlpha(src, Arithmetic::mul(dst, StrengthCompositeFunctionBase<channels_type>::strength));
        }
    }
};

/**
 * A special Color Burn variant for alpha channel.
 *
 * The meaning of alpha channel is a bit different from the one in color.
 * Color burn can quickly make the values less than 0 so,
 * contrary to the color values case, we should clamp to the unit range
 */

template<class T>
inline T colorBurnAlphaHelper(T src, T dst)
{
    using composite_type = typename KoColorSpaceMathsTraits<T>::compositetype;
    using namespace Arithmetic;
    // Handle the case where the denominator is 0. See color dodge for a
    // detailed explanation
    if(isZeroValue<T>(src)) {
        return isUnitValue<T>(dst) ? zeroValue<T>() : unitValue<T>();
    }
    return qBound(composite_type(KoColorSpaceMathsTraits<T>::zeroValue),
                  div(inv(dst), src),
                  composite_type(KoColorSpaceMathsTraits<T>::unitValue));
}

template<class T>
inline T colorBurnAlpha(T src, T dst)
{
    using namespace Arithmetic;
    using namespace KoCompositeOpClampPolicy;
    return FunctorWithSDRClampPolicy<CFColorBurn, T>::composeChannel(src, dst);
}

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_BURN, false, false>
{
    channels_type apply(channels_type src, channels_type dst)
    {
        return colorBurnAlpha(src, dst);
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_BURN, true, false>
    : public StrengthCompositeFunctionBase<channels_type>
{
    CompositeFunction(qreal strength) : StrengthCompositeFunctionBase<channels_type>(strength) {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        return colorBurnAlpha(src, Arithmetic::mul(dst, StrengthCompositeFunctionBase<channels_type>::strength));
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_BURN, true, true>
    : public StrengthCompositeFunctionBase<channels_type>
{
    const channels_type invertedStrength;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , invertedStrength(Arithmetic::inv(StrengthCompositeFunctionBase<channels_type>::strength))
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        return colorBurnAlpha(Arithmetic::unionShapeOpacity(src, invertedStrength), dst);
    }
};

/**
 * A special Linear Dodge variant for alpha channel.
 *
 * The meaning of alpha channel is a bit different from the one in color. If
 * alpha channel of the destination is totally null, we should not try
 * to resurrect its contents from ashes :)
 */
template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_LINEAR_DODGE, false, false>
{
    channels_type apply(channels_type src, channels_type dst)
    {
        using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
        using namespace Arithmetic;
        if (isZeroValue<channels_type>(dst)) {
            return zeroValue<channels_type>();
        }
        return qMin(composite_type(src) + dst, composite_type(unitValue<channels_type>()));
    }
};

template <typename channels_type, bool use_soft_texturing>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_LINEAR_DODGE, true, use_soft_texturing>
    : public StrengthCompositeFunctionBase<channels_type>
{
    CompositeFunction(qreal strength) : StrengthCompositeFunctionBase<channels_type>(strength) {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
        using namespace Arithmetic;
        if (isZeroValue<channels_type>(dst)) {
            return zeroValue<channels_type>();
        }
        if constexpr (use_soft_texturing) {
            return qMin(composite_type(mul(src, StrengthCompositeFunctionBase<channels_type>::strength) + dst),
                        composite_type(unitValue<channels_type>()));
        } else {
            return qMin(composite_type(src) + mul(dst, StrengthCompositeFunctionBase<channels_type>::strength),
                        composite_type(unitValue<channels_type>()));
        }
    }
};

/**
 * A special Linear Burn variant for alpha channel
 *
 * The meaning of alpha channel is a bit different from the one in color. We should
 * clamp the values around [zero, max] only to avoid the brush to **erase** the content
 * of the layer below
 */
template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_LINEAR_BURN, false, false>
{
    channels_type apply(channels_type src, channels_type dst)
    {
        using namespace Arithmetic;
        using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
        return qMax(composite_type(KoColorSpaceMathsTraits<channels_type>::zeroValue),
                    composite_type(src) + dst - unitValue<channels_type>());
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_LINEAR_BURN, true, false>
    : public StrengthCompositeFunctionBase<channels_type>
{
    CompositeFunction(qreal strength) : StrengthCompositeFunctionBase<channels_type>(strength) {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using namespace Arithmetic;
        using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
        return qMax(composite_type(zeroValue<channels_type>()),
                    composite_type(src) + mul(dst, StrengthCompositeFunctionBase<channels_type>::strength)
                        - unitValue<channels_type>());
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_LINEAR_BURN, true, true>
    : public StrengthCompositeFunctionBase<channels_type>
{
    const channels_type invertedStrength;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , invertedStrength(Arithmetic::inv(StrengthCompositeFunctionBase<channels_type>::strength))
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using namespace Arithmetic;
        using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
        return qMax(composite_type(zeroValue<channels_type>()),
                    composite_type(unionShapeOpacity(src, invertedStrength) + dst - unitValue<channels_type>()));
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_HARD_MIX_PHOTOSHOP, false, false>
{
    channels_type apply(channels_type src, channels_type dst)
    {
        return CFHardMixPhotoshop<channels_type>::composeChannel(src, dst);
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_HARD_MIX_PHOTOSHOP, true, false>
    : public StrengthCompositeFunctionBase<channels_type>
{
    CompositeFunction(qreal strength) : StrengthCompositeFunctionBase<channels_type>(strength) {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        return CFHardMixPhotoshop<channels_type>::composeChannel(src, Arithmetic::mul(dst, StrengthCompositeFunctionBase<channels_type>::strength));
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_HARD_MIX_PHOTOSHOP, true, true>
    : public StrengthCompositeFunctionBase<channels_type>
{
    const channels_type invertedStrength;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , invertedStrength(Arithmetic::inv(StrengthCompositeFunctionBase<channels_type>::strength))
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using namespace Arithmetic;
        return mul(CFHardMixPhotoshop<channels_type>::composeChannel(unionShapeOpacity(src, invertedStrength), dst),
                   unionShapeOpacity(dst, StrengthCompositeFunctionBase<channels_type>::strength));
    }
};

/**
 * A special Hard Mix Softer variant for alpha channel
 *
 * The meaning of alpha channel is a bit different from the one in color.
 * We have to clamp the values to the unit range
 */

template<class T>
inline T hardMixSofterPhotoshopAlpha(T src, T dst) {
    using namespace Arithmetic;
    typedef typename KoColorSpaceMathsTraits<T>::compositetype composite_type;
    const composite_type srcScaleFactor = static_cast<composite_type>(2);
    const composite_type dstScaleFactor = static_cast<composite_type>(3);
    return qBound(composite_type(KoColorSpaceMathsTraits<T>::zeroValue),
                  dstScaleFactor * dst - srcScaleFactor * inv(src),
                  composite_type(KoColorSpaceMathsTraits<T>::unitValue));
}

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_HARD_MIX_SOFTER_PHOTOSHOP, false, false>
{
    channels_type apply(channels_type src, channels_type dst)
    {
        return hardMixSofterPhotoshopAlpha(src, dst);
    }
};

template <typename channels_type, bool use_soft_texturing>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_HARD_MIX_SOFTER_PHOTOSHOP, true, use_soft_texturing>
    : public StrengthCompositeFunctionBase<channels_type>
{
    CompositeFunction(qreal strength) : StrengthCompositeFunctionBase<channels_type>(strength) {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        if constexpr (use_soft_texturing) {
            return hardMixSofterPhotoshopAlpha(Arithmetic::mul(src, StrengthCompositeFunctionBase<channels_type>::strength), dst);
        } else {
            return hardMixSofterPhotoshopAlpha(src, Arithmetic::mul(dst, StrengthCompositeFunctionBase<channels_type>::strength));
        }
    }
};

/**
 * A special Subtract variant for alpha channel.
 *
 * The meaning of alpha channel is a bit different from the one in color.
 * If the result of the subtraction becomes negative, we should clamp it
 * to the unit range. Otherwise, the layer may have negative alpha channel,
 * which generates funny artifacts :) See bug 424210.
 */
template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_SUBTRACT, false, false>
{
    channels_type apply(channels_type src, channels_type dst)
    {
        using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
        using namespace Arithmetic;
        return qMax(composite_type(KoColorSpaceMathsTraits<channels_type>::zeroValue),
                    composite_type(dst) - src);
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_SUBTRACT, true, false>
    : public StrengthCompositeFunctionBase<channels_type>
{
    const channels_type invertedStrength;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , invertedStrength(Arithmetic::inv(StrengthCompositeFunctionBase<channels_type>::strength))
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
        using namespace Arithmetic;
        return qMax(composite_type(zeroValue<channels_type>()),
                    composite_type(dst) - (composite_type(src) + invertedStrength));
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_SUBTRACT, true, true>
    : public StrengthCompositeFunctionBase<channels_type>
{
    CompositeFunction(qreal strength) : StrengthCompositeFunctionBase<channels_type>(strength) {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
        using namespace Arithmetic;
        return qMax(composite_type(KoColorSpaceMathsTraits<channels_type>::zeroValue),
                    composite_type(dst) - mul(src, StrengthCompositeFunctionBase<channels_type>::strength));
    }
};

template <typename channels_type, bool use_soft_texturing>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_HEIGHT, true, use_soft_texturing>
    : public StrengthCompositeFunctionBase<channels_type>
{
    const channels_type invertedStrength;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(0.99 * strength)
        , invertedStrength(Arithmetic::inv(StrengthCompositeFunctionBase<channels_type>::strength))
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
        using namespace Arithmetic;
        if constexpr (use_soft_texturing) {
            return qBound(composite_type(zeroValue<channels_type>()),
                          div(dst, invertedStrength) -
                          mul(src, StrengthCompositeFunctionBase<channels_type>::strength),
                          composite_type(unitValue<channels_type>()));
        } else {
            return qBound(composite_type(zeroValue<channels_type>()),
                          div(dst, invertedStrength) - (composite_type(src) + invertedStrength),
                          composite_type(unitValue<channels_type>()));
        }
    }
};

template <typename channels_type, bool use_soft_texturing>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_LINEAR_HEIGHT, true, use_soft_texturing>
    : public StrengthCompositeFunctionBase<channels_type>
{
    const channels_type invertedStrength;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(0.99 * strength)
        , invertedStrength(Arithmetic::inv(StrengthCompositeFunctionBase<channels_type>::strength))
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
        using namespace Arithmetic;

        if constexpr (use_soft_texturing) {
            const composite_type modifiedDst = div(dst, invertedStrength);
            const channels_type srcTimesStrength = mul(src, StrengthCompositeFunctionBase<channels_type>::strength);
            const composite_type multiply = modifiedDst * inv(srcTimesStrength) / unitValue<channels_type>();
            const composite_type height = modifiedDst - srcTimesStrength;
            return qBound(composite_type(zeroValue<channels_type>()),
                          qMax(multiply, height),
                          composite_type(unitValue<channels_type>()));
        } else {
            const composite_type modifiedDst = div(dst, invertedStrength) - invertedStrength;
            const composite_type multiply = modifiedDst * inv(src) / unitValue<channels_type>();
            const composite_type height = modifiedDst - src;
            return qBound(composite_type(zeroValue<channels_type>()),
                          qMax(multiply, height),
                          composite_type(unitValue<channels_type>()));
        }
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_HEIGHT_PHOTOSHOP, true, false>
    : public StrengthCompositeFunctionBase<channels_type>
{
    using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
    const composite_type weight;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , weight(composite_type(10) * StrengthCompositeFunctionBase<channels_type>::strength)
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using namespace Arithmetic;
        return qBound(composite_type(zeroValue<channels_type>()),
                      dst * weight / unitValue<channels_type>() - src,
                      composite_type(unitValue<channels_type>()));
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_HEIGHT_PHOTOSHOP, true, true>
    : public StrengthCompositeFunctionBase<channels_type>
{
    using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;
    const composite_type weight;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , weight(composite_type(9) * StrengthCompositeFunctionBase<channels_type>::strength)
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using namespace Arithmetic;
        return qBound(composite_type(zeroValue<channels_type>()),
                      composite_type(dst) + dst * weight / unitValue<channels_type>() -
                      mul(src, StrengthCompositeFunctionBase<channels_type>::strength),
                      composite_type(unitValue<channels_type>()));
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_LINEAR_HEIGHT_PHOTOSHOP, true, false>
    : public StrengthCompositeFunctionBase<channels_type>
{
    using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;

    const composite_type weight;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , weight(composite_type(10) * StrengthCompositeFunctionBase<channels_type>::strength)
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using namespace Arithmetic;
        const composite_type modifiedDst = dst * weight / unitValue<channels_type>();
        const composite_type multiply = inv(src) * modifiedDst / unitValue<channels_type>();
        const composite_type height = modifiedDst - src;
        return qBound(composite_type(zeroValue<channels_type>()),
                      qMax(multiply, height),
                      composite_type(unitValue<channels_type>()));
    }
};

template <typename channels_type>
struct CompositeFunction<channels_type, KIS_MASKING_BRUSH_COMPOSITE_LINEAR_HEIGHT_PHOTOSHOP, true, true>
    : public StrengthCompositeFunctionBase<channels_type>
{
    using composite_type = typename KoColorSpaceMathsTraits<channels_type>::compositetype;

    const composite_type weight;

    CompositeFunction(qreal strength)
        : StrengthCompositeFunctionBase<channels_type>(strength)
        , weight(composite_type(9) * StrengthCompositeFunctionBase<channels_type>::strength)
    {}
    
    channels_type apply(channels_type src, channels_type dst)
    {
        using namespace Arithmetic;
        const composite_type modifiedDst = dst + dst * weight / unitValue<channels_type>();
        const channels_type srcTimesStrength = mul(src, StrengthCompositeFunctionBase<channels_type>::strength);
        const composite_type multiply = modifiedDst * inv(srcTimesStrength) / unitValue<channels_type>();
        const composite_type height = modifiedDst - srcTimesStrength;
        return qBound(composite_type(zeroValue<channels_type>()),
                      qMax(multiply, height),
                      composite_type(unitValue<channels_type>()));
    }
};

}

template <typename channels_type, int composite_function, bool mask_is_alpha = false,
          bool use_strength = false, bool use_soft_texturing = false>
class KisMaskingBrushCompositeOp : public KisMaskingBrushCompositeOpBase
{
public:
    using MaskPixel = typename std::conditional<mask_is_alpha, quint8, KoGrayU8Traits::Pixel>::type;

    template <bool use_strength_ = use_strength, typename = typename std::enable_if<!use_strength_>::type>
    KisMaskingBrushCompositeOp(int dstPixelSize, int dstAlphaOffset)
        : m_dstPixelSize(dstPixelSize)
        , m_dstAlphaOffset(dstAlphaOffset)
    {}

    template <bool use_strength_ = use_strength, typename = typename std::enable_if<use_strength_>::type>
    KisMaskingBrushCompositeOp(int dstPixelSize, int dstAlphaOffset, qreal strength)
        : m_dstPixelSize(dstPixelSize)
        , m_dstAlphaOffset(dstAlphaOffset)
        , m_compositeFunction(strength)
    {}

    void composite(const quint8 *srcRowStart, int srcRowStride,
                   quint8 *dstRowStart, int dstRowStride,
                   int columns, int rows) override
    {
        dstRowStart += m_dstAlphaOffset;

        for (int y = 0; y < rows; y++) {
            const quint8 *srcPtr = srcRowStart;
            quint8 *dstPtr = dstRowStart;

            for (int x = 0; x < columns; x++) {

                const MaskPixel *srcDataPtr = reinterpret_cast<const MaskPixel*>(srcPtr);

                const quint8 mask = preprocessMask(srcDataPtr);
                const channels_type maskScaled = KoColorSpaceMaths<quint8, channels_type>::scaleToA(mask);

                channels_type *dstDataPtr = reinterpret_cast<channels_type*>(dstPtr);
                *dstDataPtr = m_compositeFunction.apply(maskScaled, *dstDataPtr);

                srcPtr += sizeof(MaskPixel);
                dstPtr += m_dstPixelSize;
            }

            srcRowStart += srcRowStride;
            dstRowStart += dstRowStride;
        }
    }


private:
    inline quint8 preprocessMask(const quint8 *pixel)
    {
        return *pixel;
    }

    inline quint8 preprocessMask(const KoGrayU8Traits::Pixel *pixel)
    {
        return KoColorSpaceMaths<quint8>::multiply(pixel->gray, pixel->alpha);
    }

private:
    int m_dstPixelSize;
    int m_dstAlphaOffset;
    KisMaskingBrushCompositeDetail::CompositeFunction
        <channels_type, composite_function, use_strength, use_soft_texturing> m_compositeFunction;
};

#endif // KISMASKINGBRUSHCOMPOSITEOP_H
