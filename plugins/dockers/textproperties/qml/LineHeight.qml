/*
 *  SPDX-FileCopyrightText: 2024 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Layouts 1.12
import org.krita.flake.text 1.0

TextPropertyBase {
    propertyName: i18nc("@label", "Line Height");
    propertyType: TextPropertyBase.Character;
    property alias isNormal: lineHeightNormalCbx.checked;
    property alias lineHeight: lineHeightSpn.value;
    property int lineHeightUnit: LineHeightModel.Lines;

    onPropertiesUpdated: {
        blockSignals = true;
        isNormal = properties.lineHeight.isNormal;
        lineHeight = properties.lineHeight.value * lineHeightSpn.multiplier;
        lineHeightUnit = properties.lineHeight.unit;
        visible = properties.lineHeightState !== KoSvgTextPropertiesModel.PropertyUnset;
        blockSignals = false;
    }

    onLineHeightUnitChanged: {
        lineHeightUnitCmb.currentIndex = lineHeightUnitCmb.indexOfValue(lineHeightUnit)
        if (!blockSignals) {
            properties.lineHeight.unit = lineHeightUnit
        }
    }
    onIsNormalChanged: {
        if (!blockSignals) {
            properties.lineHeight.isNormal = isNormal;
        }
    }

    onLineHeightChanged: {
        if (!blockSignals) {
            properties.lineHeight.value = lineHeight / lineHeightSpn.multiplier;
        }
    }

    GridLayout {
        columns: 3;
        columnSpacing: columnSpacing;
        width: parent.width;

        RevertPropertyButton {
            revertEnabled: properties.lineHeightState === KoSvgTextPropertiesModel.PropertySet;
            onClicked: properties.lineHeightState = KoSvgTextPropertiesModel.PropertyUnset;
        }

        Label {
            text: propertyName;
            elide: Text.ElideRight;
            Layout.fillWidth: true;
        }

        CheckBox {
            text: i18nc("@option:check", "Normal")
            id: lineHeightNormalCbx;
            onCheckedChanged: {
                lineHeightSpn.enabled = !checked;
                lineHeightUnitCmb.enabled = !checked;
            }
        }
        Item {
            width: firstColumnWidth;
            height: 1;
        }
        DoubleSpinBox {
            id: lineHeightSpn
            Layout.fillWidth: true;
            enabled: !lineHeightNormalCbx.enabled;
            from: 0;
            to: 999 * multiplier;
        }

        ComboBox {
            id: lineHeightUnitCmb;
            property QtObject spinBoxControl: lineHeightSpn;
            model: [
                { text: i18nc("@label:inlistbox", "Pt"), value: LineHeightModel.Absolute},
                { text: i18nc("@label:inlistbox", "Em"), value: LineHeightModel.Em},
                { text: i18nc("@label:inlistbox", "Ex"), value: LineHeightModel.Ex},
                { text: i18nc("@label:inlistbox", "%"), value: LineHeightModel.Percentage},
                { text: i18nc("@label:inlistbox", "Lines"), value: LineHeightModel.Lines},
            ]
            textRole: "text";
            valueRole: "value";
            enabled: !lineHeightNormalCbx.enabled;

            onActivated: {
                var currentValueInPt = 0;
                if (lineHeightUnit === LineHeightModel.Absolute) {
                    currentValueInPt = spinBoxControl.value;
                } else if (lineHeightUnit === LineHeightModel.Ex) {
                    currentValueInPt = spinBoxControl.value * properties.resolvedXHeight(false);
                } else if (lineHeightUnit === LineHeightModel.Percentage) {
                    currentValueInPt = (spinBoxControl.value / 100) * properties.resolvedFontSize(false);
                } else { // EM, Lines
                    currentValueInPt = spinBoxControl.value * properties.resolvedFontSize(false);
                }

                var newValue = 0;
                if (currentValue === LineHeightModel.Absolute) {
                    newValue = currentValueInPt
                } else if (currentValue === LineHeightModel.Ex) {
                    newValue = currentValueInPt / properties.resolvedXHeight(false);
                } else if (currentValue === LineHeightModel.Percentage) {
                    newValue = (currentValueInPt / properties.resolvedFontSize(false)) * 100;
                } else { // Em, Lines
                    newValue = currentValueInPt / properties.resolvedFontSize(false);
                }
                lineHeightUnit = currentValue;
                spinBoxControl.value = newValue;
            }
        }
    }
}
