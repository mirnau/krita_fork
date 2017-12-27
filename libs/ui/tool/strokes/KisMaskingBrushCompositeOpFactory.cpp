/*
 *  Copyright (c) 2017 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "KisMaskingBrushCompositeOpFactory.h"

#include "kis_assert.h"

#include <KoCompositeOpRegistry.h>
#include <KoCompositeOpFunctions.h>

#include "KisMaskingBrushCompositeOp.h"

#include <KoConfig.h>
#ifdef HAVE_OPENEXR
#include <half.h>
#endif /* HAVE_OPENEXR */


namespace {

/**
 * A special Linear Burn variant for alpha channel
 *
 * The meaning of alpha channel is a bit different from the one in color. We should
 * clamp the values around [zero, max] only to avoid the brush to **erase** the content
 * of the layer below
 */

template<class T>
inline T maskingLinearBurn(T src, T dst) {
    using namespace Arithmetic;
    typedef typename KoColorSpaceMathsTraits<T>::compositetype composite_type;
    return qBound(composite_type(KoColorSpaceMathsTraits<T>::zeroValue),
                  composite_type(src) + dst - unitValue<T>(),
                  composite_type(KoColorSpaceMathsTraits<T>::max));
}

/**
 * A special Linear Dodge variant for alpha channel.
 *
 * The meaning of alpha channel is a bit different from the one in color. If
 * alpha channel of the destination is totally null, we should try to resurrect
 * its contents from ashes :)
 */
template<class T>
inline T maskingAddition(T src, T dst) {
    typedef typename KoColorSpaceMathsTraits<T>::compositetype composite_type;
    using namespace Arithmetic;

    if (dst == zeroValue<T>()) {
        return zeroValue<T>();
    }

    return Arithmetic::clamp<T>(composite_type(src) + dst);
}



template <typename channel_type>
KisMaskingBrushCompositeOpBase *createTypedOp(const QString &id, int pixelSize, int alphaOffset)
{
    KisMaskingBrushCompositeOpBase *result = 0;

    if (id == COMPOSITE_MULT) {
        result = new KisMaskingBrushCompositeOp<channel_type, cfMultiply>(pixelSize, alphaOffset);
    } else if (id == COMPOSITE_DARKEN) {
        result = new KisMaskingBrushCompositeOp<channel_type, cfDarkenOnly>(pixelSize, alphaOffset);
    } else if (id == COMPOSITE_OVERLAY) {
        result = new KisMaskingBrushCompositeOp<channel_type, cfOverlay>(pixelSize, alphaOffset);
    } else if (id == COMPOSITE_DODGE) {
        result = new KisMaskingBrushCompositeOp<channel_type, cfColorDodge>(pixelSize, alphaOffset);
    } else if (id == COMPOSITE_BURN) {
        result = new KisMaskingBrushCompositeOp<channel_type, cfColorBurn>(pixelSize, alphaOffset);
    } else if (id == COMPOSITE_LINEAR_BURN) {
        result = new KisMaskingBrushCompositeOp<channel_type, maskingLinearBurn>(pixelSize, alphaOffset);
    } else if (id == COMPOSITE_LINEAR_DODGE) {
        result = new KisMaskingBrushCompositeOp<channel_type, maskingAddition>(pixelSize, alphaOffset);
    } else if (id == COMPOSITE_HARD_MIX) {
        result = new KisMaskingBrushCompositeOp<channel_type, cfHardMix>(pixelSize, alphaOffset);
    }

    KIS_SAFE_ASSERT_RECOVER (result && "Unknown composite op for masking brush!") {
        result = new KisMaskingBrushCompositeOp<channel_type, cfMultiply>(pixelSize, alphaOffset);
    }

    return result;
}

}

KisMaskingBrushCompositeOpBase *KisMaskingBrushCompositeOpFactory::create(const QString &id, KoChannelInfo::enumChannelValueType channelType, int pixelSize, int alphaOffset)
{
    KisMaskingBrushCompositeOpBase *result = 0;

    switch (channelType) {
    case KoChannelInfo::UINT8:
        result = createTypedOp<quint8>(id, pixelSize, alphaOffset);
        break;
    case KoChannelInfo::UINT16:
        result = createTypedOp<quint16>(id, pixelSize, alphaOffset);
        break;
    case KoChannelInfo::UINT32:
        result = createTypedOp<quint32>(id, pixelSize, alphaOffset);
        break;

#ifdef HAVE_OPENEXR
    case KoChannelInfo::FLOAT16:
        result = createTypedOp<half>(id, pixelSize, alphaOffset);
        break;
#endif /* HAVE_OPENEXR */

    case KoChannelInfo::FLOAT32:
        result = createTypedOp<float>(id, pixelSize, alphaOffset);
        break;
    case KoChannelInfo::FLOAT64:
        result = createTypedOp<double>(id, pixelSize, alphaOffset);
        break;
//    NOTE: we have no color space like that, so it is not supported!
//    case KoChannelInfo::INT8:
//        result = createTypedOp<qint8>(id, pixelSize, alphaOffset);
//        break;
    case KoChannelInfo::INT16:
        result = createTypedOp<qint16>(id, pixelSize, alphaOffset);
        break;
    default:
        KIS_SAFE_ASSERT_RECOVER_NOOP(0 && "Unknown channel type for masking brush!");
    }

    return result;
}

QStringList KisMaskingBrushCompositeOpFactory::supportedCompositeOpIds()
{
    QStringList ids;
    ids << COMPOSITE_MULT;
    ids << COMPOSITE_DARKEN;
    ids << COMPOSITE_OVERLAY;
    ids << COMPOSITE_DODGE;
    ids << COMPOSITE_BURN;
    ids << COMPOSITE_LINEAR_BURN;
    ids << COMPOSITE_LINEAR_DODGE;
    ids << COMPOSITE_HARD_MIX;
    return ids;
}
