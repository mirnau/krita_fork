/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2008 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */
#include "kis_brush_option_widget.h"
#include <klocalizedstring.h>

#include <kis_image.h>
#include <kis_image_config.h>

#include "kis_brush_selection_widget.h"
#include "kis_brush.h"

#include <lager/state.hpp>
#include "KisBrushModel.h"
#include "kis_precision_option.h"
#include "kis_paintop_lod_limitations.h"

struct KisBrushOptionWidget::Private
{
    Private()
    {
    }

    lager::state<KisBrushModel::BrushData, lager::automatic_tag> brushData;
    lager::state<KisBrushModel::PrecisionData, lager::automatic_tag> brushPrecisionData;

    KisBrushOptionWidgetFlags flags;
};

KisBrushOptionWidget::KisBrushOptionWidget(KisBrushOptionWidgetFlags flags)
    : KisPaintOpOption(i18n("Brush Tip"), KisPaintOpOption::GENERAL, true),
      m_d(new Private())
{
    m_d->flags = flags;

    m_checkable = false;
    m_brushSelectionWidget = new KisBrushSelectionWidget(KisImageConfig(true).maxBrushSize(), m_d->brushData, m_d->brushPrecisionData, flags);
    m_brushSelectionWidget->hide();
    setConfigurationPage(m_brushSelectionWidget);

    setObjectName("KisBrushOptionWidget");

    // TODO: merge them into a single struct to avoid double updates
    lager::watch(m_d->brushData, std::bind(&KisBrushOptionWidget::emitSettingChanged, this));
    lager::watch(m_d->brushPrecisionData, std::bind(&KisBrushOptionWidget::emitSettingChanged, this));
}

KisBrushSP KisBrushOptionWidget::brush() const
{
    return m_brushSelectionWidget->brush();
}


void KisBrushOptionWidget::setImage(KisImageWSP image)
{
    m_brushSelectionWidget->setImage(image);
}

void KisBrushOptionWidget::writeOptionSetting(KisPropertiesConfigurationSP settings) const
{
    m_d->brushData->write(settings.data());

    if (m_d->flags & KisBrushOptionWidgetFlag::SupportsPrecision) {
        m_d->brushPrecisionData->write(settings.data());
    }
}

void KisBrushOptionWidget::readOptionSetting(const KisPropertiesConfigurationSP setting)
{
    std::optional<KisBrushModel::BrushData> data = KisBrushModel::BrushData::read(setting.data(), resourcesInterface());
    KIS_SAFE_ASSERT_RECOVER_RETURN(data);
    m_d->brushData.set(*data);

    if (m_d->flags & KisBrushOptionWidgetFlag::SupportsPrecision) {
        m_d->brushPrecisionData.set(KisBrushModel::PrecisionData::read(setting.data()));
    }
}

void KisBrushOptionWidget::hideOptions(const QStringList &options)
{
    m_brushSelectionWidget->hideOptions(options);
}

lager::reader<bool> KisBrushOptionWidget::lightnessModeEnabled() const
{
    return m_brushSelectionWidget->lightnessModeEnabled();
}

lager::reader<qreal> KisBrushOptionWidget::effectiveBrushSize() const
{
    using namespace KisBrushModel;
    return m_d->brushData.map(qOverload<const BrushData&>(&KisBrushModel::effectiveSizeForBrush));
}

lager::reader<KisPaintopLodLimitations> KisBrushOptionWidget::lodLimitationsReader() const
{
    return m_d->brushData.map(&KisBrushModel::brushLodLimitations);
}

#include "moc_kis_brush_option_widget.cpp"
