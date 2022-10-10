/*
 *  SPDX-FileCopyrightText: 2022 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KISMIRROROPTIONWIDGET_H
#define KISMIRROROPTIONWIDGET_H

#include <KisCurveOptionWidget2.h>
#include <KisMirrorOptionData.h>


class PAINTOP_EXPORT KisMirrorOptionWidget : public KisCurveOptionWidget2
{
    Q_OBJECT
public:
    using data_type = KisMirrorOptionData;

    KisMirrorOptionWidget(lager::cursor<KisMirrorOptionData> optionData);
    ~KisMirrorOptionWidget();

    void writeOptionSetting(KisPropertiesConfigurationSP setting) const override;
    void readOptionSetting(const KisPropertiesConfigurationSP setting) override;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KISMIRROROPTIONWIDGET_H
