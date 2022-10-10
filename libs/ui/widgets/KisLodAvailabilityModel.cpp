/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisLodAvailabilityModel.h"

#include <KisZug.h>

namespace {
KisLodAvailabilityModel::AvailabilityState
calcLodAvailabilityState(const KisLodAvailabilityData &data, qreal effectiveBrushSize, const KisPaintopLodLimitations &l) {
    KisLodAvailabilityModel::AvailabilityState state = KisLodAvailabilityModel::Available;

    if (!l.blockers.isEmpty()) {
        state = KisLodAvailabilityModel::BlockedFully;
    } else if (data.isLodSizeThresholdSupported &&
               effectiveBrushSize < data.lodSizeThreshold) {

        state = KisLodAvailabilityModel::BlockedByThreshold;
    } else if (!l.limitations.isEmpty()) {
        state = KisLodAvailabilityModel::Limited;
    }

    return state;
}
}


KisLodAvailabilityModel::KisLodAvailabilityModel(lager::cursor<KisLodAvailabilityData> _data, lager::reader<qreal> _effectiveBrushSize, lager::reader<KisPaintopLodLimitations> _lodLimitations)
    : data(_data)
    , effectiveBrushSize(_effectiveBrushSize)
    , lodLimitations(_lodLimitations)
    , LAGER_QT(isLodUserAllowed) {data[&KisLodAvailabilityData::isLodUserAllowed]}
    , LAGER_QT(isLodSizeThresholdSupported) {data[&KisLodAvailabilityData::isLodSizeThresholdSupported]}
    , LAGER_QT(lodSizeThreshold) {data[&KisLodAvailabilityData::lodSizeThreshold]}
    , LAGER_QT(availabilityState) {lager::with(data, effectiveBrushSize, lodLimitations).map(&calcLodAvailabilityState)}
    , LAGER_QT(effectiveLodAvailable) {LAGER_QT(availabilityState).xform(kiszug::map_less_equal<int>(static_cast<int>(Limited)))}
{
}
